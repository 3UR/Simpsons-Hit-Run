//-----------------------------------------------------------------------------
// Copyright (C) 2001 Radical Entertainment Ltd.  All rights reserved.
//
// staticheap.hpp
//
// Description: A heap with little overhead that cannot be freed
//
// Modification History:
//
//  + Created Mar 21, 2003 Ian Gipson
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Included Files
//-----------------------------------------------------------------------------
#include "pch.hpp"
#include <cstdint>
#include "platalloc.hpp"
#include "raddebug.hpp"
#include "radmemory.hpp"
#include <radmemorymonitor.hpp>
#include "staticheap.h"

//-----------------------------------------------------------------------------
// Local Declarations
//-----------------------------------------------------------------------------

//=============================================================================
// StaticHeap::AddRef
//=============================================================================
// Description: adds a reference to the reference count
//
// Parameters:  none
//
// Return:      none
//
//=============================================================================
void StaticHeap::AddRef( void )
{
    //does nothing
}

//=============================================================================
// StaticHeap::Align
//=============================================================================
// Description: given a size of memory, return the total size that will be 
//              allocated
//
// Parameters:  none
//
// Return:      none
//
//=============================================================================
size_t StaticHeap::Align( const size_t size, size_t alignment )
{
    if( size % alignment == 0 )
    {
        return size;
    }
    else
    {
        int returnMe = ( ( size / alignment ) + 1 ) * alignment;
        return returnMe;
    }
}

//=============================================================================
// StaticHeap::Align
//=============================================================================
// Description: given a size of memory, return the total size that will be 
//              allocated
//
// Parameters:  none
//
// Return:      none
//
//=============================================================================
char* StaticHeap::Align( const char* ptr, const size_t alignment )
{
    size_t sizeVersionOfPtr = reinterpret_cast< const size_t >( ptr );
    size_t fixedSizeVersionOfPtr = Align( sizeVersionOfPtr, alignment );
    return reinterpret_cast< char* >( fixedSizeVersionOfPtr );
}

//=============================================================================
// StaticHeap::AllowFreeing
//=============================================================================
// Description: switches on and off the assert about freeing memory
//
// Parameters:  freeingAllowed - should freeing memory be ok or not
//
// Return:      none
//
//=============================================================================
void StaticHeap::AllowFreeing( const bool freeingAllowed )
{
    m_FreeingAllowed = freeingAllowed;
}

//=============================================================================
// StaticHeap::CanFreeMemory
//=============================================================================
// Description: can the heap free this memory
//
// Parameters:  pMemory - pointer to memory
//
// Return:      bool - true or false - is it contained
//
//=============================================================================
bool StaticHeap::CanFreeMemory( void* pMemory )
{
    return Contains( pMemory );
}

//=============================================================================
// StaticHeap::CanFreeMemoryAligned
//=============================================================================
// Description: can the heap free this memory
//
// Parameters:  pMemory - pointer to memory
//
// Return:      bool - true or false - is it contained
//
//=============================================================================
bool StaticHeap::CanFreeMemoryAligned( void* pMemory )
{
    return Contains( pMemory );
}

//=============================================================================
// StaticHeap::Contains
//=============================================================================
// Description: does the heap contain this memory address
//
// Parameters:  pMemory - pointer to memory
//
// Return:      bool - true or false - is it contained
//
//=============================================================================
bool StaticHeap::Contains( const void* pMemory )
{
    if( pMemory < m_BasePointer )
    {
        return false;
    }
    if( pMemory >= m_End )
    {
        return false;
    }
    return true;
}

//=============================================================================
// StaticHeap::CreateHeap( size_t size )
//=============================================================================
// Description: creates the heap
//
// Parameters:  ()
//
// Return:      none
//
//=============================================================================
void StaticHeap::CreateHeap( size_t size )
{
    m_FreeingAllowed = false;
    m_TotalAllocations = 0;
    m_TotalSize = size;
    m_BasePointer = reinterpret_cast< char* >( radMemoryPlatAlloc( size ) );
    rAssert( m_BasePointer != NULL );
    m_CurrentPointer = m_BasePointer;
    m_End = reinterpret_cast< char* >( reinterpret_cast< size_t >( m_BasePointer ) + size );
    m_Overflow = 0;

    rReleasePrintf("StaticHeap Start: 0x%x, End:0x%x\n", m_BasePointer, m_End );

    #ifdef RADMEMORYMONITOR
    {
        radMemoryMonitorIdentifyAllocation( (void*)m_BasePointer, g_nameFTech, "StaticAllocator::m_StartOfMemory" );
        radMemoryMonitorDeclareSection( (void*)m_BasePointer, m_TotalSize, IRadMemoryMonitor::MemorySectionType_DynamicData );
        char szName[ 128 ];
        sprintf( szName, "[StaticAllocator]" );
        radMemoryMonitorIdentifySection( (void*)m_BasePointer, szName );
    }
    #endif // RADMEMORYMONITOR
}

