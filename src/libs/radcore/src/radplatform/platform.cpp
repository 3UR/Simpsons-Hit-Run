//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        platform.cpp
//
// Subsystem:	Foundation Technologies - platform object
//
// Description:	This file contains the implementation of the plafrom services.
//
// Revisions:   Mar 13, 2001 Creation
//
//=============================================================================

//============================================================================
// Include Files
//============================================================================


#include "pch.hpp"
#include <string.h>
#include <radobject.hpp>
#include <radmemory.hpp>
#include <radmemorymonitor.hpp>
#include <radplatform.hpp>
#include <radstring.hpp>
#include <radobjectlist.hpp>

#if defined( RAD_WIN32 ) || defined( RAD_UWP )
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <windows.h>
#endif

#undef NULL
#define NULL 0

//============================================================================
// Constants
//============================================================================

#define VersionString "Foundation Technologies - RadCore - Version 3.01\n"
   
//============================================================================
// Class radPlatform
//============================================================================
// The following class is constructed as a global singleton object. It is 
// not destroyed since the memory system is not in place when contructed.
// The reference counting is not employed other than to catch problems
// releasing it.
//============================================================================

class radPlatform : public IRadPlatform
{
    public:

    // Don't write a constructor for this class, Construct() will get
    // called by the memory system before global constructors are called.

    ~radPlatform( void )
    {
    }

    void* operator new( size_t size, void* p )
    {
        return( p );
    }    	

    void Construct( radMemoryAllocator allocator )
    {
        rDebugString( VersionString );

        #if defined( RAD_WIN32 ) || defined( RAD_UWP )
            m_pMainWindow = NULL;
        #endif

        m_RefCount = 0;
        m_Initialized = false;
        m_ThisAllocator = allocator;
    }
    
    //
    // Add reference and delete do nothing except update reference count.
    //

    void AddRef( void )
    {
        rWarningMsg( m_Initialized, "radPlatform has not been initialized");
        m_RefCount++;
    }
            
    void Release( void )
    {
        rAssert( m_RefCount != 0 );
        m_RefCount--;
    }

#if defined( RAD_WIN32 ) || defined( RAD_UWP )

    //
    // Windows specific interfaces.
    //
    static int SDLCALL MainWindowProcedure
    (
        void* userdata, SDL_Event* event
    );

    void Initialize( SDL_Window* pMainWindow, radMemoryAllocator allocator )
    {
        rAssertMsg( !m_Initialized, "radPlatform already initialized");

        radMemoryMonitorIdentifyAllocation( this, g_nameFTech, "radPlatform" );

        m_ThisAllocator = allocator;
        m_Initialized = true;
        m_pMainWindow = pMainWindow;

        rWarningMsg( m_pMainWindow != NULL, "hMainWindow set to NULL in platform component" );

        #if defined( RAD_WIN32 ) || defined( RAD_UWP )
            SDL_VERSION( &m_wmInfo.version );
            SDL_GetWindowWMInfo( pMainWindow, &m_wmInfo );
        #endif
    }

    void Terminate( void  )
    {
        rAssertMsg( m_Initialized, "radPlatform not initialized");
        rAssertMsg( m_RefCount == 0, "radPlatorm still in use" );
        m_Initialized = false;
    }

    #if defined(RAD_WIN32)
	virtual HWND GetMainWindowHandle( void )
    {   
        rWarningMsg( m_wmInfo.subsystem == SDL_SYSWM_WINDOWS, "WM info doesn't match the windows subsystem." );
        return( m_wmInfo.info.win.window );
    }
    virtual HINSTANCE GetInstanceHandle( void )
    {
        return( m_wmInfo.info.win.hinstance );
    }
    #endif

    virtual void RegisterMainWindowCallback( IRadPlatformWin32MessageCallback* pICallback )
    {
        rAssert( pICallback != NULL );
        SDL_AddEventWatch( MainWindowProcedure, pICallback );
    }

    virtual void UnRegisterMainWindowCallback( IRadPlatformWin32MessageCallback* pICallback )
    {
        rAssert( pICallback != NULL );

        SDL_DelEventWatch( MainWindowProcedure, pICallback );
    }

    private:    
    
    SDL_Window* m_pMainWindow;
#if defined( RAD_WIN32 ) || defined( RAD_UWP )
    SDL_SysWMinfo m_wmInfo;
#endif

#endif

    //
    // Data members common to all plaforms.
    //
    unsigned int m_RefCount;
public:
    bool         m_Initialized;
    unsigned int m_ThisAllocator;

};

//
// The one and only platform object.
//
static radPlatform* pthePlatform;
static unsigned int thePlaftormSpace[(sizeof( radPlatform ) / sizeof( unsigned int))  + 1 ];

//=============================================================================
// Function: radPlatformInitialize
//=============================================================================
// This function should be invoked very early in the life of a game. Specific
// parameters required for each platform.
//
//=============================================================================
#if defined( RAD_WIN32 ) || defined( RAD_UWP )
//
// Windows requires the game provide the main window handle and the module
// instance.
//
void radPlatformInitialize( SDL_Window* pMainWindow, radMemoryAllocator allocator )
{
    pthePlatform = new( thePlaftormSpace ) radPlatform( );
    pthePlatform->Construct( allocator );
    pthePlatform->Initialize( pMainWindow, allocator );
}

int SDLCALL radPlatform::MainWindowProcedure
(
    void* userdata, SDL_Event* event
)
{
    radPlatform* pThis = pthePlatform;

    IRadPlatformWin32MessageCallback* callback = (IRadPlatformWin32MessageCallback*)userdata;

    callback->OnWindowMessage( pThis->m_pMainWindow, event );

    return 0;
}

#endif

//=============================================================================
// Function: radPlatformTerminate
//=============================================================================

void radPlatformTerminate( void )
{
    pthePlatform->Terminate( );
    pthePlatform->~radPlatform();
}

//=============================================================================
// Function: radPlatformTerminate
//=============================================================================

IRadPlatform* radPlatformGet( void )
{
    rAssertMsg( pthePlatform != NULL, "radPlatform not initialized" );

    return( pthePlatform );

}
