//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        mutex.cpp
//
// Subsystem:	Radical Threading Services - Mutex Implementation
//
// Description:	This file contains the implementation of the threading services
//              mutex. A mutex provide a way of creating thread mutual exclusion.
//
// Author:		Peter Mielcarski
//
// Revisions:	V1.00	Jan 8, 2002
//
//=============================================================================

//=============================================================================
// Include Files
//=============================================================================

#include "pch.hpp"

#include <radthread.hpp>
#include <radmemorymonitor.hpp>
#include "mutex.hpp"
#include "system.hpp"

//=============================================================================
// Local Definitions
//=============================================================================

//=============================================================================
// Statics
//=============================================================================

//=============================================================================
// Public Functions
//=============================================================================

//=============================================================================
// Function:    radThreadCreateMutex
//=============================================================================
// Description: This is the factory for the mutex object. Invoke this to create
//              a mutual exclusion object.
//
// Parameters:  ppMutex - where to retrun the mutex interface pointer
//              allocator - where to allocate object from.
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

void radThreadCreateMutex
( 
    IRadThreadMutex**   ppMutex,  
    radMemoryAllocator  allocator
)
{
    //
    // Simply new up the object. The object sets its reference count to 1 so
    // we need not add ref it here.
    //
    *ppMutex = new( allocator ) radThreadMutex( );
}

//=============================================================================
// Public Member Functions
//=============================================================================

//=============================================================================
// Function:    radThreadMutex::radThreadMutex
//=============================================================================
// Description: This is the constructor for the mutex object. Create the platform
//              specific primitive.
//
// Parameters:  none
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

radThreadMutex::radThreadMutex( void )
    :
    m_ReferenceCount( 1 )
{ 
    radMemoryMonitorIdentifyAllocation( this, g_nameFTech, "radThreadMutex" );
    m_Mutex = SDL_CreateMutex();
}

//=============================================================================
// Function:    radThreadMutex::~radThreadMutex
//=============================================================================
// Description: This is the destructor for the mutex object. Simply free the
//              OS resource.
//
// Parameters:  none
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

radThreadMutex::~radThreadMutex( void )
{
    SDL_DestroyMutex(m_Mutex);
}

//=============================================================================
// Function:    radThreadMutex::Lock
//=============================================================================
// Description: This is invoked to lock a critcal section of code for execution
//              by only a single thread. If invoked by same thread more than once,
//              ownership is granted.
//
// Parameters:  none
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

void radThreadMutex::Lock( void )
{ 
    SDL_LockMutex(m_Mutex);
}

//=============================================================================
// Function:    radThreadMutex::Unlock
//=============================================================================
// Description: This is invoke to unlock the critical section.
//
// Parameters:  none
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

void radThreadMutex::Unlock( void )
{
    SDL_UnlockMutex(m_Mutex);
}

//=============================================================================
// Function:    radThreadMutex::AddRef
//=============================================================================
// Description: Reference Management.
//
// Parameters:  n/a
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void radThreadMutex::AddRef
(
	void
)
{
    //
    // Protect this operation with mutex as this is not guarenteed to be thread
    // safe.
    //
    radThreadInternalLock( );
	m_ReferenceCount++;
    radThreadInternalUnlock( );
}

//=============================================================================
// Function:    radThreadMutex::Release
//=============================================================================
// Description: Reference Management.
//
// Parameters:  n/a
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void radThreadMutex::Release
(
	void
)
{
    //
    // Protect this operation with mutex as this is not guarenteed to be thread
    // safe.
    //
    radThreadInternalLock( );

	m_ReferenceCount--;

	if ( m_ReferenceCount == 0 )
	{
        radThreadInternalUnlock( );
		delete this;
	}
    else
    {
        radThreadInternalUnlock( );
    }
}

//=============================================================================
// Function:    radThreadMutex::Dump
//=============================================================================
// Description: This member is used to display object info
//
// Parameters:  string buffer and size of buffer
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

#ifdef RAD_DEBUG

void radThreadMutex::Dump( char* pStringBuffer, unsigned int bufferSize )
{
    sprintf( pStringBuffer, "Object: [radThreadMutex] At Memory Location:[%p]\n", this );
}

#endif


