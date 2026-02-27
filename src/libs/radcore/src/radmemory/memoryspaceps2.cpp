//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        memoryspaceps2.cpp
//
// Subsystem:	Foundation Technologies - Memory Operation Services
//
// Description:	This file contains the ps2 implementation of the 
//              remote memory space operations
//
// Revisions:   13-Oct-2001     Creation     Rob
//
//=============================================================================

//=============================================================================
// Include Files
//=============================================================================

#include <radmemorymonitor.hpp>
#include <radmemory.hpp>
#include <radtime.hpp>
#include <eekernel.h>
#include <sif.h>
#include <sifdev.h>
#include <string.h>
#include <radplatform.hpp>
#include <radthread.hpp>

#include "memoryspaceps2.hpp"

//=============================================================================
// Globals
//=============================================================================

unsigned int radMemorySpace_OptimalAlignment = 128;
unsigned int radMemorySpace_OptimalMultiple  = 128;

//
// Memory pool for all non-local request objects
//
// Note that this pool holds multiple types of request, similar to a union.
//

#define REQUEST_POOL_SIZE 32
#define REQUEST_POOL_GROW_SIZE REQUEST_POOL_SIZE

static IRadMemoryPool * g_pIRadMemoryPool;

//
// One global object for local copy requests that always returns IsDone() == true
//
static MemorySpaceAsyncRequest_CopyEeToEe g_MemorySpaceAsyncRequest_CopyEeToEe_PlaceHolder;

//
// Lock on static list.
//

static IRadThreadMutex * g_pIRadThreadMutex = NULL;

static unsigned int gRequestStartTime;
static unsigned int gTotalRequests;
static unsigned int gTotalTime;

//=============================================================================
// Macros
//=============================================================================

#define IOP_MEMORY_SIZE ( 1024 * 1024 * 2 ) // Size of consumer IOP memory

//
// Iop to Iop reads go Iop --> Ee --> Iop.  This is our static transfer buffer
// size that we will chunk up our reads in.
//

#define IOP_TO_IOP_TRANSFER_BUFFER_SIZE ( 1024 * 64 )

//=============================================================================
// Static Member Init
//=============================================================================

char * MemorySpaceAsyncRequest_CopyIopToIop::s_pTransferBuffer = NULL;
unsigned int MemorySpaceAsyncRequest_CopyIopToIop::s_TransferBufferSize = 0;

MemorySpaceAsyncRequest * MemorySpaceAsyncRequest::s_pRequest_Head = NULL;

struct radMemoryAllocator_Iop : public IRadMemoryAllocator
{
    virtual void AddRef( void ) { }
    virtual void Release( void ) { }

    virtual void* GetMemory( unsigned int size )
    {
        // rReleasePrintf( "CORE: REQUEST FOR IOP MEMORY: [0x%x] bytes\n", size );
        
        void * pMemory;

        if ( size > 0 )
        {
            pMemory = ::sceSifAllocIopHeap( size );

            rReleaseAssertMsg( pMemory != NULL, "Out of Iop Memory" );
        }
        else
        {
            pMemory = NULL;
        }

        #ifdef RADMEMORYMONITOR

            if ( pMemory != NULL )
            {
                radMemoryMonitorDeclareAllocation(
                    pMemory,
                    size,
                    MM_MAX_CALLSTACK_DEPTH,
                    radMemorySpace_Iop );
            }

        #endif RADMEMORYMONITOR        
                
        return pMemory;
    }

	virtual void  FreeMemory( void* pMemory )
    {
        if ( pMemory != NULL )
        {
            ::sceSifFreeIopHeap( pMemory );

            radMemoryMonitorRescindAllocation(
                pMemory,
                radMemorySpace_Iop );
                
        }
    }

    virtual bool  CanFreeMemory( void * pMemory )
    {
        rAssertMsg( false, "Not Supported" );
        return false;
    }

