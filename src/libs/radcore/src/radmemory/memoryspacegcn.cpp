//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//==============================================================================
// Include Files 
//==============================================================================

#include <dolphin.h>
#include <radmemorymonitor.hpp>
#include <radmemory.hpp>
#include <raddebug.hpp>
#include <string.h>
#include <radthread.hpp>

#include <dolphin.h>

#define ARAM_SIZE_IN_BYTES ( 1024 * 1024 * 16 )
#define REQUEST_POOL_SIZE 16
#define MAX_ARAM_ALLOCATIONS 20
#define GROW_ARAM_ALLOCATIONS MAX_ARAM_ALLOCATIONS

//==============================================================================
// Globals 
//==============================================================================

unsigned int radMemorySpace_OptimalMultiple = ARQ_DMA_ALIGNMENT;
unsigned int radMemorySpace_OptimalAlignment = ARQ_DMA_ALIGNMENT;

IRadMemoryHeap * g_pIRadMemoryHeap_Aram = NULL;
IRadThreadMutex * g_pIRadThreadMutex = NULL;

//==============================================================================
// Component: radMemorySpaceCopyRequest_Local
//
// Description: Global object that always return true.  We only need one global
//     object because the copy is actually synchronous and we just "trick" the
//     client into thinking he has a separate instance of a copy object.
//==============================================================================

struct radMemorySpaceCopyRequest_Local
    :
    public IRadMemorySpaceCopyRequest
{
    inline radMemorySpaceCopyRequest_Local( );
    inline ~radMemorySpaceCopyRequest_Local( void );
    virtual void AddRef( void );
    virtual void Release( void );
    virtual bool IsDone( void );

    private:
    
        unsigned int m_RefCount;
};

inline radMemorySpaceCopyRequest_Local::radMemorySpaceCopyRequest_Local( )
    :
    m_RefCount( 0 )
{
    radMemoryMonitorIdentifyAllocation( this, g_nameFTech, "radMemorySpaceCopyRequest_Local" );
}

inline radMemorySpaceCopyRequest_Local::~radMemorySpaceCopyRequest_Local( void )
{
    // Just make sure the application is reference counting properly.

    rAssert( m_RefCount == 0 );
}

void radMemorySpaceCopyRequest_Local::AddRef( void )
{
    m_RefCount++;
}

void radMemorySpaceCopyRequest_Local::Release( void )
{
    rAssert( m_RefCount > 0 );
    m_RefCount--; 
}

bool radMemorySpaceCopyRequest_Local::IsDone( void )
{
    return true;
}

// This is our ONE global local copy object.

static radMemorySpaceCopyRequest_Local g_RadMemorySpaceCopyRequest_Local;

// The copy request pool.  We create these objects in a pool because we don't
// want to thrash on malloc() and pools are extremely fast.

static IRadMemoryPool * g_pIRadMemoryPool_CopyRequests = NULL;

//==============================================================================
// Component: radMemorySpaceCopyRequest_Remote
//
// Description:  This class handles copying from ARam to MainRam or MainRam to
//      ARam.  All objects are created in a memory pool.
//==============================================================================

struct radMemorySpaceCopyRequest_Remote
    :
    public IRadMemorySpaceCopyRequest,
    public radBaseObject
{
    public:

        IMPLEMENT_BASEOBJECT( "radMemorySpaceCopyRequest_Remote" )

        radMemorySpaceCopyRequest_Remote( void * pDest, radMemorySpace spaceDest,
	        const void * pSrc, radMemorySpace spaceSrc, unsigned int bytes );
        
        ~radMemorySpaceCopyRequest_Remote( void );

        virtual void AddRef( void );
        virtual void Release( void );
	    virtual bool IsDone( void );

        void * operator new( size_t size );
        void operator delete( void * pMemory );

    private:

        static void AramQueueCallback( ARQRequest * pRequest );
    
        ARQRequest m_ArqRequest;
        unsigned int m_RefCount;   

        static IRadMemoryPool * s_pIRadMemoryPool;
};

void * radMemorySpaceCopyRequest_Remote::operator new( size_t size )
{
    rAssert( g_pIRadMemoryPool_CopyRequests != NULL );
    rAssert( g_pIRadThreadMutex != NULL );

    g_pIRadThreadMutex->Lock( );
    
    void * pMemory = g_pIRadMemoryPool_CopyRequests->GetMemory( size );

    g_pIRadThreadMutex->Unlock( );

    return pMemory;
}