//=============================================================================
// StaticHeap::FreeMemory
//=============================================================================
// Description: frees memory - not supported in this heap
//
// Parameters:  pMemory - pointer to the memory we're trying to free
//
// Return:      NONE
//
//=============================================================================
void StaticHeap::FreeMemory( void* pMemory )
{
    #ifdef RAD_TUNE
        if( m_FreeingAllowed )
        {
            rTuneWarningMsg( false, "Freeing from static heap!" );    
        }
        else
        {
            rTuneAssertMsg( false, "Freeing from static heap!" );
        }         
    #endif
}

//=============================================================================
// StaticHeap::FreeMemoryAligned
//=============================================================================
// Description: frees aligned memory - not supported in this heap
//
// Parameters:  pMemory - the meomry we're trying to free
//
// Return:      none
//
//=============================================================================
void StaticHeap::FreeMemoryAligned( void* pMemory )
{
    #ifdef RAD_TUNE
        if( m_FreeingAllowed )
        {
            rTuneWarningMsg( false, "Freeing from static heap!" );    
        }
        else
        {
            rTuneAssertMsg( false, "Freeing from static heap!" );
        }        
    #endif
}

//=============================================================================
// StaticHeap::FreeMemorySize
//=============================================================================
// Description: returns the amount of free space in the heap
//
// Parameters:  none
//
// Return:      size_t - the size that is free
//
//=============================================================================
size_t StaticHeap::FreeMemorySize() const
{
    size_t freeSpace = reinterpret_cast< size_t >( m_End ) - 
        reinterpret_cast< size_t >( m_CurrentPointer );
    return freeSpace;
}

//=============================================================================
// StaticHeap::GetMemory( size_t size )
//=============================================================================
// Description: allocates some memory from the heap
//
// Parameters:  ()
//
// Return:      pointer to some memory
//
//=============================================================================
void* StaticHeap::GetMemory ( unsigned int size )
{
    size_t alignment;
    if( size <= 4 )
    {
        alignment = 4;
    }
    else
    {
        alignment = STANDARD_ALIGNMENT;
    }
    size_t numberOfBytes = Align( size, alignment );

    //
    // The current pointer must be aligned to the alignment 
    //
    m_CurrentPointer = Align( m_CurrentPointer, alignment );

    //
    // Start allocating the memory
    //
    void* returnMe = static_cast< void* >( m_CurrentPointer );
    m_CurrentPointer += numberOfBytes;
    if( m_CurrentPointer >= m_End )
    {
        m_Overflow += numberOfBytes;
        m_CurrentPointer -= numberOfBytes;
        PrintOutOfMemoryMessage( size );
        return radMemoryPlatAlloc( size );
    }

    ++m_TotalAllocations;
    static unsigned int memoryToBreakOn = 0x00A8DD80;
    //rTuneAssert( reinterpret_cast< unsigned int >( returnMe ) != memoryToBreakOn );
    ::radMemoryMonitorDeclareAllocation( returnMe, size );
    return returnMe;
}