    virtual void* GetMemoryAligned( unsigned int size, unsigned int alignment )
    {
        void * pMemory = GetMemory( size );

        rAssert( pMemory == NULL || (alignment == 0) || (((unsigned int)pMemory) % alignment) == 0 );

        return pMemory;
    }

	virtual void  FreeMemoryAligned( void * pMemory ) 
    {
        FreeMemory( pMemory );
    }

    virtual bool  CanFreeMemoryAligned( void * pMemory )
    {
        rAssertMsg( false, "Not supported" );
        return false;
    }
};

radMemoryAllocator_Iop g_RadMemoryAllocator_Iop;

IRadMemoryAllocator * radMemorySpaceGetAllocator( radMemorySpace memorySpace, radMemoryAllocator allocator )
{
    if ( memorySpace == radMemorySpace_Ee )
    {
        return ::radMemoryGetAllocator( allocator );
    }
    else if ( memorySpace == radMemorySpace_Iop )
    {
        return & g_RadMemoryAllocator_Iop;
    }
    else if ( memorySpace == radMemorySpace_Sound )
    {
        rAssert( false ); // Must go through sound system for this
    }
    else
    {
        rAssert( false ); // bad enum value
    }

    return NULL;
}

//============================================================================
// radMemorySpaceInitialize
//============================================================================

void radMemorySpaceInitialize( void )
{    
    rAssert( g_pIRadMemoryPool == NULL );
    rAssert( g_pIRadThreadMutex == NULL );

    //
    // Create a mutex for protecting our request list.
    //

    ::radThreadCreateMutex( & g_pIRadThreadMutex, RADMEMORY_ALLOC_DEFAULT );
    rAssert( g_pIRadThreadMutex );

    // Find out the size of our largest request object, this will be our
    // pool element size.  This information is known at compile time, but
    // I can't figure out how to resolve it into a number at compile time.


    // MemorySpaceAsyncRequest_CopyEeToEe is a special case, we don't
    // need to new up an object beacuse it is synchronous.

    unsigned int elementSize = sizeof( MemorySpaceAsyncRequest_CopyIopToIop );

    if ( sizeof( MemorySpaceAsyncRequest_CopyIopToEe ) > elementSize )
    {
        elementSize = sizeof( MemorySpaceAsyncRequest_CopyIopToEe );
    }

    if ( sizeof( MemorySpaceAsyncRequest_CopyEeToIop ) > elementSize )
    {
        elementSize = sizeof( MemorySpaceAsyncRequest_CopyEeToIop );
    }

    radMemoryCreatePool( & g_pIRadMemoryPool,
                            elementSize, 
                            REQUEST_POOL_SIZE,
						    REQUEST_POOL_GROW_SIZE, // Grow
                            false, // Thread safe
                            Moderate, // Debug Level
   					        RADMEMORY_ALLOC_DEFAULT,
                            "MemorySpace Copy Request" ); // This will always be malloc()

    MemorySpaceAsyncRequest_CopyIopToIop::Initialize( IOP_TO_IOP_TRANSFER_BUFFER_SIZE );
}

//============================================================================
// radMemorySpaceTerminate
//============================================================================

void radMemorySpaceTerminate( void )
{
    rAssert( g_pIRadMemoryPool != NULL );
    g_pIRadMemoryPool->Release( );
    g_pIRadMemoryPool = NULL;

    MemorySpaceAsyncRequest_CopyIopToIop::Terminate( );

    g_pIRadThreadMutex->Release( );
    g_pIRadThreadMutex = NULL;
}


//============================================================================
// radMemoryAlloc
//============================================================================

void * radMemorySpaceAlloc( radMemorySpace space, radMemoryAllocator allocator, unsigned int numBytes )
{
    void * pMemory = NULL;

	if( space == radMemorySpace_Ee )
	{
		pMemory = ::radMemoryAlloc( allocator, numBytes );
	}
	else if( space == radMemorySpace_Iop )
	{
		pMemory = g_RadMemoryAllocator_Iop.GetMemory( numBytes );
	}
	else
	{
		rAssertMsg( false, "radMemoryAlloc: Unsupported memory space.\n" );
	}

    return pMemory;
}