void radMemorySpaceCopyRequest_Remote::operator delete( void * pMemory )
{
    g_pIRadThreadMutex->Lock( );

    g_pIRadMemoryPool_CopyRequests->FreeMemory( pMemory );

    g_pIRadThreadMutex->Unlock( );
}

radMemorySpaceCopyRequest_Remote::radMemorySpaceCopyRequest_Remote( void * pDest,
    radMemorySpace spaceDest, const void * pSrc, radMemorySpace spaceSrc,
    unsigned int bytes )
    :
    m_RefCount( 0 )
{
    radMemoryMonitorIdentifyAllocation( this, g_nameFTech, "radMemorySpaceCopyRequest_Remote" );
    if ( spaceSrc == radMemorySpace_Main )
    {
        DCFlushRange( (void*) pSrc, bytes );
    }
    else
    {
        DCInvalidateRange( (void*) pDest, bytes );
    }  
              
    ARQPostRequest
    (
        & m_ArqRequest,
        (unsigned long) this,
        spaceDest == radMemorySpace_Aram ? ARQ_TYPE_MRAM_TO_ARAM : ARQ_TYPE_ARAM_TO_MRAM,
        ARQ_PRIORITY_LOW,
        (unsigned long) pSrc,
        (unsigned long) pDest,
        bytes,
        (ARQCallback) AramQueueCallback
    );
}

void radMemorySpaceCopyRequest_Remote::AddRef( void )
{
    m_RefCount++;
}

void radMemorySpaceCopyRequest_Remote::Release( void )
{
    rAssert( m_RefCount > 0 );

    if ( m_RefCount == 1 )
    {
        delete this;
    }
    else
    {
        m_RefCount--;
    }
}

bool radMemorySpaceCopyRequest_Remote::IsDone( void )
{
    return m_ArqRequest.owner == NULL;
}

radMemorySpaceCopyRequest_Remote::~radMemorySpaceCopyRequest_Remote( void )
{
    rAssertMsg( IsDone( ) == true, "You can't release a async copy request until it is done." );
}

void radMemorySpaceCopyRequest_Remote::AramQueueCallback
(
    ARQRequest * pRequest
)
{
    pRequest->owner = NULL;
}

//==============================================================================
// ::radMemorySpaceCopyAsync( )
//==============================================================================

IRadMemorySpaceCopyRequest * radMemorySpaceCopyAsync(
	void * pDest, radMemorySpace spaceDest,
	const void * pSrc, radMemorySpace spaceSrc, unsigned int bytes )
{
    if ( spaceDest != spaceSrc )
    {
        return new radMemorySpaceCopyRequest_Remote(
            pDest, spaceDest, pSrc, spaceSrc, bytes );
    }
    else if ( spaceDest == radMemorySpace_Main && spaceSrc == radMemorySpace_Main )
    {
        // Just do the synchronous copy and return a placeholder global object
        // that always returns true for IsDone( );

        rAssert( pDest != NULL );
        rAssert( pSrc  != NULL );
        rAssert( bytes > 0 );

        ::memcpy( pDest, pSrc, bytes );

        return & g_RadMemorySpaceCopyRequest_Local;
    }

    rAssert( false ); // Not implemented

    return NULL;
}

//==============================================================================
// ::radMemorySpaceAlloc( )
//
// Description: Allocates memory in specified memory space
//==============================================================================

void * radMemorySpaceAlloc( 
	radMemorySpace space, radMemoryAllocator allocator,	unsigned int numBytes )
{
    if ( space == ::radMemorySpace_Main )
    {
        return ::radMemoryAlloc( allocator, numBytes );
    }
    else if ( space == radMemorySpace_Aram )
    {
        return g_pIRadMemoryHeap_Aram->GetMemory( numBytes );
    }
    else
    {
        rAssert( 0 );
        return NULL;
    }
}

//==============================================================================
// ::radMemorySpaceAllocAligned( )
//
// Description: Allocates aligned memory in specified memory space
//==============================================================================

void * radMemorySpaceAllocAligned( 
	radMemorySpace space, radMemoryAllocator allocator,	
	unsigned int numBytes, unsigned int alignment )
{
    if ( space == radMemorySpace_Main )
    {
        return ::radMemoryAllocAligned( allocator, numBytes, alignment );
    }
    else if ( space == radMemorySpace_Aram )
    {
        return g_pIRadMemoryHeap_Aram->GetMemoryAligned( numBytes, alignment );
    }
    else
    {
        rAssert( 0 );
        return NULL;
    }
}

