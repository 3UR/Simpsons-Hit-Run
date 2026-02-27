//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        externalmemoryobject.cpp
//
// Subsystem:	Foundation Technologies - ExternalMemory
//
// Description:	This file contains the implementation of the Foundation 
//              Technologies external memory component.
//
// Date:    	June 20, 2000
//
// Author:      Tim Hinds
//
//=============================================================================

//============================================================================
// Class: Include Files
//============================================================================

#include "pch.hpp"
#include <stdlib.h>
#include "externalmemoryobject.hpp"
#include "externalmemoryheap.hpp"

//============================================================================
// ExternalMemoryObject::operator delete
//============================================================================

//============================================================================
// ExternalMemoryObject::AddRef
//============================================================================

/* virtual */ void ExternalMemoryObject::AddRef
(
	void
)
{
    m_ReferenceCount++;
}

//============================================================================
// ExternalMemoryObject::Release
//============================================================================

/* virtual */ void ExternalMemoryObject::Release( void )
{
    rAssert( m_ReferenceCount > 0 );
  
    if ( m_ReferenceCount == 1 )
    {                
        m_pExternalMemoryHeap->Free( this );
    }
    else
    {
        m_ReferenceCount--;
    }
}

//============================================================================
// ExternalMemoryObject::GetMemoryAddress
//============================================================================

/* virtual */ void* ExternalMemoryObject::GetMemoryAddress
(
	void
)
{
    return (void*) m_ClientAddress;
}

//============================================================================
// ExternalMemoryObject::GetMemorySize
//============================================================================

/* virtual */ unsigned int ExternalMemoryObject::GetMemorySize( void )
{
    return m_ClientSize;
}