//============================================================================
// radMemoryAllocAligned
//============================================================================

void * radMemorySpaceAllocAligned
( 
	radMemorySpace space, 
	radMemoryAllocator allocator,
	unsigned int numBytes, 
	unsigned int alignment 
)
{
    void * pMemory = NULL;

	if( space == radMemorySpace_Ee )
	{
		pMemory = ::radMemoryAllocAligned( allocator, numBytes, alignment );
	}
    else if ( space == radMemorySpace_Iop )
    {
        pMemory = g_RadMemoryAllocator_Iop.GetMemoryAligned( numBytes, alignment );
    }
	else
	{
		rAssertMsg( false, "radMemoryAllocAligned: Unsupported memory space.\n" );
	}

    return pMemory;
}

//============================================================================
// radMemoryFree
//============================================================================

void radMemorySpaceFree
(  
	radMemorySpace space, 
	radMemoryAllocator allocator,
	void * pMemory
)
{
	if( space == radMemorySpace_Ee )
	{
		::radMemoryFree( allocator, pMemory );
	}
	else if( space == radMemorySpace_Iop )
	{
		g_RadMemoryAllocator_Iop.FreeMemory( pMemory );
	}
	else
	{
		rAssertMsg( false, "radMemoryFree: Unsupported memory space.\n" );
	}
}

//============================================================================
// radMemoryFreeAligned
//============================================================================

void radMemorySpaceFreeAligned
( 
	radMemorySpace space, 
	radMemoryAllocator allocator,
	void * pMemory
)
{
	if( space == radMemorySpace_Ee )
	{
		::radMemoryFreeAligned( allocator, pMemory );
	}
	else if ( space == radMemorySpace_Iop )
	{
		g_RadMemoryAllocator_Iop.FreeMemoryAligned( pMemory );
	}
    else
    {
		rAssertMsg( false, "radMemoryFree: Unsupported memory space.\n" );
    }

}

//============================================================================
// radMemoryCopyAsync
//============================================================================

IRadMemorySpaceCopyRequest * radMemorySpaceCopyAsync
(
	void * pDest,
	radMemorySpace spaceDest,
	const void * pSrc,
	radMemorySpace spaceSrc,
	unsigned int bytes
)
{
	// Return a new request object based on the source and destination memory spaces

	if( spaceDest == radMemorySpace_Ee && spaceSrc == radMemorySpace_Ee )
	{
        rAssert( pDest != NULL );
        rAssert( pSrc != NULL );

        ::memcpy( pDest, pSrc, bytes );

        return & g_MemorySpaceAsyncRequest_CopyEeToEe_PlaceHolder;
    }
	else if( spaceDest == radMemorySpace_Iop && spaceSrc == radMemorySpace_Ee )
    {
		MemorySpaceAsyncRequest * pR = new( "MemorySpaceAsyncRequest_CopyEeToIop" ) MemorySpaceAsyncRequest_CopyEeToIop( pDest, pSrc, bytes );
        MemorySpaceAsyncRequest::QueueRequest( pR );
        return pR;
	}
	else if( spaceDest == radMemorySpace_Ee && spaceSrc == radMemorySpace_Iop )
	{
		MemorySpaceAsyncRequest * pR = new( "MemorySpaceAsyncRequest_CopyIopToEe" ) MemorySpaceAsyncRequest_CopyIopToEe( pDest, pSrc, bytes );
        MemorySpaceAsyncRequest::QueueRequest( pR );
        return pR;
	}
	else if( spaceDest == radMemorySpace_Iop && spaceSrc == radMemorySpace_Iop )
	{
		MemorySpaceAsyncRequest * pR = new( "MemorySpaceAsyncRequest_CopyIopToIop" ) MemorySpaceAsyncRequest_CopyIopToIop( pDest, pSrc, bytes );
        MemorySpaceAsyncRequest::QueueRequest( pR );
        return pR;
	}

    rAssertMsg( false, "radMemoryCopyAsync: Unsupported source/dest memory space combination.\n" );
	return NULL;
}

