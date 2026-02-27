//=============================================================================
// Copyright (C) 2002 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   Win32Platform   
//
// Description: Abstracts the differences for setting up and shutting down
//              the different platforms.
//
// History:     + Stolen and cleaned up from Penthouse -- Darwin Chau
//
//=============================================================================

#ifndef WIN32PLATFORM_H
#define WIN32PLATFORM_H

//========================================
// System Includes
//========================================

#include <windows.h>

//========================================
// Nested Includes
//========================================
#include "platform.h" // base class
#include <data/config/gameconfig.h> // interface
#include <SDL2/SDL.h>

//========================================
// Forward References
//========================================
struct IRadMemoryHeap;
class tPlatform;
class tContext;

//=============================================================================
//
// Synopsis:    Provides abstraction for setting up and closing a win32 exe.
//
//=============================================================================
class Win32Platform : public GamePlatform, public GameConfigHandler
{
public:

    enum Resolution
    {
        Res_640x480 = 0,
        Res_800x600,
        Res_1024x768,
        Res_1152x864,
        Res_1280x1024,
        Res_1600x1200,
        Res_1920x1080,
        Res_2560x1440,
        Res_3840x2160,
        Res_5120x2880,
        Res_7680x4320
    };

public:

    // Static Methods for accessing this singleton.
    static Win32Platform* CreateInstance();
    static Win32Platform* GetInstance();
    static void DestroyInstance();

    // Had to workaround our nice clean design cause FTech must be init'ed
    // before anything else is done.
    static bool InitializeWindow();
    static void InitializeFoundation();
    static void InitializeMemory();
    static void ShutdownMemory();

    // Implement Platform interface.
    virtual void InitializePlatform();
    virtual void ShutdownPlatform();

    virtual void LaunchDashboard();
    virtual void ResetMachine();
    virtual void DisplaySplashScreen( SplashScreen screenID, 
        const char* overlayText = NULL, 
        float fontScale = 1.0f, 
        float textPosX = 0.0f,
        float textPosY = 0.0f,
        tColour textColour = tColour( 255,255,255 ),
        int fadeFrames = 3 );

    virtual void DisplaySplashScreen( const char* textureName,
        const char* overlayText = NULL, 
        float fontScale = 1.0f, 
        float textPosX = 0.0f,
        float textPosY = 0.0f,
        tColour textColour = tColour( 255,255,255 ),
        int fadeFrames = 3 );

    virtual bool OnDriveError( radFileError error, const char* pDriveName, void* pUserData );  
    virtual void OnControllerError(const char *msg);

    SDL_Window* GetWindow() const { return mWnd; }

    // Set the resolution of the display, or increase the size of the window.
    // Returns true if the change was successful, false if resolution is not supported.
    bool SetResolution( Resolution res, int bpp, bool fullscreen );

    Resolution GetResolution() const;
    int GetBPP() const;
    bool IsFullscreen() const;

    // Implementation of the GameConfigHandler interface
    virtual const char* GetConfigName() const;
    virtual int GetNumProperties() const;
    virtual void LoadDefaults();
    virtual void LoadConfig( ConfigString& config );
    virtual void SaveConfig( ConfigString& config );

private:

    // Constructors, Destructors, and Operators
    Win32Platform();
    virtual ~Win32Platform();

    // Unused Constructors, Destructors, and Operators
    Win32Platform( const Win32Platform& aPlatform );
    Win32Platform& operator=( const Win32Platform& aPlatform );

    // Methods from Platform
    virtual void InitializeFoundationDrive();
    virtual void ShutdownFoundation();

    virtual void InitializePure3D();
    virtual void ShutdownPure3D();

    // Initializes the d3d context
    void InitializeContext();

    // Resolution related methods
    static void TranslateResolution( Resolution res, int&x, int&y );
    bool IsResolutionSupported( Resolution res, int bpp ) const;

    // Windows methods.
    void ResizeWindow();
    static void ShowTheCursor( bool show );
    static int SDLCALL WndProc( void* userdata, SDL_Event* msg );

private:

    // Pointer to the one and only instance of this singleton.
    static Win32Platform* spInstance;

    // Private Attributes
    // Had to make these static because of the initialization order problem.
    static SDL_Window* mWnd;
    static void * mhMutex;
    static bool mShowCursor;

    // Pure 3D attributes
    tPlatform* mpPlatform; 
    tContext* mpContext;

    // window properties.
    Resolution mResolution;
    int mbpp;
    bool mFullscreen;
    char mRenderer[ConfigString::MaxLength];
};

#endif // WIN32PLATFORM_H
