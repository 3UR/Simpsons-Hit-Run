//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        weakcallbackwrapper.cpp
//
// Subsystem:	Foundation Technologies - callback wrapper
//
// Description:	This file contains the implementation of the callback wrapper
//
// Revisions:   Mar 14, 2000     Creation     TEH
//
//=============================================================================

//============================================================================
// Include Files
//============================================================================

#include "pch.hpp"
#include <stdlib.h>
#include <radmemory.hpp>
#include <radobject.hpp>
#include <radobjectlist.hpp>
#include <raddebug.hpp>
#include <radmemorymonitor.hpp>

//============================================================================
// Component: radWeakCallbackWrapper
//============================================================================

struct radWeakCallbackWrapper
	:
	public IRadWeakCallbackWrapper,
	public radRefCount

{

	IMPLEMENT_REFCOUNTED( "radWeakCallbackWrapper" )

    //========================================================================
    // bWeakCallbackWrapper::SetWeakCallback
    //========================================================================

	virtual void SetWeakInterface( void * pWeakCallback )
	{
		m_pWeakCallback = pWeakCallback;
	}

    //========================================================================
    // bWeakCallbackWrapper::GetWeakCallback
    //========================================================================

    virtual void * GetWeakInterface( void )
	{
		return m_pWeakCallback;
	}

    //========================================================================
    // bWeakCallbackWrapper::SetUserData
    //========================================================================

    virtual void SetUserData( void * pUserData )
    {
        m_UserData = pUserData;
    }

    //========================================================================
    // bWeakCallbackWrapper::GetUserData
    //========================================================================

    virtual void * GetUserData( void )
    {
        return m_UserData;
    }

    //========================================================================
    // bWeakCallbackWrapper::bWeakCallbackWrapper
    //========================================================================

	radWeakCallbackWrapper( void )
		:
		radRefCount( 1 ),
		m_pWeakCallback( NULL ),
        m_UserData( NULL )
	{
        radMemoryMonitorIdentifyAllocation( this, g_nameFTech, "radWeakCallbackWrapper" );
    }

	unsigned int m_RefCount;

	void * m_pWeakCallback;
    void * m_UserData;


};

void radWeakCallbackWrapperCreate( IRadWeakCallbackWrapper** ppIWcr, radMemoryAllocator allocator )
{
    *ppIWcr = new ( allocator ) radWeakCallbackWrapper( );
}


//=============================================================================
// Public Functions
//=============================================================================

//=============================================================================
// Function:    radWeakCallbackRegister
//=============================================================================
// Description: Register a callback
//
// Paramters:   pCallbackList - a list of possible callbacks
//              allocator - the allocator to use when creating the
//                          weak callback wrapper
//              pCallback - a void* to a callback
//              pUserData - a void* to some user data
//
// Returns:     n/a
//
//------------------------------------------------------------------------------

void radWeakCallbackRegister
(
    IRadObjectList* pCallbackList,
    radMemoryAllocator allocator,
    void* pCallback,
    void* pUserData
)
{
    radRef< IRadWeakCallbackWrapper > xIWcw;
    ::radWeakCallbackWrapperCreate( & xIWcw, allocator );
    
    xIWcw->SetWeakInterface( (void*) pCallback );
    xIWcw->SetUserData( pUserData );
    
    pCallbackList->AddObject( xIWcw );
}

//=============================================================================
// Function:    radWeakCallbackUnregister
//=============================================================================
// Description: Unregister a callback.  Both the callback and the user data
//              must match.
//
// Paramters:   pCallbackList - a list of possible callbacks
//              pCallback - a void* to a callback
//              pUserData - a void* to some user data
//
// Returns:     n/a
//
//------------------------------------------------------------------------------

void radWeakCallbackUnregister
(
    IRadObjectList* pCallbackList,
    void* pCallback,
    void* pUserData
)
{
    radRef< IRadWeakCallbackWrapper > xIWcw;
    
    for
    (
        unsigned int i = 0;
        i < pCallbackList->GetSize( );
        i++
    )
    {
        xIWcw = reinterpret_cast< IRadWeakCallbackWrapper * >
        (
            pCallbackList->GetAt( i )
        );
        
        if
        (
            xIWcw->GetWeakInterface( ) == pCallback &&
            xIWcw->GetUserData( ) == pUserData
        )
        {
            pCallbackList->RemoveObject( xIWcw );
            return;
        }
        
        xIWcw = NULL;
    }
    
    rAssert( 0 ); // not found
}