//============================================================================
//
// struct MemorySpaceAsyncRequest_CopyIopToEe
//
//============================================================================

MemorySpaceAsyncRequest_CopyEeToEe::MemorySpaceAsyncRequest_CopyEeToEe( void )
    :
    m_RefCount( 0 )
{
    radMemoryMonitorIdentifyAllocation( this, g_nameFTech, "MemorySpaceAsyncRequest_CopyEeToEe" );
}

MemorySpaceAsyncRequest_CopyEeToEe::~MemorySpaceAsyncRequest_CopyEeToEe( void )
{
    rAssert( m_RefCount == 0 ); // game has refcounting problem.
}

bool MemorySpaceAsyncRequest_CopyEeToEe::IsDone( void )
{
    return true;
}

void MemorySpaceAsyncRequest_CopyEeToEe::AddRef( void )
{
    m_RefCount++;
}

void MemorySpaceAsyncRequest_CopyEeToEe::Release( void )
{
    rAssert( m_RefCount > 0 );
    m_RefCount--;
}

//============================================================================
//
// struct MemorySpaceAsyncRequest
//
//============================================================================

MemorySpaceAsyncRequest::MemorySpaceAsyncRequest( void )
    :
    m_pRequest_Next( NULL ),
    m_RefCount( 0 )
{

}

MemorySpaceAsyncRequest::~MemorySpaceAsyncRequest( void )
{
}

void * MemorySpaceAsyncRequest::operator new( size_t size, const char * pIdentifier )
{
    g_pIRadThreadMutex->Lock( );

    void * pMemory = g_pIRadMemoryPool->GetMemory( size );

    g_pIRadThreadMutex->Unlock( );

    radMemoryMonitorIdentifyAllocation( pMemory, g_nameFTech, pIdentifier );

    return pMemory;
}

void MemorySpaceAsyncRequest::operator delete( void * pMemory  )
{
    g_pIRadThreadMutex->Lock( );

    g_pIRadMemoryPool->FreeMemory( pMemory );

    g_pIRadThreadMutex->Unlock( );
}


/* static */ void MemorySpaceAsyncRequest::ServiceHead( void )
{
    // Called by derived class IsDone(), each request's IsDone() is
    // responsible for servicing the head request so that the game doesn't
    // "lock up" spinning on an inactive request in the queue.

    g_pIRadThreadMutex->Lock( );

    bool done = true;

    while( s_pRequest_Head != NULL && done == true )
    {
        do
        {
            done = s_pRequest_Head->Service( );
        } while( ! done );

        if ( done )
        {
            unsigned int dif = radTimeGetMicroseconds( ) - gRequestStartTime;
            gTotalRequests ++;
            gTotalTime += dif;
                        
           // rReleasePrintf( "Async request took: [%d] usec, ave: [%d] usec\n", dif, gTotalTime / gTotalRequests );
            
            s_pRequest_Head = s_pRequest_Head->m_pRequest_Next;

            if ( s_pRequest_Head != NULL )
            {
                gRequestStartTime = radTimeGetMicroseconds( );            
                s_pRequest_Head->DoRequest( );
            }   
        }
    }

    g_pIRadThreadMutex->Unlock( );
}