//=============================================================================
// StaticHeap::GetMemoryAligned( size_t size )
//=============================================================================
// Description: allocates some memory from the heap
//
// Parameters:  size - how much memory
//              alignment - what byte alignment do we want
//
// Return:      pointer to some memory
//
//=============================================================================
void* StaticHeap::GetMemoryAligned( unsigned int size, unsigned int align )
{  
    static unsigned int biggestEverAlignment = align;
    if( align > biggestEverAlignment )
    {
        biggestEverAlignment = align;
    }

    size_t maxAlign;
    if( STANDARD_ALIGNMENT > align )
    {
        maxAlign = STANDARD_ALIGNMENT;
    }
    else
    {
        maxAlign = align;
    }
    //
    // Fix up the current pointer
    //
    m_CurrentPointer = Align( m_CurrentPointer, maxAlign );


    size_t numberOfBytes = Align( size, STANDARD_ALIGNMENT );    
    void* returnMe = static_cast< void* >( m_CurrentPointer );
    m_CurrentPointer += numberOfBytes;
    if( m_CurrentPointer >= m_End )
    {
        m_Overflow += numberOfBytes;
        PrintOutOfMemoryMessage( numberOfBytes );
        return radMemoryPlatAllocAligned( size, align );
    }
    ++m_TotalAllocations;
    static unsigned int memoryToBreakOn = 0x00A8DD80;
    //rTuneAssert( reinterpret_cast< unsigned int >( returnMe ) != memoryToBreakOn );
    ::radMemoryMonitorDeclareAllocation( returnMe, size );
    return returnMe;
}

//=============================================================================
// StaticHeap::GetMemoryObject
//=============================================================================
// Description: ???
//
// Parameters:  ???
//
// Return:      none
//
//=============================================================================
void StaticHeap::GetMemoryObject( IRadMemoryObject** ppMemoryObject, unsigned int size )
{
    rAssert( false );
}

//=============================================================================
// StaticHeap::GetMemoryObjectAligned
//=============================================================================
// Description: ???
//
// Parameters:  ???
//
// Return:      none
//
//=============================================================================
void StaticHeap::GetMemoryObjectAligned( IRadMemoryObject ** ppIRadMemoryObject, unsigned int size, unsigned int alignment )
{
    rAssert( false );
}

//=============================================================================
// StaticHeap::GetSize
//=============================================================================
// Description: returns the size of the heap
//
// Parameters:  NONE
//
// Return:      none
//
//=============================================================================
unsigned int StaticHeap::GetSize( void )
{
    return m_TotalSize;
}

void StaticHeap::GetStatus
(
	unsigned int* totalFreeMemory,
	unsigned int* largestBlock,
	unsigned int* numberOfObjects,
	unsigned int* highWaterMark )
{
    if( totalFreeMemory != NULL )    *totalFreeMemory = FreeMemorySize();
    if( largestBlock    != NULL )    *largestBlock = FreeMemorySize();
    if( numberOfObjects != NULL )    *numberOfObjects = m_TotalAllocations;
    if( highWaterMark   != NULL )    *highWaterMark = GetSize() - FreeMemorySize();
}


//=============================================================================
// StaticHeap::PrintOutOfMemoryMessage
//=============================================================================
// Description: prints out an out of memory error
//
// Parameters:  requested - how much memory we were looking for
//
// Return:      none
//
//=============================================================================
void  StaticHeap::PrintOutOfMemoryMessage( size_t requested )
{
    size_t available = reinterpret_cast< uintptr_t >( m_End ) - reinterpret_cast< uintptr_t >( m_CurrentPointer ); 
    rReleasePrintf( "Static heap full - requested:%d.\tavailable:%d.\toverflow:%d.\n", requested, available, m_Overflow );
    //rReleaseAssert( false );
}

//=============================================================================
// StaticHeap:Release
//=============================================================================
// Description: releases a reference count
//
// Parameters:  none
//
// Return:      none
//
//=============================================================================
void StaticHeap::Release( void )
{
    rAssert( false );
}

//=============================================================================
// StaticHeap::ValidateHeap
//=============================================================================
// Description: checks if the heap is valid - doesn nothing right now
//
// Parameters:  none
//
// Return:      none
//
//=============================================================================
bool StaticHeap::ValidateHeap( void )
{
    rAssert( false );
    return true;
}

//=============================================================================
// radMemoryCreateStaticHeap
//=============================================================================
// Description: creates a static heap
//
// Parameters:  none
//
// Return:      none
//
//=============================================================================
IRadMemoryHeap* radMemoryCreateStaticHeap( unsigned int size,
	radMemoryAllocator allocator,
    const char * pName )
{
    rReleasePrintf("%s ", pName );
    StaticHeap* pHeap = new ( allocator ) StaticHeap;
    pHeap->CreateHeap( size );
    return pHeap;
}
