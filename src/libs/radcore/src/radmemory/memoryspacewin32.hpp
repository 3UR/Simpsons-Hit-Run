//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        memoryspacewin32.hpp
//
// Subsystem:	Foundation Technologies - Memory Operation Services
//
// Description:	This file contains the implementation of the win32
//              remote memory space operations
//
// Revisions:   13-Oct-2001     Creation     Rob
//
//=============================================================================

#ifndef MEMORYSPACEWIN32_HPP
#define MEMORYSPACEWIN32_HPP

//============================================================================
// Include Files
//============================================================================

#include <radmemory.hpp>

//============================================================================
// struct MemorySpaceAsyncRequest_Copy
//============================================================================

// This is just a placeholder object that always returns true when asked if it
// is done all copies are synchronous on WIN32.  There is only one of these
// objects in the system.

struct MemorySpaceAsyncRequest_Copy
	:
	public IRadMemorySpaceCopyRequest
{
    MemorySpaceAsyncRequest_Copy( void );
    ~MemorySpaceAsyncRequest_Copy( void );

    virtual void AddRef( void );
    virtual void Release( void );
	virtual bool IsDone( void );

    unsigned int m_RefCount;
};

#endif // MEMORYSPACEWIN32_HPP