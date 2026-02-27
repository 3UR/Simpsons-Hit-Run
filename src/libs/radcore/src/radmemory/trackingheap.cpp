//-----------------------------------------------------------------------------
// Copyright (C) 2001 Radical Entertainment Ltd.  All rights reserved.
//
// TrackingHeap.hpp
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
#include "platalloc.hpp"
#include "raddebug.hpp"
#include "radmemory.hpp"
#include <radmemorymonitor.hpp>
#include "trackingheap.h"

//-----------------------------------------------------------------------------
// Local Declarations
//-----------------------------------------------------------------------------

//=============================================================================
// TrackingHeap::TrackingHeap
//=============================================================================
// Description: constructor
//
// Parameters:  none
//
// Return:      none
//
//=============================================================================
TrackingHeap::TrackingHeap():
    m_TrackAllocations( true ),
    m_TotalAllocated( 0 ),
    m_MaxSize( 0 ),
    m_HighWater( 0 ),
    m_NumberOfAllocations( 0 )
{
}

//=============================================================================
// TrackingHeap::~TrackingHeap
//=============================================================================
// Description: constructor
//
// Parameters:  none
//
// Return:      none
//
//=============================================================================
TrackingHeap::~TrackingHeap()
{
}

//=============================================================================
// TrackingHeap::AddRef
//=============================================================================
// Description: adds a reference to the reference count
//
// Parameters:  none
//
// Return:      none
//
//=============================================================================
void TrackingHeap::AddRef( void )
{
    radRefCount::Implement_AddRef();
}

//=============================================================================
// TrackingHeap::CanFreeMemory
//=============================================================================
// Description: can the heap free this memory
//
// Parameters:  pMemory - pointer to memory
//
// Return:      bool - true or false - is it contained
//
//=============================================================================
bool TrackingHeap::CanFreeMemory( void* pMemory )
{
    ADDRESS_SIZE_MAP::iterator found = m_Map.find( pMemory );
    if( found == m_Map.end() )
    {
        return false;
    }
    return true;
}

//=============================================================================
// TrackingHeap::CanFreeMemoryAligned
//=============================================================================
// Description: can the heap free this memory
//
// Parameters:  pMemory - pointer to memory
//
// Return:      bool - true or false - is it contained
//
//=============================================================================
bool TrackingHeap::CanFreeMemoryAligned( void* pMemory )
{
    return CanFreeMemory( pMemory );
}

//=============================================================================
// TrackingHeap::FreeMemory
//=============================================================================
// Description: frees memory - not supported in this heap
//
// Parameters:  pMemory - pointer to the memory we're trying to free
//
// Return:      NONE
//
//=============================================================================
void TrackingHeap::FreeMemory( void* pMemory )
{
    ADDRESS_SIZE_MAP::iterator found = m_Map.find( pMemory );
    if( ( found == m_Map.end() ) && m_TrackAllocations )
    {
        rAssert( false );
        return;
    }
    size_t foundSize = ( *found ).second;
    --m_NumberOfAllocations;
    radMemoryPlatFree( pMemory );
    if( m_TrackAllocations )
    {
        m_TotalAllocated -= foundSize;
        m_Map.erase( pMemory );
    }
    ::radMemoryMonitorRescindAllocation( pMemory );
}
//=============================================================================
// TrackingHeap::FreeMemoryAligned
//=============================================================================
// Description: frees aligned memory - not supported in this heap
//
// Parameters:  pMemory - the meomry we're trying to free
//
// Return:      none
//
//=============================================================================
void TrackingHeap::FreeMemoryAligned( void* pMemory )
{
    ADDRESS_SIZE_MAP::iterator found = m_Map.find( pMemory );
    if( ( found == m_Map.end() ) && m_TrackAllocations )
    {
        rAssert( false );
        return;
    }
    size_t foundSize = ( *found ).second;
    --m_NumberOfAllocations;
    radMemoryPlatFreeAligned( pMemory );
    if( m_TrackAllocations )
    {
        m_TotalAllocated -= foundSize;
        m_Map.erase( pMemory );
    }
    ::radMemoryMonitorRescindAllocation( pMemory );
}
//=============================================================================
// TrackingHeap::GetMemory( size_t size )
//=============================================================================
// Description: allocates some memory from the heap
//
// Parameters:  ()
//
// Return:      pointer to some memory
//
//=============================================================================
void* TrackingHeap::GetMemory ( unsigned int size )
{
    void* returnMe = radMemoryPlatAlloc( size );
    if( returnMe != NULL )
    {
        RecordAllocation( returnMe, size );
    }
    return returnMe;
}

