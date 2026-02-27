//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        dispatcher.cpp
//
// Subsystem:	Foundation Technologies - Dispatcher
//
// Description:	This file contains the implementation of the Foundation 
//              Technologies Dispatcher. The dispatcher is responsible
//              for controlling the flow of low level events within the system.
//
// Date:    	Mar 12, 2001
//
//=============================================================================

//=============================================================================
// Include Files
//=============================================================================

#include "pch.hpp"

#include <raddispatch.hpp>
#include <radobject.hpp>
#include <radmemory.hpp>
#include <raddebug.hpp>
#include <radmemorymonitor.hpp>

#include "dispatcher.hpp"

//=============================================================================
// Local Defintions
//=============================================================================

//=============================================================================
// Public Member Functions
//=============================================================================

//=============================================================================
// Function:    radDispatchCreate
//=============================================================================
// Description: This is the object factory for the dispatcher.
//
// Parameters:  pIRadDispatcher - returns the object.
//              maxCallback,    - max queued callbacks
//              alloc           - where to get memory
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void radDispatchCreate
( 
    IRadDispatcher**   pIRadDispatcher, 
    unsigned int       maxCallbacks,
    radMemoryAllocator alloc
)
{
    //
    // Simply new up a a dispatcher object.
    //
    *pIRadDispatcher = new( alloc ) radDispatcher( maxCallbacks, alloc );
}


//=============================================================================
// Function:    radDispatcher::radDispatcher
//=============================================================================
// Description: Constructor.Nothing to interesting. Just initialize members.
//
// Parameters:  maxcallbacks
//              allocator
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

radDispatcher::radDispatcher
( 
    unsigned int maxCallbacks,
    radMemoryAllocator alloc    
)
    :
    m_ReferenceCount( 1 ),
    m_MaxEvents( maxCallbacks ),
    m_EventQueueHeadIndex( 0 ),
    m_EventQueueTailIndex( 0 ),
    m_EventsQueued( 0 )
{
    radMemoryMonitorIdentifyAllocation( this, g_nameFTech, "radDispatcher" );
    //
    // Allocate memory to use for queing events.
    //
    m_EventQueue = (Event*) radMemoryAlloc( alloc, sizeof(Event) * m_MaxEvents );

    m_Mutex = SDL_CreateMutex();
}

//=============================================================================
// Function:    fDispatcher::~fDispatcher
//=============================================================================
// Description: Destructor. Free any resources.
//
// Parameters:  none
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

radDispatcher::~radDispatcher( void )
{
    //
    // If this asserts the caller did not call purge.
    //
    rAssert( m_EventsQueued == 0 );
   
    SDL_DestroyMutex(m_Mutex);

    //
    // Free up the memory
    //
    radMemoryFree( m_EventQueue );
}

//=============================================================================
// Function:    radDispatcher::AddRef
//=============================================================================
// Description: This member can be invoked to update the reference count of
//              the dispatcher.
//
// Parameters:  none
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void radDispatcher::AddRef( void )
{
    m_ReferenceCount++;
}

//=============================================================================
// Function:    radDispatcher::Release
//=============================================================================
// Description: This member can be invoked to update the reference count of
//              the dispatcher. When reaches zero, dispatcher is deleted.
//
// Parameters:  none
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void radDispatcher::Release( void )
{
    m_ReferenceCount--;
    
    if( m_ReferenceCount == 0 )
    {
       delete this;
    }
}

//=============================================================================
// Function:    radDispatcher::Dump
//=============================================================================
// Description: In a debug build, report the name of this class.
//
// Parameters:  pStringBuffer - where to copy name,
//              bufferSize    - size of buffer
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

#ifdef RAD_DEBUG

void radDispatcher::Dump( char * pStringBuffer, unsigned int bufferSize )
{
    sprintf( pStringBuffer, "Object: [radDispatcher] At Memory Location:[%p]\n", this );
}

#endif

//=============================================================================
// Function:    fDispatcher::QueueCallback
//=============================================================================
// Description: This member is invoked to post an event to the event queue. It
//              can be safely called from another thread.
//
// Parameters:  dispatchEvent - event to add to queue.
//              user data,
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void radDispatcher::QueueCallback
( 
    IRadDispatchCallback* pDispatchCallback,
    void*                 userData 
)
{
    //
    // Update reference count on the dispatch event object since we are holding
    // a pointer to it,
    //      
    pDispatchCallback->AddRef( );

    //
    // Protect the addition of this record to the event list.
    SDL_LockMutex(m_Mutex);

    //
    // Assert that we have not exceeded the maximum number of events in the queue.
    //
    rAssert( m_EventsQueued != m_MaxEvents );                         

    //
    // Add it to the queue at the head.
    //
    m_EventQueue[ m_EventQueueHeadIndex ].m_Callback = pDispatchCallback;
    m_EventQueue[ m_EventQueueHeadIndex ].m_UserData = userData;
    m_EventQueueHeadIndex++;
    if( m_EventQueueHeadIndex == m_MaxEvents )
    {
        m_EventQueueHeadIndex = 0;
    }        
    m_EventsQueued++;

    //
    // Remove protection
    //
    SDL_UnlockMutex(m_Mutex);
}


//=============================================================================
// Function:    radDispatcher::QueueCallbackFromInterrupt
//=============================================================================
// Description: This member is invoked to post an event to the event queue. It
//              can be safely called from an interrupt service routine
//
// Parameters:  dispatchEvent - event to add to queue.
//              user data,
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void radDispatcher::QueueCallbackFromInterrupt
( 
    IRadDispatchCallback* pDispatchCallback,
    void*                 userData 
)
{
    #if defined ( RAD_WIN32 ) || defined( RAD_UWP )
        
    //
    // Not supported under Windows or UWP
    //
    (void) pDispatchCallback;
    (void) userData;
    rAssert( false );

    #endif
}

//=============================================================================
// Function:    radDispatcher::Service
//=============================================================================
// Description: Dispatches an events in the queue when the function is entered.
//
// Parameters:  none
//
// Returns:     number of events queued after exit.
//
// Notes:
//------------------------------------------------------------------------------

unsigned int radDispatcher::Service( void )
{
    //
    // For each invocation, we remove all of the events queued.
    //
    unsigned int eventsToDispatch = m_EventsQueued;

    //
    // We only dispatch as many events that were initially in the queue. Since
    // it is possible for this routine to be invoked in a nested manner, we
    // also make sure there are events in the queue as well.
    //    
    //
    // Protect the manilpulation of this record the event list. Platform specif locks
    // required.
    //
    SDL_LockMutex(m_Mutex);

    while( (m_EventsQueued != 0) && (eventsToDispatch != 0) )
    {
        //
        // We have an event to dispatch. Remove it from the queue and update tail index.
        //
        Event event = m_EventQueue[ m_EventQueueTailIndex ];
        m_EventQueueTailIndex++;
        if( m_EventQueueTailIndex == m_MaxEvents )
        {
            m_EventQueueTailIndex = 0;
        }        
        m_EventsQueued--;
        eventsToDispatch--;

        //
        // Now remove lock and invoke event handler.
        //
        SDL_UnlockMutex(m_Mutex);

        event.m_Callback->OnDispatchCallack( event.m_UserData );

        //
        // Since we are now finished with the event, we can update our reference to the object.
        //
        event.m_Callback->Release( );   // don't call radRelease( ) to report this release to memory monitor

        SDL_LockMutex(m_Mutex);
    }

    SDL_UnlockMutex(m_Mutex);

    return( m_EventsQueued );
          
}