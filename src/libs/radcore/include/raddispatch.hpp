//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        raddispatch.hpp
//
// Subsystem:	Foundation Technologies - Dispatcher
//
// Description:	This file contains all definitions and interfaces required to
//              interact with the Dispatcher. The dispatcher provides a mechanism
//              for queuing a function for deffered execution. The dispather
//              services are thread safe and can be used accross thread and
//              interrupt boundaries.
//
//
// Revisions:   Mar 4, 2001        Creation
//
//=============================================================================

#ifndef	RADDISPATCH_HPP
#define RADDISPATCH_HPP

//=============================================================================
// Include Files
//=============================================================================

#include <radobject.hpp>                     
#include <radmemory.hpp>                     

//=============================================================================
// Forward Declarations
//=============================================================================

struct IRadDispatcher;
struct IRadDispatchCallback;

//=============================================================================
// Functions
//=============================================================================

//
// Use this function to create a dispatcher object. Provided an inidcation as
// to the maximum dispatch callbacks that can be queued at any one time.
//
void radDispatchCreate( IRadDispatcher** pIRadDispatcher, unsigned int maxCallbacks = 128,
                        radMemoryAllocator alloc = RADMEMORY_ALLOC_DEFAULT );


//=============================================================================
// Interface Declarations
//=============================================================================

// 
// This is the Dispatcher interface. It is responsible for scheduling backround events
// into the thread context of the calling application. Normally a single
// dispatch object will be contructed, however more than one can be used.
//
struct IRadDispatcher : public IRefCount
{
    //
    // Use this member to drive the dispatch of callbacks. Each invocation
    // of this member will invoked all callbacks queued at the invocation
    // of the function. The routine will return the number of events remaining
    // in the queue. Can be used to purge all.
    //
    virtual unsigned int Service( void ) = 0; 

    //
    // Use this member of queue a dispatch callback for deffered exectuion.
    //
    virtual void QueueCallback( IRadDispatchCallback* pDispatchCallback, void* userData ) = 0;

    //
    // This member also queues an event. However, it supports queing from
    // an interrupt context. It will not modify the state of the interrupt mask. Need due
    // to limitations of some OSs.
    //
    virtual void QueueCallbackFromInterrupt( IRadDispatchCallback* pDispatchCallback, void* userData ) = 0;
};

//
// This is the inteface to a dispatch callack. Implement to received deffered function invocation. 
//
struct IRadDispatchCallback : public IRefCount
{
    //
    // Implement handler to field event.
    //
    virtual void OnDispatchCallack( void* userData ) = 0;
};

#endif