//=============================================================================
// TrackingHeap::GetMemoryAligned( size_t size )
//=============================================================================
// Description: allocates some memory from the heap
//
// Parameters:  size - how much memory
//              alignment - what byte alignment do we want
//
// Return:      pointer to some memory
//
//=============================================================================
void* TrackingHeap::GetMemoryAligned( unsigned int size, unsigned int align )
{
    void* returnMe = radMemoryPlatAllocAligned( size, align );
    if( returnMe != NULL )
    {
        RecordAllocation( returnMe, size );
    }
    return returnMe;
}

//=============================================================================
// TrackingHeap::GetMemoryObject
//=============================================================================
// Description: ???
//
// Parameters:  ???
//
// Return:      none
//
//=============================================================================
void TrackingHeap::GetMemoryObject( IRadMemoryObject** ppMemoryObject, unsigned int size )
{
    rAssert( false );
}

//=============================================================================
// TrackingHeap::GetMemoryObjectAligned
//=============================================================================
// Description: ???
//
// Parameters:  ???
//
// Return:      none
//
//=============================================================================
void TrackingHeap::GetMemoryObjectAligned( IRadMemoryObject ** ppIRadMemoryObject, unsigned int size, unsigned int alignment )
{
    rAssert( false );
}

//=============================================================================
// TrackingHeap::GetSize
//=============================================================================
// Description: returns the size of the heap
//
// Parameters:  none
//
// Return:      size of the heap in bytes
//
//=============================================================================
unsigned int TrackingHeap::GetSize()
{
    return m_MaxSize;
}


//=============================================================================
// TrackingHeap::GetStatus
//=============================================================================
// Description: records an allcoation in the map
//
// Parameters:  
//
// Return:      none
//
//=============================================================================
void TrackingHeap::GetStatus(
	unsigned int* totalFreeMemory,
	unsigned int* largestBlock,
	unsigned int* numberOfObjects,
	unsigned int* highWaterMark )
{
    if( totalFreeMemory != NULL )    *totalFreeMemory = m_MaxSize - m_TotalAllocated;
    if( largestBlock    != NULL )    *largestBlock    = 0;
    if( numberOfObjects != NULL )    *numberOfObjects = m_NumberOfAllocations;
    if( highWaterMark   != NULL )    *highWaterMark   = m_HighWater;
}

//=============================================================================
// TrackingHeap::RecordAllocation
//=============================================================================
// Description: records an allcoation in the map
//
// Parameters:  address - the adress of the allocation
//              size - the size of the allocation
//
// Return:      none
//
//=============================================================================
void TrackingHeap::RecordAllocation( void* address, size_t size )
{
    //rAssert( reinterpret_cast< unsigned int >( address ) != 0x024D3320 );
    ++m_NumberOfAllocations;
    if( m_TotalAllocated > m_HighWater )
    {
        m_HighWater = m_TotalAllocated;
    }

    //12 had better be the debug heap!
    radMemoryAllocator old = radMemorySetCurrentAllocator( 12 );
    if( m_TrackAllocations )
    {
        ::radMemoryMonitorDeclareAllocation( address, size );
        m_TotalAllocated += size;
        m_Map[ address ] = size;
        radMemorySetCurrentAllocator( old );
    }
}

//=============================================================================
// TrackingHeap:Release
//=============================================================================
// Description: releases a reference count
//
// Parameters:  none
//
// Return:      none
//
//=============================================================================
void TrackingHeap::Release( void )
{
    radRefCount::Implement_Release();
}

//=============================================================================
// TrackingHeap::SetSize
//=============================================================================
// Description: sets the size of the heap for tracking purposes
//
// Parameters:  size - size of the heap
//
// Return:      none
//
//=============================================================================
void TrackingHeap::SetSize( size_t size )
{
    m_MaxSize = size;
}

//=============================================================================
// TrackingHeap::TrackAllocations
//=============================================================================
// Description: should we bother tracking allocations to this heap?
//
// Parameters:  none
//
// Return:      none
//
//=============================================================================
void TrackingHeap::TrackAllocations( bool trackAllocations )
{
    m_TrackAllocations = trackAllocations;
}

//=============================================================================
// TrackingHeap::ValidateHeap
//=============================================================================
// Description: checks if the heap is valid - doesn nothing right now
//
// Parameters:  none
//
// Return:      none
//
//=============================================================================
bool TrackingHeap::ValidateHeap( void )
{
    return true;
}

//=============================================================================
// radMemoryCreateTrackingHeap
//=============================================================================
// Description: creates a static heap
//
// Parameters:  none
//
// Return:      none
//
//=============================================================================
IRadMemoryHeap* radMemoryCreateTrackingHeap( unsigned int size,
	radMemoryAllocator allocator,
    const char * pName )
{
    TrackingHeap* pHeap = new ( allocator ) TrackingHeap;
    pHeap->SetSize( size );
    return pHeap;
}
