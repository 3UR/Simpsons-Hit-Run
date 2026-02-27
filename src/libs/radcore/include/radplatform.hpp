
//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        radplatform.hpp
//
// Subsystem:   Foundation Technologies - Platform System
//
// Description: This file contains all definitions and interfaces required to
//              interact with platform object. The platform object, as the 
//              name implies, is platform specific. It is used for performing
//              platform specific initialization and for obtaining OS specific
//              information such as window handles.
//
// Notes:       These functions are provided to initialize the platform.
//              Should be invoked very early in the intialization phase of
//              the game before most other Foundation Technology components
//              are used. Each platfrom has a specific initialization.
//              Termination function is the same and should be invoked prior
//              to game termination.
//
// Revisions:   Mar 4, 2001        Creation
//
//=============================================================================

#ifndef RADPLATFORM_HPP
#define RADPLATFORM_HPP

//=============================================================================
// Build Configuration Check
//=============================================================================

#if !defined(RAD_UWP) && !defined(RAD_WIN32)
    #error 'FTech requires definition of RAD_UWP or RAD_WIN32'
#endif

//=============================================================================
// Include Files
//=============================================================================

#include <stdlib.h>
#include <radobject.hpp>
#include <radmemory.hpp>

#if defined(RAD_WIN32) || defined(RAD_UWP)
//
// windows.h must be included because radPlatformInitialize( ) need HWND and
// HINSTANCE definition.
//
#include <windows.h>
#endif

//=============================================================================
// Forward Declarations
//=============================================================================

struct IRadPlatform;
struct SDL_Window;
union SDL_Event;

//=============================================================================
// Generic Public Functions
//=============================================================================

//
// Invoked this function as one of the very last functions in your game
//
void radPlatformTerminate( void );

//
// Use this member to get an interface to the plaform specific interface. If the
// system has not been initialized, NULL will be returned. If you maintain the interface
// pointer, remember to addref it.
//
IRadPlatform* radPlatformGet( void );

//
// These functions are provided to convert between little and big endian.
//
#define RAD_LITTLE_ENDIAN

template<typename T>
inline T radPlatformEndian( T value ) { return( value ); }

template<>
inline unsigned short radPlatformEndian<unsigned short>( unsigned short value ) { return(value); }
template<>
inline unsigned int radPlatformEndian<unsigned int>( unsigned int value ) { return( value ); }
template<>
inline float radPlatformEndian<float>( float value ) { return( value ); }

#define radPlatformEndian16     radPlatformEndian<unsigned short>
#define radPlatformEndian32     radPlatformEndian<unsigned int>
#define radPlatformEndianFloat  radPlatformEndian<float>

#if defined(RAD_WIN32) || defined(RAD_UWP)

//=============================================================================
// RAD_WIN32 Platform
//=============================================================================

//
// Windows requires the game provide the main window handle and the module
// instance.
//
void radPlatformInitialize( SDL_Window* pMainWindow, radMemoryAllocator = RADMEMORY_ALLOC_DEFAULT );

//
// Interface used to field messages from the main window.
//
struct IRadPlatformWin32MessageCallback
{
    virtual void OnWindowMessage
    (
        SDL_Window* pWnd, 
        const SDL_Event* event
    ) = 0;
};

//
// Windows plaform interface. 
//
struct IRadPlatform : public IRefCount
{
    // TODO(3ur): callbacks are fine to stay uncommented yes? maybe? no?
#ifndef RAD_UWP
    virtual HWND GetMainWindowHandle( void ) = 0;
    virtual HINSTANCE GetInstanceHandle( void ) = 0;
#endif
    virtual void RegisterMainWindowCallback
    (
        IRadPlatformWin32MessageCallback* pICallback
    ) = 0;
    virtual void UnRegisterMainWindowCallback
    (
        IRadPlatformWin32MessageCallback* pICallback
    ) = 0;
};

#endif // RAD_WIN32 || RAD_UWP
#endif // RADPLATFORM_HPP