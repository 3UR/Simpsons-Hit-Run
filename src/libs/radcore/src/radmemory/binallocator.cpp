//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//==============================================================================
// File: binallocator.cpp
//
// Created April 10, 2002 - Tim Hinds
//==============================================================================

#include "pch.hpp"
#include <stdlib.h> //qsort
#include "binallocator.hpp"
#include <radmemorymonitor.hpp>

//==============================================================================
// Macros
//==============================================================================

// Check to make sure the game isn't aligning to silly values.  This could
// waste a tonne of memory.

#define HUGE_ALIGNMENT_VALUE 256

//==============================================================================
// radMemoryBinAllocator::~radMemoryBinAllocator
//==============================================================================

radMemoryBinAllocator::~radMemoryBinAllocator( void )
{
    #ifdef RADMEMORY_BIN_ALLOCATOR_DEBUG

        // Check that all of our bins are empty.

        for( unsigned int bin = 0; bin < m_NumberOfBins; bin++ )
        {
            rAssert( m_pBinArray[ bin ].m_NumberAllocated == 0 );
        }

    #endif // RADMEMORY_BIN_ALLOCATOR_DEBUG

    #ifdef RADMEMORYMONITOR

        for( unsigned int b =0; b < m_NumberOfBins; b ++ )
        {
            radMemoryMonitorRescindSection( m_pBinArray[ b ].m_pMemoryStart );
            radMemoryMonitorRescindAllocation( m_pBinArray[ b ].m_pMemoryStart );
        }

    #endif

    radMemoryMonitorRescindSection( m_pBinArray[ 0 ].m_pMemoryStart );

    ::radMemoryFreeAligned( m_pBinArray[ 0 ].m_pMemoryStart );
    ::radMemoryFree( m_pBinArray );
}

//==============================================================================
// radMemoryBinAllocator::Initialize
//
// Description:  This function allocates one large chunk of memory for the
//      heap memory and one chunk for the bin management objects.
//
//      The bin array that the client passes in is copied into our own
//      structures, then is sorted from largest bin size, to smallest bin
//      size.  All bins are aligned to the bin's size, this means that some
//      memory is wasted aligning bins, but if we sort from largest to
//      smallest in memory this wastage is minimize.
//==============================================================================

