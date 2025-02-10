
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

#if !defined(RAD_PS2) && !defined(RAD_XBOX) && !defined(RAD_WIN32)
    #error 'FTech requires definition of RAD_PS2, RAD_XBOX, or RAD_WIN32'
#endif

//=============================================================================
// Include Files
//=============================================================================

#include <stdlib.h>
#include <radobject.hpp>
#include <radmemory.hpp>

#ifdef WIN32
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

#endif

#define radPlatformEndian16     radPlatformEndian<unsigned short>
#define radPlatformEndian32     radPlatformEndian<unsigned int>
#define radPlatformEndianFloat  radPlatformEndian<float>


//=============================================================================
// RAD_WIN32 Platform
//=============================================================================

#ifdef RAD_WIN32

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
        SDL_Window* pWnd, const SDL_Event* event
    ) = 0;
};

//
// Windows plaform interface. 
//
struct IRadPlatform : public IRefCount
{
#ifdef WIN32
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

//=============================================================================
// XBOX Platform
//=============================================================================

#ifdef RAD_XBOX

//
// XBOX initialization requires no parameters
//
void radPlatformInitialize( void );

//
// XBOX Platform interface.
//
struct IRadPlatform : public IRefCount
{
    // Nothing yet
};

#endif // RAD_XBOX

//=============================================================================
// Playstation2 Platform
//=============================================================================

#ifdef RAD_PS2

//
// On the PS2, provide the root path for IRXs. Also provide the name of the 
// IOP image to load if you want this operaton performed. The IRX path should
// be of the form "IRX\\", with no leading slash. Pass enumeration identifying
// the media the IOPs and image should be loaded from. 
//
enum radPlatformIOPMedia
{
    IOPMediaHost,           // Load from host
    IOPMediaCDVD            // Load from CDVD
};

//  
// Use this enumeration to specify the media that the game is using.
//
enum radPlatformGameMediaType
{
    GameMediaCD,            // Game is distributed on CD
    GameMediaDVD            // Game is distributed on DVD
};

void radPlatformInitialize( const char* pIrxPath, radPlatformIOPMedia IOPMedia, 
                            radPlatformGameMediaType = GameMediaCD,
                            const char* pIopImage = RADPLATFORM_IOP_IMG_FILENAME,
                            radMemoryAllocator = RADMEMORY_ALLOC_DEFAULT );

//
// PS2 plaform interface. 
//
struct IRadPlatform : public IRefCount
{
    virtual void LoadIrxModule
    (
        const char* pIrxFilenameNoPath,
        int argSize = 0,
        char* args = NULL
    ) = 0;
    virtual void UnloadIrxModule
    (
        const char* IrxFilenameNoPath
    ) = 0;
    virtual radPlatformGameMediaType GetMediaType
    (
        void
    ) = 0;
};

#endif // RAD_PS2
#endif // RADPLATFORM_HPP