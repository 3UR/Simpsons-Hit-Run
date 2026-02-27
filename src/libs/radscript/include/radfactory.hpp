//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File: radfactory.hpp
//
// Subsystem: RadScript
//
// Description:	Allows creation of objects based on thier class name, or a
//      key (hash) version of that class name.  (This system stores keys
//      internally).
//
//=============================================================================

#ifndef RADFACTORY_HPP
#define RADFACTORY_HPP

//============================================================================
// Include Files
//============================================================================

#include <radobject.hpp>
#include <radmemory.hpp>
#include "radkey.hpp"

//============================================================================
// Types
//============================================================================

typedef IRefCount * (radFactoryProc)( radMemoryAllocator allocator );

//============================================================================
// Functions
//============================================================================

void radFactoryInitialize( radMemoryAllocator allocator );
void radFactoryTerminate( void );

void radFactoryRegister( const char * pClassName, radFactoryProc );
void radFactoryRegister( radKey32 key, radFactoryProc );

void radFactoryCreateInstance( const char * pClassName, IRefCount ** ppIRefCount, radMemoryAllocator = RADMEMORY_ALLOC_DEFAULT );
void radFactoryCreateInstance( radKey32 key, IRefCount ** ppIRefCount, radMemoryAllocator allocator = RADMEMORY_ALLOC_DEFAULT );

#endif