void MemorySpaceAsyncRequest::QueueRequest( MemorySpaceAsyncRequest * pRequest )
{
    g_pIRadThreadMutex->Lock( );

    // Called by derived object constructors.

    // Add request to the end of the request list.

    if ( s_pRequest_Head == NULL )
    {
        // This is the only active request, so start it, otherwise the
        // current request will start us when it is done.

        s_pRequest_Head = pRequest;
        
        gRequestStartTime = radTimeGetMicroseconds( );
        pRequest->DoRequest( );
    }
    else
    {
        // There is another request oustanding, add ourselves to the
        // end of the request list.

        MemorySpaceAsyncRequest * pSearch = s_pRequest_Head;
          
        while ( pSearch != NULL )
        {
            if ( pSearch->m_pRequest_Next == NULL )
            {
                pSearch->m_pRequest_Next = pRequest;
                break;
            }

            pSearch = pSearch->m_pRequest_Next;
        }
    }

    g_pIRadThreadMutex->Unlock( );

    ServiceHead( );
}

//============================================================================
//
// struct MemorySpaceAsyncRequest_CopyEeToIop
//
//============================================================================

//============================================================================
// MemorySpaceAsyncRequest_CopyEeToIop::MemorySpaceAsyncRequest_CopyEeToIop
//============================================================================

MemorySpaceAsyncRequest_CopyEeToIop::MemorySpaceAsyncRequest_CopyEeToIop(
	void * pDest,
	const void * pSrc,
	unsigned int bytes )
    :
    m_pDest( pDest ),
    m_pSource( pSrc ),
    m_Bytes( bytes ),
    m_State( Queued )
 {
	//
	// Ensure destination is multiple 64 byte aligned.
	// Source is 16 byte algined.
	// size is multiple of 16.
	//
	rAssertMsg( (bytes & 0xf) == 0, "radMemoryCpyAsync: Cached reads must be a multiple of 16 bytes." );
	rAssert( ((unsigned int) pDest & 0x3) == 0 );
	rAssert( ((unsigned int) pSrc & 0xf) == 0 );

    radMemoryMonitorIdentifyAllocation( this, g_nameFTech, "MemorySpaceAsyncRequest_CopyEeToIop" );
}

MemorySpaceAsyncRequest_CopyEeToIop::~MemorySpaceAsyncRequest_CopyEeToIop
( 
	void
)
{
    rAssertMsg( m_State == Done, "You cannot release a copy request until it is done" );
}

void MemorySpaceAsyncRequest_CopyEeToIop::DoRequest( void )
{
    rAssert( m_State == Queued );

    m_State = Waiting;
}

bool MemorySpaceAsyncRequest_CopyEeToIop::Service( void )
{
    State oldState;

    do
    {
        oldState = m_State;

        if ( m_State == Waiting )
        {
	        sceSifDmaData dmaData;

	        FlushCache( 0 );

	        dmaData.data = (unsigned int) m_pSource;
	        dmaData.addr = (unsigned int) m_pDest;   
	        dmaData.mode = 0x00; // This caused some big trouble
	        dmaData.size = m_Bytes;

	        m_TransactionId = sceSifSetDma(&dmaData, 1 );

	        rAssert( m_TransactionId != 0 );

            m_State = Started;
        }
        else if ( m_State == Started )
        {
	        if ( sceSifDmaStat( m_TransactionId ) < 0 )
            {
                m_State = Done;

                return true;
            }
        }
        else
        {
            rAssert( false );
        }
    } while( oldState != m_State );

    return false;
}

/* virtual */ bool MemorySpaceAsyncRequest_CopyEeToIop::IsDone( void )
{
    // Service head of list to keep system going, this could be this object.

    ServiceHead( );   

    return m_State == Done;
}

//============================================================================
// MemorySpaceAsyncRequest_CopyIopToEe::MemorySpaceAsyncRequest_CopyIopToEe
//============================================================================

MemorySpaceAsyncRequest_CopyIopToEe::MemorySpaceAsyncRequest_CopyIopToEe(
	void * pDest,
	const void * pSrc,
	unsigned int bytes )
    :
    m_pDest( pDest ),
    m_pSource( pSrc ),
    m_Bytes( bytes ),
    m_State( Queued )
{

    rAssertMsg( (bytes & 0xf) == 0, "radMemoryCopyAsync: Cached reads must be a multiple of 16 bytes." );
    rAssert( ((unsigned int) pSrc & 0x3) == 0 );
    rAssert( ((unsigned int) pDest & 0x40) == 0 ); // 64 byte cache alignment
                
    radMemoryMonitorIdentifyAllocation( this, g_nameFTech, "MemorySpaceAsyncRequest_CopyIopToEe" );
}

