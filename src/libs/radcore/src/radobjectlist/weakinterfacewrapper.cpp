//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        weakinterfacewrapper.cpp
//
// Subsystem:	Foundation Technologies - interface wrapper
//
// Description:	This file contains the implementation of the interface wrapper
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
#include <radmemorymonitor.hpp>
#include <radobject.hpp>
#include <radobjectlist.hpp>
#include <raddebug.hpp>

//============================================================================
// Component: radWeakInterfaceWrapper
//============================================================================

struct radWeakInterfaceWrapper
	:
	public IRadWeakInterfaceWrapper,
    public radRefCount
{
    IMPLEMENT_REFCOUNTED( "radWeakInterfaceWrapper" )

    //========================================================================
    // bWeakInterfaceWrapper::SetWeakInterface
    //========================================================================

	virtual void SetWeakInterface( void * pWeakInterface )
	{
		m_pWeakInterface = pWeakInterface;
	}

    //========================================================================
    // bWeakInterfaceWrapper::GetWeakInterface
    //========================================================================

    virtual void * GetWeakInterface( void )
	{
		return m_pWeakInterface;
	}

    //========================================================================
    // bWeakInterfaceWrapper::bWeakInterfaceWrapper
    //========================================================================

	radWeakInterfaceWrapper( void )
		:
		m_pWeakInterface( NULL )
	{
        radMemoryMonitorIdentifyAllocation( this, g_nameFTech, "radWeakInterfaceWrapper" );
	}

	void * m_pWeakInterface;
};

void radWeakInterfaceWrapperCreate( IRadWeakInterfaceWrapper** ppWI , radMemoryAllocator allocator )
{
    *ppWI = new ( allocator ) radWeakInterfaceWrapper( );
}


