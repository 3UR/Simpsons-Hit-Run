//=============================================================================
// Copyright (C) 2025 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   UWPPlatform   
//
// Description: Abstracts the differences for setting up and shutting down
//              the different platforms.
//
// History:     + Stolen and cleaned up from Penthouse -- Darwin Chau
//
//=============================================================================

#ifndef UWPPLATFORM_H
#define UWPPLATFORM_H

//========================================
// Nested Includes
//========================================
#include <main/platform.h> // base class
#include <SDL2/SDL.h>

//========================================
// Forward References
//========================================
struct IRadMemoryHeap;
class tPlatform;
class tContext;

//=============================================================================
//
// Synopsis:    Provides abstraction for setting up and closing a UWP app.
//
//=============================================================================
class UwpPlatform : public GamePlatform
{
public:

    // Static Methods for accessing this singleton.
    static UwpPlatform* CreateInstance();
    static UwpPlatform* GetInstance();
    static void DestroyInstance();

    // Had to workaround our nice clean design cause FTech must be init'ed
    // before anything else is done.
    static void InitializeWindow();
    static void InitializeFoundation();
    static void InitializeMemory();

    // Implement Platform interface.
    virtual void InitializePlatform();
    virtual void ShutdownPlatform();

    virtual void LaunchDashboard();
    virtual void ResetMachine();
    virtual void DisplaySplashScreen(SplashScreen screenID,
        const char* overlayText = NULL,
        float fontScale = 1.0f,
        float textPosX = 0.0f,
        float textPosY = 0.0f,
        tColour textColour = tColour(255, 255, 255),
        int fadeFrames = 3);

    virtual void DisplaySplashScreen(const char* textureName,
        const char* overlayText = NULL,
        float fontScale = 1.0f,
        float textPosX = 0.0f,
        float textPosY = 0.0f,
        tColour textColour = tColour(255, 255, 255),
        int fadeFrames = 3);

    virtual bool OnDriveError(radFileError error, const char* pDriveName, void* pUserData);
    virtual void OnControllerError(const char* msg);


protected:

    virtual void InitializeFoundationDrive();
    virtual void ShutdownFoundation();

    virtual void InitializePure3D();
    virtual void ShutdownPure3D();

private:

    // Constructors, Destructors, and Operators
    UwpPlatform();
    virtual ~UwpPlatform();

    // Pointer to the one and only instance of this singleton.
    static UwpPlatform* spInstance;

    // Private Attributes
    // Had to make these static because of the initialization order problem.
    static SDL_Window* mWnd;

    // Pure 3D attributes
    tPlatform* mpPlatform;
    tContext* mpContext;
};

#endif // UWPPLATFORM_H
