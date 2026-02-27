//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        semaphore.cpp
//
// Subsystem:	Radical Threading Services - Semaphore Implementation
//
// Description:	This file contains the implementation of the threading services
//              semaphore. A semaphore is an object which can be used to coordinate
//              and sychronize threads
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
#include "semaphore.hpp"
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
// Function:    radThreadCreateSemaphore
//=============================================================================
// Description: This is the factory for the semaphore object. Invoke this to create
//              a semaphore object.
//
// Parameters:  ppSemaphore - where to return the semaphore interface pointer
//              count - initial semaphore count.
//              allocator - where to allocate object from.
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

void radThreadCreateSemaphore
( 
    IRadThreadSemaphore** ppSemaphore,
    unsigned int count,
    radMemoryAllocator allocator
)
{
    //
    // Simply new up the object. The object sets its reference count to 1 so
    // we need not add ref it here.
    //
    *ppSemaphore = new( allocator ) radThreadSemaphore( count );
}

//=============================================================================
// Public Member Functions
//=============================================================================

//=============================================================================
// Function:    radThreadSemaphore::radThreadSemaphore
//=============================================================================
// Description: This is the constructor for the semaphore object. Create the platform
//              specific primitive.
//
// Parameters:  count - initail semaphore count
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

radThreadSemaphore::radThreadSemaphore( unsigned int count )
    :
    m_ReferenceCount( 1 )
{ 
    radMemoryMonitorIdentifyAllocation( this, g_nameFTech, "radThreadSemaphore" );
    m_Semaphore = SDL_CreateSemaphore(0);
}

//=============================================================================
// Function:    radThreadSemaphore::~radThreadSemaphore
//=============================================================================
// Description: This is the destructor for the semaphore object. Simply free the
//              OS resource.
//
// Parameters:  none
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

radThreadSemaphore::~radThreadSemaphore( void )
{
    SDL_DestroySemaphore(m_Semaphore);
}

//=============================================================================
// Function:    radThreadSemaphore::Wait
//=============================================================================
// Description: This is invoked to wait on the semaphore. The thread is
//              suspended if the count goes to zero.
//
// Parameters:  none
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

void radThreadSemaphore::Wait( void )
{ 
    SDL_SemWait(m_Semaphore);
}

//=============================================================================
// Function:    radThreadSemaphore::Signal
//=============================================================================
// Description: This is invoke to signal the semaphore.
//
// Parameters:  none
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

void radThreadSemaphore::Signal( void )
{
    SDL_SemPost(m_Semaphore);
}

//=============================================================================
// Function:    radThreadSemaphore::AddRef
//=============================================================================
// Description: Reference Management.
//
// Parameters:  n/a
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void radThreadSemaphore::AddRef
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
// Function:    radThreadSemaphore::Release
//=============================================================================
// Description: Reference Management.
//
// Parameters:  n/a
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void radThreadSemaphore::Release
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
// Function:    radThreadSemaphore::Dump
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

void radThreadSemaphore::Dump( char* pStringBuffer, unsigned int bufferSize )
{
    sprintf( pStringBuffer, "Object: [radThreadSemaphore] At Memory Location:[%p]\n", this );
}

#endif


