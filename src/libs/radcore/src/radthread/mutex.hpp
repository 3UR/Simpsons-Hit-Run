//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        mutex.hpp
//
// Subsystem:	Radical Threading Services - Mutex 
//
// Description:	This file contains all definitions and classes relevant to
//              the implementation of the radical threading services mutex
//
// Revisions:	January 7, 2002  PGM    Creation
//
// Notes:       
//=============================================================================

#ifndef	MUTEX_HPP
#define MUTEX_HPP

//=============================================================================
// Include Files
//=============================================================================

#include <SDL2/SDL.h>

#include <radobject.hpp>
#include <radmemory.hpp>
#include <radthread.hpp>

//=============================================================================
// Forward Class Declarations
//=============================================================================

//=============================================================================
// Defintions
//=============================================================================

//=============================================================================
// Class Declarations
//=============================================================================

//
// This class derives from the mutex interface. It has platform specific 
// implementations.
//
class radThreadMutex : public IRadThreadMutex,
                       public radObject
{
    public:

    //
    // Constructor, destructor. Nothing interesting
    //
    radThreadMutex( void );
    ~radThreadMutex( void );

    //
    // Members of the IRadThreadMutex
    //
    virtual void Lock( void );
    virtual void Unlock( void );

    //
    // Members of IRefCount
    //
    virtual void AddRef( void );
    virtual void Release( void );

    //
    // Used for tracking active objects.
    //
    #ifdef RAD_DEBUG
    virtual void Dump( char* pStringBuffer, unsigned int bufferSize );
    #endif

    private:

    //
    // This member maintains the reference count of this object.
    //
    unsigned int m_ReferenceCount;    

    SDL_mutex* m_Mutex;
};

#endif