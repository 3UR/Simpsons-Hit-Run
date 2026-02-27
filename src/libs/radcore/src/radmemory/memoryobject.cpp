//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        memoryobject.cpp
//
// Subsystem:	Foundation Technologies - Memory
//
// Description:	This file contains the implementation of the Foundation 
//              Technologies memory object access.
//
// Date:    	May 02, 2000
//
// Author:      Donnie Gossett
//
//=============================================================================

//=============================================================================
// Include Files
//=============================================================================

#include "pch.hpp"
#include <stdlib.h>
#include <raddebug.hpp>
#include <radobject.hpp>
#include <radmemory.hpp>
#include <radmemorymonitor.hpp>
#include "memory.hpp"

//=============================================================================
// Local Defintions
//=============================================================================

//=============================================================================
// Static Data Defintions
//=============================================================================

//=============================================================================
// Public Member Functions
//=============================================================================

//=============================================================================
// Function:    MemoryObject::MemoryObject
//=============================================================================
// Description: Constructor for MemoryObject object.
//
// Parameters:  size			size of memory object's usable memory block not
//								including memory object itself.
//				memoryAddress	address of memoby block
//              pMemoryHeap		pointer to parent heap
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

MemoryObject::MemoryObject
(
	unsigned int    size,
	void*           memoryAddress,
	IRadMemoryHeap* pMemoryHeap
)
    :
		m_Size( size ),
		m_MemoryAddress( memoryAddress ),
		m_MemoryHeap( pMemoryHeap ),
        m_ReferenceCount( 1 )                  // Initial reference count
{
    radMemoryMonitorIdentifyAllocation( this, g_nameFTech, "MemoryObject" );
	rAssert( size > 0 );
	rAssert( memoryAddress != NULL );
	rAssert( pMemoryHeap != NULL );
}

//=============================================================================
// Function:    MemoryObject::GetMemoryAddress
//=============================================================================
// Description: This member is used to retrieve the actual address of memory 
//				that is backed by this object. The address returned is just a 
//				pointer not an interface. Do not attempt to delete the object 
//				using this address. Not that this memory will not fail as no 
//				alloction is performed.
//
// Parameters:  none.
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void* MemoryObject::GetMemoryAddress
( 
	void 
)
{
	return( m_MemoryAddress );
}
    
//=============================================================================
// Function:    MemoryObject::GetMemorySize
//=============================================================================
// Description: Use this member to get the size of the memory represented by 
//				this object.
//
// Parameters:  none.
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

unsigned int MemoryObject::GetMemorySize
( 
	void 
)
{
	return( m_Size );
}

//=============================================================================
// Function:    MemoryObject::AddRef
//=============================================================================
// Description: Invoke this member anytime another object is given a pointer to 
//				this object. Simply updates the reference count.
//
//
// Parameters:  none.
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void MemoryObject::AddRef
( 
	void 
)
{
    m_ReferenceCount++;
}

//=============================================================================
// Function:    MemoryObject::~MemoryObject
//=============================================================================
// Description: Destructor for the memoryobject object.
//
// Parameters:  none.
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

MemoryObject::~MemoryObject
(
)
{
	rAssert( m_ReferenceCount == 0 );
}

//=============================================================================
// Function:    MemoryObject::Release
//=============================================================================
// Description: This member should be used instead of delete. It will ensure 
//				correct management of the objects lifetime.
//
// Parameters:  none.
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void MemoryObject::Release
( 
	void 
)   
{
    // Decrement the reference count. If zero, delete the object.
	rAssert( m_ReferenceCount != 0 );
    m_ReferenceCount--;
        
    if( m_ReferenceCount == 0 )
    {
		// Save pointer to parent heap
		IRadMemoryHeap* memoryHeap = m_MemoryHeap;

		// Destruct this object
		this->~MemoryObject( );

		// Free memory associated with this object
		memoryHeap->FreeMemory( this );
    }
}