void radMemoryBinAllocator::Initialize(
        IRadMemoryBinAllocator::Bin * pBinArray,
        unsigned int sizeOfBinArray,
        radMemoryAllocator allocator,
        const char * pName )
{
    rAssert( pBinArray != NULL );
    rAssert( sizeOfBinArray > 0 );
    rAssert( STANDARD_ALIGNMENT >= sizeof ( InternalBin::FreeBlock ) );

    unsigned int bin;     // counter

    m_NumberOfBins = sizeOfBinArray;

    // Allocate memory for our bin management objects.

    m_pBinArray = (InternalBin*) ::radMemoryAlloc( allocator, sizeof( InternalBin ) * m_NumberOfBins );
    rAssert( m_pBinArray != NULL );

    radMemoryMonitorIdentifyAllocation(
        m_pBinArray, g_nameFTech, "Bin Allocator - Bin Objects", NULL, radMemorySpace_Local );

    // Go through and initialize all of our bin objects.

    unsigned int heapMemorySize = 0;

    for( bin = 0; bin < m_NumberOfBins; bin ++ )
    {
        InternalBin * pInternalBin = & ( m_pBinArray[ bin ] );
        Bin         * pBin         = & ( pBinArray[ bin ] );

        rAssert( pBin->m_ElementSizeInBytes > 0 );
        rAssertMsg( pBin->m_NumberOfElements < NULL_INDEX, "Too many elements for bin's free list index type, recompile with larger size type" );
                
        pInternalBin->m_HighWaterMark = 0;
        pInternalBin->m_NumberAllocated = 0;
        pInternalBin->m_pMemoryStart = NULL;
        pInternalBin->m_pMemoryEnd = NULL;
        pInternalBin->m_FreeHeadIndex = NULL_INDEX;
        pInternalBin->m_NumberOfElements =pBin->m_NumberOfElements;

        // Make sure the elements size is greater than a free block and
        // that it is greater than minimum alignment.

        rAssert( pBin->m_ElementSizeInBytes >= STANDARD_ALIGNMENT );

        // Round the size of to minimumElementSize / alignment.

        pInternalBin->m_ElementSizeInBytes =
            ::radMemoryRoundUp( pBin->m_ElementSizeInBytes, STANDARD_ALIGNMENT );

        // Warn client if we are rounding their bins.

        if( pInternalBin->m_ElementSizeInBytes != pBin->m_ElementSizeInBytes )
        {
            rDebugPrintf(
                "radMemoryBinAllocator: Rounding bin size: [%d] to [%d], must be multiple of [%d]\n",
                pBin->m_ElementSizeInBytes,
                pInternalBin->m_ElementSizeInBytes,
                STANDARD_ALIGNMENT );
        }

        // Figure out how big a chunk of memory we need for our bins.

        heapMemorySize +=
            ( pInternalBin->GetPaddedElementSize( ) ) * pBin->m_NumberOfElements;
    }

    qsort( m_pBinArray, m_NumberOfBins, sizeof( InternalBin ), QSortCompareFunction );

    // Allocate the memory for this heap

    char * pMemory = (char*) ::radMemoryGetAllocator( allocator )->GetMemoryAligned(
        heapMemorySize,
        STANDARD_ALIGNMENT );

    rAssert( pMemory != NULL ); // Make sure we have it

    // Tell the memory monitor about the heap
#ifdef RADMEMORYMONITOR

    radMemoryMonitorIdentifyAllocation(
        pMemory, g_nameFTech, "Bin Allocator Heap Memory", NULL, radMemorySpace_Local );

    radMemoryMonitorDeclareSection(
        pMemory, heapMemorySize, IRadMemoryMonitor::MemorySectionType_DynamicData );
    {
    char binName[ 256 ];
    sprintf( binName, "%s[radMemoryBinAllocator]", pName );
    radMemoryMonitorIdentifySection( pMemory, binName );
    }
#endif
    // Set up the free list for all bins.

    for( bin = 0; bin < m_NumberOfBins; bin ++ )
    {
        InternalBin * pBin = & ( m_pBinArray[ bin ] );

        pBin->m_pMemoryStart = pMemory;
        pMemory +=  pBin->GetPaddedElementSize( ) * pBin->m_NumberOfElements;
        pBin->m_pMemoryEnd = pMemory;

        #ifdef RADMEMORYMONITOR // Don't want to comple int binName[]

        // Each bin is a section inside the main memory, tell the memory monitor
        // about this.
        {
        char binName[ 256 ];
        sprintf( binName, "%s[radMemoryBinAllocator]", pName );

        radMemoryMonitorDeclareAllocation(
            pBin->m_pMemoryStart, pBin->m_pMemoryEnd - pBin->m_pMemoryStart );

        radMemoryMonitorIdentifyAllocation(
            pBin->m_pMemoryStart, g_nameFTech, binName, NULL );

        radMemoryMonitorDeclareSection(
            pBin->m_pMemoryStart, pBin->m_pMemoryEnd - pBin->m_pMemoryStart,
            IRadMemoryMonitor::MemorySectionType_DynamicData );

        radMemoryMonitorIdentifySection(
            pBin->m_pMemoryStart, binName );
        }
        #endif // RADMEMORYMONITOR

        rAssert( (((uintptr_t)pBin->m_pMemoryStart) % STANDARD_ALIGNMENT ) == 0 );
                
        pBin->SetUpFreeList( );
    }
}