void MemorySpaceAsyncRequest_CopyIopToEe::DoRequest( void )
{
    rAssert( m_State == Queued );

    m_State = Waiting;
}

MemorySpaceAsyncRequest_CopyIopToEe::~MemorySpaceAsyncRequest_CopyIopToEe
( 
	void
)
{
    rAssertMsg( m_State == Done, "You cannot release a copy request until it is done" );
}

bool MemorySpaceAsyncRequest_CopyIopToEe::Service( void )
{
    State oldState;

    do
    {
        oldState = m_State;

        if ( m_State == Waiting )
        {
            FlushCache( 0 );

	        int ret = ::sceSifGetOtherData
            (
                & m_ReceiveData,
                ( void * )m_pSource,
                m_pDest,
                m_Bytes,
                SIF_RPCM_NOWAIT
            );

	        rAssert( ret >= 0 );

            m_State = Started;
        }
        if ( m_State == Started )
        {
            if ( sceSifCheckStatRpc( (sceSifRpcData * ) & m_ReceiveData ) == 0 )
            {
                m_State = Done;

                return true;
            }
        }
        else
        {
            rAssert( false );
        }
    } while ( oldState != m_State );

    return false;
}


/* virtual */ bool MemorySpaceAsyncRequest_CopyIopToEe::IsDone( void )
{
    ServiceHead( );
    
    return m_State == Done;
}

//============================================================================
//
// struct MemorySpaceAsyncRequest_CopyIopToIop
//
//============================================================================

//============================================================================
// (static) MemorySpaceAsyncRequest_CopyIopToIop::Initialize
//============================================================================

void MemorySpaceAsyncRequest_CopyIopToIop::Initialize(
    unsigned int transferBufferSize )
{
    // We chunk up our reads, and can only do one iop to iop transfer at a
    // time.

    rAssert( s_pTransferBuffer == NULL );
    s_TransferBufferSize = radMemoryRoundUp( transferBufferSize, radMemorySpace_OptimalMultiple );
    
    s_pTransferBuffer = (char*) ::radMemoryAllocAligned(
        RADMEMORY_ALLOC_DEFAULT, 
        IOP_TO_IOP_TRANSFER_BUFFER_SIZE,
        radMemorySpace_OptimalAlignment );

    rAssert( s_pTransferBuffer != NULL );
}

//============================================================================
// (static) MemorySpaceAsyncRequest_CopyIopToIop::Termainte
//============================================================================

void MemorySpaceAsyncRequest_CopyIopToIop::Terminate( void )
{
    // Free up our "static" transfer buffer.

    rAssert( s_pTransferBuffer != NULL );

    ::radMemoryFreeAligned( RADMEMORY_ALLOC_DEFAULT, s_pTransferBuffer );

    s_pTransferBuffer = NULL;
    s_TransferBufferSize = 0;
}

//============================================================================
// MemorySpaceAsyncRequest_CopyIopToIop::MemorySpaceAsyncRequest_CopyIopToIop
//============================================================================

MemorySpaceAsyncRequest_CopyIopToIop::MemorySpaceAsyncRequest_CopyIopToIop
(
	void * pDest, 
	const void * pSrc, 
	unsigned int bytes
)
    :
    m_SourceAddress( (unsigned int) pSrc ),
	m_DestinationAddress( (unsigned int) pDest ),
	m_BytesToCopy( bytes ),
    m_BytesCopied( 0 ),
    m_BytesCopying( 0 ),
    m_State( Queued )
{
    radMemoryMonitorIdentifyAllocation( this, g_nameFTech, "MemorySpaceAsyncRequest_CopyIopToIop" );
	// Number of bytes must be multiple of 16
	// Source and Destination must be aligned to 4 Bytes (Both on IOP)

	rAssertMsg( (m_BytesToCopy & 0xf) == 0, "radMemoryCopyAsync: Cached reads must be a multiple of 16 bytes." );
    rAssert( (m_SourceAddress & 0x3) == 0 );
    rAssert( (m_DestinationAddress & 0x3) == 0 );
}

