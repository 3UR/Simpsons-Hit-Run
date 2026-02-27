//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        memoryspacewin32.cpp
//
// Subsystem:	Foundation Technologies - Memory Operation Services
//
// Description:	This file contains the win 32implementation of the 
//              remote memory space operations
//
// Revisions:   13-Oct-2001     Creation     Rob
//
//=============================================================================

#include "pch.hpp"
#include <radmemory.hpp>
#include <string.h>
#include <radmemorymonitor.hpp>

#include "memoryspacewin32.hpp"

//============================================================================
//
// Global Functions
//
//============================================================================

// XBox doesn't have any optimal memory space alignment or multiple requirements

unsigned int radMemorySpace_OptimalMultiple = 4;
unsigned int radMemorySpace_OptimalAlignment = 4;

// Only need one object global object because win32/xbox copies are synchronous.

MemorySpaceAsyncRequest_Copy g_MemorySpaceAsyncRequest_Copy_Placeholder;

void radMemorySpaceInitialize( void )
{
    // Do nothing on this platform
}

void radMemorySpaceTerminate( void )
{
    // Do nothing on this platform
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
    rAssert( pDest != NULL );
    rAssert( spaceDest = radMemorySpace_Main );
    rAssert( pSrc != NULL );
    rAssert( spaceSrc = radMemorySpace_Main );
    rAssert( bytes > 0 );

	::memcpy( pDest, pSrc, bytes ); 

	return & g_MemorySpaceAsyncRequest_Copy_Placeholder;
}

MemorySpaceAsyncRequest_Copy::MemorySpaceAsyncRequest_Copy( void )
    :
    m_RefCount( 0 )
{
    radMemoryMonitorIdentifyAllocation( this, g_nameFTech, "MemorySpaceAsyncRequest_Copy" );
}

MemorySpaceAsyncRequest_Copy::~MemorySpaceAsyncRequest_Copy( void )
{
    rAssert( m_RefCount == 0 ); // The game is not reference counting properly
}

void MemorySpaceAsyncRequest_Copy::AddRef( void )
{
    m_RefCount++;
}

void MemorySpaceAsyncRequest_Copy::Release( void )
{
    rAssert( m_RefCount > 0 );
    m_RefCount--;
}

bool MemorySpaceAsyncRequest_Copy::IsDone( void )
{
    return true;
}

//============================================================================
// radMemoryAlloc
//============================================================================

void * radMemorySpaceAlloc( radMemorySpace space, radMemoryAllocator allocator, unsigned int numBytes )
{
	return ::radMemoryAlloc( allocator, numBytes );
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
	return ::radMemoryAllocAligned( allocator, numBytes, alignment );
}

//============================================================================
// radMemorySpaceGetAllocator
//============================================================================

IRadMemoryAllocator * radMemorySpaceGetAllocator( radMemorySpace memSpace, radMemoryAllocator allocator )
{
    rAssert( memSpace == radMemorySpace_Main );

    return ::radMemoryGetAllocator( allocator );
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
	::radMemoryFree( allocator, pMemory );
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
	::radMemoryFreeAligned( allocator, pMemory );
}




