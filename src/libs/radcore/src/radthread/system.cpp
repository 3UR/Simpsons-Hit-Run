//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        system.cpp
//
// Subsystem:	Radical Threading Services - System wide services
//
// Description:	This file contains the implementation of the threading system
//              initialization and helper functions.
//
// Author:		Peter Mielcarski
//
// Revisions:	V1.00	Jan 8, 2002
//
//=============================================================================

//=============================================================================
// Include Files
//=============================================================================

#include "pch.hpp"
#include <SDL2/SDL.h>

#include <raddebug.hpp>
#include <radthread.hpp>
#include "system.hpp"
#include "thread.hpp"

//=============================================================================
// Local Definitions
//=============================================================================

//=============================================================================
// Statics
//=============================================================================

//
// Flag to make sure initialization occurs and not more than once.
//
static bool g_SystemInitialized = false;

//
// Need an exclusion object for each of the various platforms.
//
static SDL_mutex* g_ExclusionObject = nullptr;

//=============================================================================
// Public Functions
//=============================================================================

//=============================================================================
// Function:    radThreadInitialize
//=============================================================================
// Description: This function should be invoked prior to invoking any other
//              threading services. It illegal to call it more than once.
//
// Parameters:  milliseconds - round robin periond
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

void radThreadInitialize( unsigned int milliseconds )
{
    rAssertMsg( !g_SystemInitialized, "radThread system already initialized\n");

    //
    // To manage our threading objects we in a thread safe manner, we need 
    // OS exculision objects for our own use. Create them here.
    //
    g_ExclusionObject = SDL_CreateMutex();

    g_SystemInitialized = true;

    //
    // Initialize the threading components.
    //
    radThread::Initialize( milliseconds );
}

//=============================================================================
// Function:    radThreadTerminate
//=============================================================================
// Description: This function should be at the end of all other threading objects
//              have beem released. It performs some simple cleanup.
//
// Parameters:  N/A
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

void radThreadTerminate( void )
{
    rAssertMsg( g_SystemInitialized, "radThread system has not been initialized\n");

    //
    // Terminate the threading components.
    //
    radThread::Terminate( );

    //
    // Free up the exclusion object using the appropriate platform specific 
    // function.
    //
    SDL_DestroyMutex(g_ExclusionObject);

    g_SystemInitialized = false;
}

//=============================================================================
// Function:    radThreadInternalLock
//=============================================================================
// Description: This function is used internally to provide thread exculision
//
// Parameters:  N/A
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

void radThreadInternalLock( void )
{
    rAssertMsg( g_SystemInitialized, "radThread system has not been initialized\n");

    //
    // Just perform the OS specific implementation.
    //
    SDL_LockMutex(g_ExclusionObject);
}

//=============================================================================
// Function:    radThreadInternalUnlock
//=============================================================================
// Description: This function is used internally to provide release thread exclusion
//
// Parameters:  N/A
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

void radThreadInternalUnlock( void )
{
    rAssertMsg( g_SystemInitialized, "radThread system has not been initialized\n");

    //
    // Just perform the OS specific implementation.
    //
    SDL_UnlockMutex(g_ExclusionObject);
}