//============================================================================
// MemorySpaceAsyncRequest_CopyIopToIop::~MemorySpaceAsyncRequest_CopyIopToIop
//============================================================================

MemorySpaceAsyncRequest_CopyIopToIop::~MemorySpaceAsyncRequest_CopyIopToIop
( 
	void
)
{
    rAssertMsg( m_State == Done, "You cannot release a copy request until it is done" );
}

//============================================================================
// MemorySpaceAsyncRequest_CopyIopToIop::StartTransfer
//============================================================================

void MemorySpaceAsyncRequest_CopyIopToIop::DoRequest( void )
{
    // Just set our state and service the state machine.

    rAssert( m_State == Queued );

    m_State = Waiting;
}

//============================================================================
// MemorySpaceAsyncRequest_CopyIopToIop::IsDone
//============================================================================

/* virtual */ bool MemorySpaceAsyncRequest_CopyIopToIop::IsDone( void )
{
    ServiceHead( );

    return m_State == Done;
}

//============================================================================
// MemorySpaceAsyncRequest_CopyIopToIop::Service
//============================================================================

bool MemorySpaceAsyncRequest_CopyIopToIop::Service( void )
{
    // Our big state machine, this is fairly straight forward--move a chunk
    // from IOPsource --> transfer buffer, then move transfer buffer -->
    // IOPtarget.

    State oldState;

    do
    {
        oldState = m_State;

        switch( m_State )
        {
            case Waiting:
            {
                m_State = Started;
            }
            case Started: // fall through
            case EeToIopDone:
            {
                m_BytesCopied += m_BytesCopying;

                if ( m_BytesCopied == m_BytesToCopy )
                {
                    m_State = Done;

                    return true;

                    break;
                }

                m_State = IopToEe;
                        
                m_BytesCopying = ( m_BytesToCopy - m_BytesCopied );

                if ( m_BytesCopying > s_TransferBufferSize )
                {
                    m_BytesCopying = s_TransferBufferSize;
                }

                rAssert( m_BytesCopying > 0 );

                FlushCache( 0 );

	            int ret = ::sceSifGetOtherData
                (
                    & m_ReceiveData,
                    (void*)(m_SourceAddress + m_BytesCopied),
                    (void*)(s_pTransferBuffer),
                    m_BytesCopying,
                    SIF_RPCM_NOWAIT
                );

	            rAssert( ret >= 0 );
                break;
            }
            case IopToEe:
            {
		        if( sceSifCheckStatRpc( (sceSifRpcData* ) & m_ReceiveData ) == 0 )
		        {
                    // Done

                    m_State = IopToEeDone;
                }

                break;
            }

            case IopToEeDone:
            {
                m_State = EeToIop;

			    sceSifDmaData dmaData;

			    dmaData.data = (unsigned int) s_pTransferBuffer;
			    dmaData.addr = m_DestinationAddress + m_BytesCopied;   
			    dmaData.mode = 0x00;
			    dmaData.size = m_BytesCopying;

			    m_TransactionId = sceSifSetDma( & dmaData, 1 );

			    rAssert( m_TransactionId != 0 );

                break;

            }
            case EeToIop:
            {
		        if( sceSifDmaStat( m_TransactionId ) < 0 )
		        {
                    m_State = EeToIopDone;
                }
            
                break;
            }
            case Queued:
            case Done:
            default:
            {
                rAssert( false ); // should never get here.
                break;
            }
        }
    }
    while( oldState != m_State );

    return false;
}