//==============================================================================
// radMemoryBinAllocator::GetMemory
//
// Description:  This method is called by the outside world directly.
//      Simply search for the correct size bin and ask it for memory.
//==============================================================================

void* radMemoryBinAllocator::GetMemory( unsigned int size )
{
    void * pMemory = GetMemoryUnAligned( size );

    #ifdef RADMEMORYMONITOR

        if ( pMemory != NULL )
        {
            radMemoryMonitorDeclareAllocation( pMemory, size );
        }

    #endif

    return pMemory;    
}

//==============================================================================
// radMemoryBinAllocator::FreeMemory
//
// Description:  This method is called by the outside world directly.
//      Find the bin that allocated the memory and ask it to free it.
//==============================================================================

void radMemoryBinAllocator::FreeMemory( void* pMemory )
{
    // NULL is a valid pointer to free, just do nothing in this case.

    if ( pMemory != NULL )
    {
        InternalBin * pIb = FindBin( pMemory );

        rAssertMsg( pIb != NULL, "radMemoryBinAllocator - This heap did not allocate this memory" );

        pIb->FreeMemory( pMemory );

        radMemoryMonitorRescindAllocation( pMemory );
    }
}

//==============================================================================
// radMemoryBinAllocator::FreeMemory
//
// Description:  This method is called by the outside world directly.
//      Just check to see that we allocated this memory.
//==============================================================================

bool radMemoryBinAllocator::CanFreeMemory( void * pMemory )
{
    // Just quickly check if it is the range of the big chunk of memory that
    // we are managing.  This doesn't actually check that it is on the FreeList.

    return ( pMemory >= m_pBinArray[ 0 ].m_pMemoryStart &&
             pMemory < m_pBinArray[ m_NumberOfBins - 1 ].m_pMemoryEnd );
}

//==============================================================================
// radMemoryBinAllocator::GetMemoryAligned
//
// Description:  This method is called by the outside world directly.
//      We don't "really" support aligned memory we just add the alignment
//      value to the size and align it ourselves.  We adjust it back
//      during free aligned.  The application could use FreeMemory( ) always,
//      instead of FreeMemoryAligned(), but we assert in the former version,
//      that the memory pointer is aligned to a bin's block multiple.
//==============================================================================

void* radMemoryBinAllocator::GetMemoryAligned( unsigned int size, unsigned int alignment )
{
    // Warn that we are wasting a lot of memory

    rAssertMsg( alignment < HUGE_ALIGNMENT_VALUE,
        "Bin heap doesn't handle huge aligments very well" );

    void * pMemory;

    if ( alignment <= STANDARD_ALIGNMENT )
    {
        // Just call the "regular" get memory function--the memory is always
        // aligned to STANDARD_ALIGNMENT.  This way we don't waste any memory.

        pMemory = GetMemoryUnAligned( size );
    }
    else
    {
        //
        // We do the alignment ourselves.
        //

        pMemory = GetMemoryUnAligned( size + alignment );
        pMemory = (void*) ::radMemoryRoundUp( (uintptr_t) pMemory, alignment );
    }

    #ifdef RADMEMORYMONITOR
    
        if ( pMemory != NULL )
        {
            radMemoryMonitorDeclareAllocation( pMemory, size );
        }

    #endif // RADMEMORYMONITOR

    // Check to make sure we really did satisfy the request.

    rAssert( ( alignment == 0 ) || ( ((uintptr_t)pMemory) % alignment == 0 ) );

    return pMemory;
}

//==============================================================================
// radMemoryBinAllocator::FreeMemoryAligned
//
// Description:  This method is called by the outside world directly.
//      The application cannot replace FreeMemoryAligned with FreeMemory or
//      we will assert.
//==============================================================================

