//=============================================================================
// Copyright (C) 2025 Radical Entertainment Ltd.  All rights reserved.
//
// File:        uwpmain.cpp
//
// Description: This file contains the main enrty point to the game.
//
// History:     + Stolen and cleaned up from Penthouse -- Darwin Chau
//
//=============================================================================

//========================================
// System Includes
//========================================
// Standard Library
#include <Windows.h>
#include <string.h>
// Foundation Tech
#include <raddebug.hpp>
#include <radobject.hpp>

//========================================
// Project Includes
//========================================
#include <main/game.h>
#include <main/uwpplatform.h>
#include <main/singletons.h>
#include <memory/memoryutilities.h>
#include <memory/srrmemory.h>

//========================================
// Forward Declarations
//========================================
static void LogOutputFunction(void* userdata, int category, SDL_LogPriority priority, const char* message)
{
    rDebugPrintf("[SDL] %s\n", message);
}

//=============================================================================
// Function:    main
//=============================================================================
//
// Description: Main SDL entry point.
// 
// Parameters:  Arguments
//
// Returns:     Error code
//
//=============================================================================
int SDL_main(int argc, char* argv[])
{
    //
    // Initialize SDL subsystems
    //

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0) return -1;

    SDL_LogSetAllPriority(SDL_LOG_PRIORITY_WARN);

    // this is really verbose and not useful unless debugging so just keep it disabled for now
    #if 0
    SDL_LogSetOutputFunction(LogOutputFunction, NULL);
    #endif

    //
    // Have to get FTech setup first so that we can use all the memory services.
    //
    UwpPlatform::InitializeWindow();
    UwpPlatform::InitializeFoundation();
    
    srand(Game::GetRandomSeed());


    // Now disable the default heap
    //
#ifndef RAD_RELEASE
    tName::SetAllocator(GMA_DEBUG);
#endif

    HeapMgr()->PushHeap(GMA_PERSISTENT);

    //
    // Instantiate all the singletons before doing anything else.
    //
    CreateSingletons();

    //
    // Construct the platform object.
    //
    UwpPlatform* pPlatform = UwpPlatform::CreateInstance();
    rAssert(pPlatform != NULL);
    
    //
    // Create the game object.
    //
    Game* pGame = Game::CreateInstance(pPlatform);
    rAssert(pGame != NULL);
    
     
    //
    // Initialize the game.
    //
    pGame->Initialize();

    HeapMgr()->PopHeap(GMA_PERSISTENT);

    //
    // Run it!  Control will not return from here until the game is stopped.
    //
    pGame->Run();
    
    //
    // Terminate the game (this frees all resources allocated by the game).
    //
    pGame->Terminate();
    
    //
    // Destroy the game object.
    //
    Game::DestroyInstance();
    
    //
    // Destroy the game and platform (do it in this order in case the game's 
    // destructor references the platform.
    //
    UwpPlatform::DestroyInstance();
    
    //
    // Show some debug output
    //
#ifdef RAD_DEBUG
    radObject::DumpObjects();
#endif;

    //
    // Dump all the singletons.
    //
    DestroySingletons();

    return 0;
}

//=============================================================================
// Function:    WinMain
//=============================================================================
//
// Description: Main UWP entry point. Based off https://github.com/worleydl/uwp_gl_sample/blob/master/uwp/main.cpp#L72C1-L73C1
// 
// Parameters:  win32 parameters
//
// Returns:     win32 return.
//
//=============================================================================
int CALLBACK WinMain(HINSTANCE, HINSTANCE, LPSTR argv, int argc)
{
    return SDL_WinRTRunApp(SDL_main, NULL);
}