//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        twowayallocator.cpp
//
// Subsystem:	Foundation Technologies - Two Way Allocator
//
// Description:	This file contains the implementation of the two way allocator.
//              The two way allocator allocates memory from one side of 
//              memory to the other.  The allocator must be reset to allocate
//              from the the beginning again.
//
// Revisions:   June 27, 2002   Creation     RWS
//
//=============================================================================

//=============================================================================
// Include Files
//=============================================================================

#include "pch.hpp"
#include <radmemorymonitor.hpp>
#include "twowayallocator.hpp"

//=============================================================================
// radMemoryTwoWayAllocator::radMemoryTwoWayAllocator
//=============================================================================

radMemoryTwoWayAllocator::radMemoryTwoWayAllocator( void )
    :
    radRefCount( 0 ),
    m_pTop( NULL ),
    m_pBottom( NULL ),
    m_pCurrentTop( NULL ),
    m_pCurrentBottom( NULL ),
    m_NumTopAllocations( 0 ),
    m_NumBottomAllocations( 0 ),
    m_Direction( IRadMemoryTwoWayAllocator::TopDown )
{
    radMemoryMonitorIdentifyAllocation( this, g_nameFTech, "radMemoryTwoWayAllocator" );    
}

//=============================================================================
// radMemoryTwoWayAllocator::~radMemoryTwoWayAllocator
//=============================================================================

radMemoryTwoWayAllocator::~radMemoryTwoWayAllocator( void )
{
    rAssert( m_NumTopAllocations == 0 && m_NumBottomAllocations == 0 );

    ::radMemoryFree( m_pTop );   
    m_pTop = NULL;
    m_pBottom = NULL;
    m_pCurrentTop = NULL;
    m_pCurrentBottom = NULL;
}

//=============================================================================
// radMemoryTwoWayAllocator::Initialize
//=============================================================================

void radMemoryTwoWayAllocator::Initialize( unsigned int size )
{
    rAssert( m_pTop == NULL );

    // Allocate some memory to work with

    m_pTop = ( char * ) ::radMemoryAlloc( GetThisAllocator( ), size );

    // Identify the allocation

    radMemoryMonitorIdentifyAllocation(
        m_pTop, g_nameFTech, "radMemoryTwoWayAllocator footprint", NULL, radMemorySpace_Local );

    // Initialize members

    m_pBottom = m_pTop + size;
    m_pCurrentTop = m_pTop;
    m_pCurrentBottom = m_pBottom;
}

//=============================================================================
// radMemoryTwoWayAllocator::GetMemory
//=============================================================================

void * radMemoryTwoWayAllocator::GetMemory( unsigned int size )
{
    return InternalAllocate( size, STANDARD_ALIGNMENT );
}

//=============================================================================
// radMemoryTwoWayAllocator::FreeMemory
//=============================================================================

void radMemoryTwoWayAllocator::FreeMemory( void * pMemory )
{
    InternalFree( pMemory );
}

//=============================================================================
// radMemoryTwoWayAllocator::CanFreeMemory 
//=============================================================================

bool radMemoryTwoWayAllocator::CanFreeMemory( void * pMemory )
{
    return InternalCanFree( pMemory ); 
}

//=============================================================================
// radMemoryTwoWayAllocator::GetMemoryAligned
//=============================================================================

void * radMemoryTwoWayAllocator::GetMemoryAligned( unsigned int size, unsigned int alignment )
{
    return InternalAllocate( size, ( alignment > STANDARD_ALIGNMENT ) ? alignment : STANDARD_ALIGNMENT );
}

//=============================================================================
// radMemoryTwoWayAllocator::FreeMemoryAligned 
//=============================================================================

void radMemoryTwoWayAllocator::FreeMemoryAligned( void * pMemory )
{
    InternalFree( pMemory );
}

//=============================================================================
// radMemoryTwoWayAllocator::CanFreeMemoryAligned
//=============================================================================

bool radMemoryTwoWayAllocator::CanFreeMemoryAligned( void * pMemory )
{
    return InternalCanFree( pMemory );
}

//=============================================================================
// radMemoryTwoWayAllocator::SetDirection
//=============================================================================

void radMemoryTwoWayAllocator::SetDirection( Direction direction )
{
    m_Direction = direction;
}

//=============================================================================
// radMemoryTwoWayAllocator::GetDirection
//=============================================================================

IRadMemoryTwoWayAllocator::Direction radMemoryTwoWayAllocator::GetDirection( void )
{
    return m_Direction;
}

//=============================================================================
// radMemoryTwoWayAllocator::Reset
//=============================================================================

void radMemoryTwoWayAllocator::Reset( Direction direction )
{
    if( direction == TopDown )
    {   
        rAssert( m_NumTopAllocations == 0 );

        m_pCurrentTop = m_pTop;
        m_NumTopAllocations = 0;
    }
    else
    {
        rAssert( m_NumBottomAllocations == 0 );

        m_pCurrentBottom = m_pBottom;
        m_NumBottomAllocations = 0;
    }
}

//=============================================================================
// ::radMemoryTwoWayAllocatorCreate
//=============================================================================

void radMemoryTwoWayAllocatorCreate
( 
    IRadMemoryTwoWayAllocator** ppTwoWayAllocator, 
    unsigned int size,
    radMemoryAllocator allocator,
    const char * pName
)
{
    radMemoryTwoWayAllocator * pRadMemoryTwoWayAllocator = new( allocator )radMemoryTwoWayAllocator( );
    rAssert( pRadMemoryTwoWayAllocator != NULL );

    radMemoryMonitorIdentifyAllocation(
        pRadMemoryTwoWayAllocator, g_nameFTech, "radMemoryTwoWayAllocator", NULL, radMemorySpace_Local );
    radAddRef( pRadMemoryTwoWayAllocator, ppTwoWayAllocator );

    pRadMemoryTwoWayAllocator->Initialize( size );

    * ppTwoWayAllocator = pRadMemoryTwoWayAllocator;
}