void  radMemoryBinAllocator::FreeMemoryAligned( void * pMemory )
{
    // NULL is a valid pointer to free, just do nothing in this case.

    if ( pMemory != NULL )
    {
        InternalBin * pIb = FindBin( pMemory );

        rAssertMsg( pIb != NULL, "radMemoryBinAllocator - This heap did not allocate this memory" );

        pIb->FreeMemoryAligned( pMemory );

        radMemoryMonitorRescindAllocation(
            pMemory );
    }
}

//==============================================================================
// radMemoryBinAllocator::CanFreeMemoryAligned
//
// Description:  This method is called by the outside world directly.
//      There is no difference between the aligned and the unaligned version--
//      just call the unaligned version.
//==============================================================================

bool  radMemoryBinAllocator::CanFreeMemoryAligned( void * pMemory )
{
    return CanFreeMemory( pMemory );
}

//==============================================================================
// radMemoryBinAllocator::GetNumberOfBins
//==============================================================================

unsigned int radMemoryBinAllocator::GetNumberOfBins( void )
{    
    return m_NumberOfBins;
}

//==============================================================================
// radMemoryBinAllocator::GetBinStatus
//==============================================================================

bool radMemoryBinAllocator::GetBinStatus(
        unsigned int binIndex,                 
        IRadMemoryBinAllocator::BinStatus * pBin )
{
    rAssert( pBin != NULL );

    // This is valid, they can iterate through until we return false.

    if ( binIndex >= m_NumberOfBins )
    {
        return false;
    }

    // Just copy our intnal stats to the external structure.

    InternalBin * pInternalBin = & ( m_pBinArray[ binIndex ] );
    pBin->m_ElementSizeInBytes = pInternalBin->m_ElementSizeInBytes;
    pBin->m_NumberOfElements   = pInternalBin->m_NumberOfElements;
    pBin->m_FreeElements       = pInternalBin->m_NumberOfElements - pInternalBin->m_NumberAllocated;
    pBin->m_HighWaterMark      = pInternalBin->m_HighWaterMark;

    return true;
}

//==============================================================================
// radMemoryBinAllocator::Validate
//==============================================================================

void radMemoryBinAllocator::Validate( void )
{
    for( unsigned int bin = 0; bin < m_NumberOfBins; bin ++ )
    {
        m_pBinArray[ bin ].Validate( );
    }
}

//==============================================================================
// radMemoryBinAllocator::QSortCompareFunction
//
// Description: Initially called to sort the client's bin array.  Not that we
//      are sorting from largest to smallest.
//==============================================================================

int radMemoryBinAllocator::QSortCompareFunction( const void * pElement1, const void * pElement2 )
{
    rAssert( pElement1 != NULL );
    rAssert( pElement2 != NULL );

    InternalBin * pBin1 = (InternalBin*) pElement1;
    InternalBin * pBin2 = (InternalBin*) pElement2;

    return (int) pBin2->m_ElementSizeInBytes - (int) pBin1->m_ElementSizeInBytes; 
}

//============================================================================
// ::radMemoryBinAllocatorCreate
//
// Description: Simple factory function.
//============================================================================

void radMemoryBinAllocatorCreate(
    IRadMemoryBinAllocator ** ppIRadMemoryBinAllocator, // "Returns" the Bin Allocator object
    IRadMemoryBinAllocator::Bin * pBinArray,            // Array of Bin Structures (see above)
    unsigned int sizeOfBinArray,                        // Number of Bin Structures
    radMemoryAllocator allocator,
    const char * pName )
{
    rAssert( ppIRadMemoryBinAllocator != NULL );

    radMemoryBinAllocator * pBi = new ( allocator ) radMemoryBinAllocator( );
    radMemoryMonitorIdentifyAllocation(
        pBi, g_nameFTech, "Bin Allocator", NULL, radMemorySpace_Local );
    rAssert( pBi != NULL );
    radAddRef( pBi, ppIRadMemoryBinAllocator );
    pBi->Initialize( pBinArray, sizeOfBinArray, allocator, pName );

    *ppIRadMemoryBinAllocator = pBi;
}