//==============================================================================
// ::radMemorySpaceFree( )
//
// Frees memory in specified memory space
//==============================================================================

void radMemorySpaceFree( radMemorySpace space, radMemoryAllocator allocator, void * pMemory )
{
    if ( space == radMemorySpace_Main )
    {
        ::radMemoryFree( allocator, pMemory );
    }
    else if ( space == radMemorySpace_Aram )
    {
        g_pIRadMemoryHeap_Aram->FreeMemory( pMemory );
    }
    else
    {
        rAssert( 0 ); // Invalid memory space
    }
}

//==============================================================================
// ::radMemorySpaceFreeAligned( )
//
// Description: Frees aligned memory in specified memory space 
//==============================================================================

void radMemorySpaceFreeAligned( radMemorySpace space, radMemoryAllocator allocator,	void * pMemory )
{
    if ( space == radMemorySpace_Main )
    {
        return ::radMemoryFreeAligned( allocator, pMemory );
    }
    else if ( space == radMemorySpace_Aram )
    {
        g_pIRadMemoryHeap_Aram->FreeMemoryAligned( pMemory );
    }
    else
    {
        rAssert( 0 ); // Invalid memory space
    }
}

//==============================================================================
// ::radMemorySpaceGetAllocator( )
//==============================================================================

IRadMemoryAllocator * radMemorySpaceGetAllocator( radMemorySpace space, radMemoryAllocator allocator )
{
    if ( space == radMemorySpace_Main )
    {
        return ::radMemoryGetAllocator( allocator );
    }
    else if ( space == radMemorySpace_Aram )
    {
        return g_pIRadMemoryHeap_Aram;
    }
    else
    {
        rAssert( 0 );
        return NULL;
    }
}

//==============================================================================
// ::radMemorySpaceInititlize( )
//
// Description: This function is called from radMemoryInitialize( )
//==============================================================================

void radMemorySpaceInitialize( unsigned int aramSizeInBytes )
{
    // You MUST call this function, even if you manage all the client area of
    // aram yourself.  It intializes the dma controller.

    ::ARInit( NULL, 0 );

    rAssert( g_pIRadMemoryPool_CopyRequests == NULL );
    rAssert( g_pIRadThreadMutex == NULL );

    ::radThreadCreateMutex( & g_pIRadThreadMutex, RADMEMORY_ALLOC_DEFAULT );
    rAssert( g_pIRadThreadMutex != NULL );

    ::radMemoryCreatePool( & g_pIRadMemoryPool_CopyRequests,
                              sizeof( radMemorySpaceCopyRequest_Remote ), // element size
                              REQUEST_POOL_SIZE, // num elements
						      REQUEST_POOL_SIZE, // grow size
                              false, // Thread safe
                              None, // Debug level 
   					          RADMEMORY_ALLOC_DEFAULT,
                              "MemorySpace Copy Requests" ); // This will always be malloc( )

    // Get the start of the USER area of aram.  The OS reserves a hunk at the
    // beginning.

    unsigned int aramStart = ::ARGetBaseAddress( );
    
    ::radMemoryExternalHeapCreate( & g_pIRadMemoryHeap_Aram,
                                 (void*) aramStart,
                                 aramSizeInBytes - aramStart,
                                 radMemorySpace_Aram,
                                 MAX_ARAM_ALLOCATIONS,
                                 GROW_ARAM_ALLOCATIONS,
          					     RADMEMORY_ALLOC_DEFAULT ); 
}

//==============================================================================
// ::radMemorySpaceTerminate( )
//
// Description: This function is called from radMemoryTerminate( )
//==============================================================================

void radMemorySpaceTerminate( void )
{
    rAssert( g_pIRadMemoryPool_CopyRequests != NULL );
    rAssert( g_pIRadMemoryHeap_Aram != NULL );

    g_pIRadMemoryPool_CopyRequests->Release( );
    g_pIRadMemoryPool_CopyRequests = NULL;
    g_pIRadMemoryHeap_Aram->Release( );
    g_pIRadMemoryHeap_Aram = NULL;

    g_pIRadThreadMutex->Release( );
    g_pIRadThreadMutex = NULL;
}