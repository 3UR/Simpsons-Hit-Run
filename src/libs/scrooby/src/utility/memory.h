//===========================================================================
// Copyright (C) 2001 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   Scrooby Memory Functions
//
// Description: None
//
// Authors:     Ian Gipson
//
//===========================================================================

// Recompilation protection flag.
#ifndef __SCROOBYMEMORY_H
#define __SCROOBYMEMORY_H

//===========================================================================
// Nested Includes
//===========================================================================
#ifdef _WINDOWS
#include <crtdbg.h>
#endif

#ifndef SCROOBY_TOOL
#include <p3d/memory.hpp>
#else
namespace p3d
{
    typedef unsigned int AllocType;
    const AllocType ALLOC_TEMPORARY = 0;

    void* MallocTemp( unsigned int size );
    void FreeTemp( void* mem );
    AllocType SetCurrentAlloc( AllocType a );
    AllocType GetCurrentAlloc();
}
#endif


#endif                                  // End conditional inclusion