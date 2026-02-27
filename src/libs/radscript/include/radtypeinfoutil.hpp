//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File: radtypeinfoutil.hpp
//
// Subsystem: RadScript.hpp
//	
// Description: Simple utility class and functions for working with run time
//      type information
//
//=============================================================================

//============================================================================
// Include Files
//============================================================================

#ifndef RADTYPEINFOUTIL_HPP
#define RADTYPEINFOUTIL_HPP

#include <radobject.hpp>
#include "radtypeinfo.hpp"

//============================================================================
// Forward Declarations
//============================================================================

struct IRadTypeInfoDistributor;
struct IRadTypeInfoInstanceMethod;

//============================================================================
// Factories & Functions
//============================================================================

IRadTypeInfoDistributor *    radTypeInfoCreateDistributor( radMemoryAllocator allocator = RADMEMORY_ALLOC_DEFAULT );
IRadTypeInfoInstanceMethod * radTypeInfoCreateInstanceMethod( IRefCount * pIRefCount, const char * pMethodName, unsigned int allocator = RADMEMORY_ALLOC_DEFAULT );

// Helpers

void radTypeInfoLoadTypeInfoSync( const char * pTypeInfo );

//============================================================================
// Interface: IRadTypeInfoDistributor
//============================================================================

struct IRadTypeInfoDistributor : public IRefCount
{
    virtual void AddObject( IRefCount * pIRefCount, const char * pMethodName ) = 0;
    virtual void RemoveObject( IRefCount * pIRefCount, const char * pMethodName ) = 0;

    virtual void Invoke( void * pParams, unsigned int numParams ) = 0;
};

//============================================================================
// Interface: IRadTypeInfoInstanceMethod
//============================================================================

struct IRadTypeInfoInstanceMethod : public IRefCount
{    
    virtual int Invoke( void * pParams, unsigned int numParams ) = 0;

    virtual IRefCount * GetObject( void ) = 0;

	virtual const char * GetMethodName( void ) = 0;

    virtual unsigned int GetMethodHashedName( void ) = 0;
};

#endif