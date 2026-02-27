//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        semaphore.hpp
//
// Subsystem:	Radical Threading Services - Semaphore
//
// Description:	This file contains all definitions and classes relevant to
//              the implementation of the radical threading services semaphore
//
// Revisions:	January 7, 2002  PGM    Creation
//
// Notes:       
//=============================================================================

#ifndef	SEMAPHORE_HPP
#define SEMAPHORE_HPP

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
// This class derives from the semaphore interface. It has platform specific 
// implementations.
//
class radThreadSemaphore : public IRadThreadSemaphore,
                           public radObject
{
    public:

    //
    // Constructor, destructor. Constructor gets initial semaphore count
    //
    radThreadSemaphore( unsigned int count );
    ~radThreadSemaphore( void );

    //
    // Members of the IRadThreadSemaphore
    //
    virtual void Wait( void );
    virtual void Signal( void );

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

    SDL_sem* m_Semaphore;
};

#endif


