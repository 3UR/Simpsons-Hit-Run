//===========================================================================
// Copyright (C) 2025 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   UwpPlatform   
//
// Description: Abstracts the differences for setting up and shutting down
//              the different platforms.
//
// History:     + Stolen and cleaned up from Penthouse -- Darwin Chau
//
//===========================================================================

//========================================
// System Includes
//========================================

// Standard Lib
#include <stdlib.h>
#include <string.h>

// Pure 3D
#include <p3d/anim/compositedrawable.hpp>
#include <p3d/anim/expression.hpp>
#include <p3d/anim/multicontroller.hpp>
#include <p3d/anim/polyskin.hpp>
#include <p3d/anim/sequencer.hpp>
#include <p3d/anim/skeleton.hpp>
#include <p3d/camera.hpp>
#include <p3d/gameattr.hpp>
#include <p3d/image.hpp>
#include <p3d/imagefont.hpp>
#include <p3d/light.hpp>
#include <p3d/locator.hpp>
#include <p3d/platform.hpp>
#include <p3d/scenegraph/scenegraph.hpp>
#include <p3d/sprite.hpp>
#include <p3d/utility.hpp>
#include <p3d/texture.hpp>
#include <p3d/file.hpp>
#include <p3d/shader.hpp>
#include <p3d/matrixstack.hpp>
#include <p3d/memory.hpp>
#include <p3d/bmp.hpp>
#include <p3d/png.hpp>
#include <p3d/targa.hpp>
#include <p3d/font.hpp>
#include <p3d/texturefont.hpp>
#include <p3d/unicode.hpp>
#include <p3d/shadow.hpp>
#include <p3d/anim/animatedobject.hpp>
#include <p3d/anim/vertexanimkey.hpp>

// Pure 3D: Loader-specific
#include <render/Loaders/GeometryWrappedLoader.h>
#include <render/Loaders/StaticEntityLoader.h>
#include <render/Loaders/StaticPhysLoader.h>
#include <render/Loaders/TreeDSGLoader.h>
#include <render/Loaders/FenceLoader.h>
#include <render/Loaders/IntersectLoader.h>
#include <render/Loaders/AnimCollLoader.h>
#include <render/Loaders/AnimDSGLoader.h>
#include <render/Loaders/DynaPhysLoader.h>
#include <render/Loaders/InstStatPhysLoader.h>
#include <render/Loaders/InstStatEntityLoader.h>
#include <render/Loaders/WorldSphereLoader.h>
#include <loading/roaddatasegmentloader.h>
#include <render/Loaders/BillboardWrappedLoader.h>
#include <render/Loaders/instparticlesystemloader.h>
#include <render/Loaders/breakableobjectloader.h>
#include <render/Loaders/AnimDynaPhysLoader.h>
#include <render/Loaders/lensflareloader.h>
#include <p3d/effects/particleloader.hpp>
#include <p3d/effects/opticloader.hpp>
#include <stateprop/statepropdata.hpp>

// Foundation Tech
#include <raddebug.hpp>
#include <radthread.hpp>
#include <radplatform.hpp>
#include <radtime.hpp>
#include <radmemorymonitor.hpp>
#include <raddebugcommunication.hpp>
#include <raddebugwatch.hpp>
#include <radfile.hpp>
#include <radmovie2.hpp>

//This is so we can get the name of the file that's failing.
#include <../src/radfile/common/requests.hpp>

// sim - for InstallSimLoaders
#include <simcommon/simutility.hpp>

// To turn off movies during an error.
#include <presentation/fmvplayer/fmvplayer.h>
#include <presentation/presentation.h>
#include <presentation/language.h>
#include <presentation/gui/guitextbible.h>

//========================================
// Project Includes
//========================================
#include <input/inputmanager.h>

#include <main/uwpplatform.h>
#include <main/game.h>
#include <main/errorsUWP.h>

#include <render/RenderManager/RenderManager.h>
#include <render/Loaders/AllWrappers.h>
#include <memory/srrmemory.h>

#include <loading/locatorloader.h>
#include <loading/cameradataloader.h>
#include <loading/roadloader.h>
#include <loading/pathloader.h>
#include <loading/intersectionloader.h>

#include <atc/atcloader.h>
#include <debug/debuginfo.h>
#include <constants/srrchunks.h>
#include <radload/radload.hpp>
#include <sound/soundmanager.h>
#include <data/gamedatamanager.h>

#define XBOX_SECTION "XBOX_SECTION"

//******************************************************************************
//
// Global Data, Local Data, Local Classes
//
//******************************************************************************

// Static pointer to instance of singleton.
UwpPlatform* UwpPlatform::spInstance = NULL;

// Other static members.
SDL_Window* UwpPlatform::mWnd = NULL;

//The Adlib font. <sigh>
#include <font/defaultfont.h>

//
// The depth of the rendering area.  This value only has an effect
// when Pure3D has taken over the entire display.  When running in
// a window on the desktop, Pure3D uses the same bit depth as the
// desktop.  Pure3D only supports 16, and 32 rendering depths.
//
static const int WindowBPP = 32;

void LoadMemP3DFile(unsigned char* buffer, unsigned int size, tEntityStore* store)
{
    tFileMem* file = new tFileMem(buffer, size);
    file->AddRef();
    file->SetFilename("memfile.p3d");
    p3d::loadManager->GetP3DHandler()->Load(file, p3d::inventory);
    file->Release();
}

//******************************************************************************
//
// Public Member Functions
//
//******************************************************************************

//==============================================================================
// UwpPlatform::CreateInstance
//==============================================================================
//
// Description: Creates the UwpPlatform.
//
// Parameters:	None.
//
// Return:      Pointer to the UwpPlatform.
//
// Constraints: This is a singleton so only one instance is allowed.
//
//==============================================================================
UwpPlatform* UwpPlatform::CreateInstance()
{
    MEMTRACK_PUSH_GROUP("UwpPlatform");
    rAssert(spInstance == NULL);

    spInstance = new(GMA_PERSISTENT) UwpPlatform;
    rAssert(spInstance);
    MEMTRACK_POP_GROUP("UwpPlatform");

    return spInstance;
}

//==============================================================================
// UwpPlatform::GetInstance
//==============================================================================
//
// Description: - Access point for the UwpPlatform singleton.  
//
// Parameters:	None.
//
// Return:      Pointer to the UwpPlatform.
//
// Constraints: This is a singleton so only one instance is allowed.
//
//==============================================================================
UwpPlatform* UwpPlatform::GetInstance()
{
    rAssert(spInstance != NULL);

    return spInstance;
}


//==============================================================================
// UwpPlatform::DestroyInstance
//==============================================================================
//
// Description: Destroy the UwpPlatform.
//
// Parameters:	None.
//
// Return:      None.
//
//==============================================================================
void UwpPlatform::DestroyInstance()
{
    rAssert(spInstance != NULL);

    delete(GMA_PERSISTENT, spInstance);
    spInstance = NULL;
}

//==============================================================================
// UwpPlatform::InitializeWindow
//==============================================================================
// Description: Creates the window class and window instance for the application.
//              We must do this before initializing the platform.
//
// Parameters:	hInstance - the handle to the instance.
//
// Return:      void
//
// Constraints: Must be initialized before the platform.
//
//==============================================================================
void UwpPlatform::InitializeWindow()
{
    // These three attributes must be set prior to creating the first window
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);

    mWnd = SDL_CreateWindow(
        "The Simpsons: Hit & Run", // Window title (not really used)
        SDL_WINDOWPOS_UNDEFINED,   // Window positions not used
        SDL_WINDOWPOS_UNDEFINED,   //
        1920,                      // Width of framebuffer
        1080,                      // Height of framebuffer
        SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN_DESKTOP // Flags, need FULLSCREEN to stretch a lower res
    );

    rAssert(mWnd != NULL);
#if defined( RAD_DEBUG )
    SDL_SetHint(SDL_HINT_EVENT_LOGGING, "1");
#endif
}

//==============================================================================
// UwpPlatform::InitializeFoundation
//==============================================================================
// Description: FTech must be setup first so that all the memory services
//              are ready to go before we begin allocating anything.
//
// Parameters:	None.
//
// Return:      None.
//
// Constraints: The FTech systems must be initialized in a particular order.
//              Consult their documentation before changing.
//
//==============================================================================
void UwpPlatform::InitializeFoundation()
{
    //
    // Initialize the memory heaps
    //
    UwpPlatform::InitializeMemory();

#ifndef FINAL
    //
    // Register an out-of-memory display handler in case something goes bad
    // while allocating the heaps
    //
    ::radMemorySetOutOfMemoryCallback(PrintOutOfMemoryMessage, NULL);
#endif

    // Setup the memory heaps
    //
    HeapMgr()->PrepareHeapsStartup();

    // Seed the heap stack
    //
    HeapMgr()->PushHeap(GMA_PERSISTENT);

    //
    // Initilalize the platform system
    // 
    ::radPlatformInitialize( mWnd );

    //
    // Initialize the timer system
    //
    ::radTimeInitialize();

    //
    // Initialize the debug communication system.
    //
    ::radDbgComTargetInitialize(WinSocket,
        radDbgComDefaultPort, // Default
        NULL,                 // Default
        GMA_DEBUG);


    //
    // Initialize the Watcher.
    //
    ::radDbgWatchInitialize("SRR2",
        32 * 16384, // 2 * Default
        GMA_DEBUG);

    //
    // Initialize the file system.
    //
    ::radFileInitialize(50, // Default
        32, // Default
        GMA_PERSISTENT);

    ::radLoadInitialize();
    //radLoad->SetSyncLoading( true );	

    ::radDriveMount(0, GMA_PERSISTENT);

    //
    // Initialize the new movie player
    //
    ::radMovieInitialize2(GMA_PERSISTENT);

    HeapMgr()->PopHeap(GMA_PERSISTENT);
}

//==============================================================================
// UwpPlatform::InitializeMemory
//==============================================================================
//
// Description: 
//
// Parameters:  
//
// Return:      
//
//==============================================================================
void UwpPlatform::InitializeMemory()
{
    //
    // Only do this once!
    //
    if (gMemorySystemInitialized == true)
    {
        return;
    }

    gMemorySystemInitialized = true;

    //
    // Initialize the thread system.
    //
    ::radThreadInitialize();

    //
    // Initialize the memory system.
    //
    ::radMemoryInitialize();
}




//==============================================================================
// UwpPlatform::InitializePlatform
//==============================================================================
// Description: Get the device ready to go.
//
// Parameters:	None.
//
// Return:      None.
//
//==============================================================================
void UwpPlatform::InitializePlatform()
{
    HeapMgr()->PushHeap(GMA_PERSISTENT);

    //
    // Rendering is good.
    //
    InitializePure3D();

    //
    // Add anything here that needs to be before the drive is opened.
    //
    DisplaySplashScreen(Error); // blank screen

    //
    // Opening the drive is SLOW...
    //
    InitializeFoundationDrive();

    //
    // Initialize the controller.
    //
    GetInputManager()->Init();

#if 0

    //
    // Display Title IP address.
    //
    XNADDR addr;
    XNetGetTitleXnAddr(&addr);

    const int BUFFER_SIZE = 256;
    char ip[BUFFER_SIZE];
    XNetInAddrToString(addr.ina, ip, BUFFER_SIZE);
    rDebugString(ip);
    rDebugString("\n");

#endif // FINAL

    HeapMgr()->PopHeap(GMA_PERSISTENT);

}


//==============================================================================
// UwpPlatform::ShutdownPlatform
//==============================================================================
// Description: Shut down the PS2.
//
// Parameters:	None.
//
// Return:      None.
//
//==============================================================================
void UwpPlatform::ShutdownPlatform()
{
    ShutdownPure3D();
    ShutdownFoundation();
}

//=============================================================================
// UwpPlatform::ResetMachine
//=============================================================================
// Description: resets the xbox
//
// Parameters:  ()
//
// Return:      void 
//
//=============================================================================
void UwpPlatform::ResetMachine()
{
    // TODO(3ur): do we need this ?
}

//=============================================================================
// UwpPlatform::LaunchDashboard
//=============================================================================
// Description: We use this a the emergency exit from the game if we arent in a context that suppose the transition 
//                    to the CONTEXT_EXIT
// Parameters:  ()
//
// Return:      void 
//
//=============================================================================
void UwpPlatform::LaunchDashboard()
{
    //
    // TODO(3UR): Maybe we need to change logic here to be more like win32platform (comment out some stuff)
    //

    GetLoadingManager()->CancelPendingRequests();

    //TODO: Make sure sounds shut down too.
    GetSoundManager()->SetMasterVolume(0.0f);

    DisplaySplashScreen(FadeToBlack);

    GetPresentationManager()->StopAll();

    //Oh boy.
    GameDataManager::DestroyInstance();  //Get rid of memcards


    p3d::loadManager->CancelAll();

    SoundManager::DestroyInstance();

    ShutdownPlatform();
    ResetMachine();
}


//=============================================================================
// UwpPlatform::DisplaySplashScreen
//=============================================================================
// Description: Comment
//
// Parameters:  ( SplashScreen screenID, 
//                const char* overlayText = NULL, 
//                float fontScale = 1.0f, 
//                float textPosX = 0.0f, 
//                float textPosY = 0.0f,
//                tColour textColour,
//                int fadeFrames = 3 )
//
// Return:      void 
//
//=============================================================================
void UwpPlatform::DisplaySplashScreen(SplashScreen screenID,
    const char* overlayText,
    float fontScale,
    float textPosX,
    float textPosY,
    tColour textColour,
    int fadeFrames)
{
    HeapMgr()->PushHeap(GMA_TEMP);

    p3d::inventory->PushSection();
    p3d::inventory->AddSection(XBOX_SECTION);
    p3d::inventory->SelectSection(XBOX_SECTION);

    P3D_UNICODE unicodeText[256];

    // Save the current Projection mode so I can restore it later
    pddiProjectionMode pm = p3d::pddi->GetProjectionMode();
    p3d::pddi->SetProjectionMode(PDDI_PROJECTION_DEVICE);

    pddiCullMode cm = p3d::pddi->GetCullMode();
    p3d::pddi->SetCullMode(PDDI_CULL_NONE);


    //CREATE THE FONT
    tTextureFont* thisFont = NULL;

    // Convert memory buffer into a texturefont.
    //
    //p3d::load(gFont, DEFAULTFONT_SIZE, GMA_TEMP);
    LoadMemP3DFile(gFont, DEFAULTFONT_SIZE, p3d::inventory);

    thisFont = p3d::find<tTextureFont>("adlibn_20");
    rAssert(thisFont);

    thisFont->AddRef();
    tShader* fontShader = thisFont->GetShader();
    //fontShader->SetInt( )


    p3d::AsciiToUnicode(overlayText, unicodeText, 256);

    // Make the missing letter into somthing I can see
    //
    thisFont->SetMissingLetter(p3d::ConvertCharToUnicode('j'));

    int a = 0;

    do
    {
        p3d::pddi->SetColourWrite(true, true, true, true);
        p3d::pddi->SetClearColour(pddiColour(0, 0, 0));
        p3d::pddi->BeginFrame();
        p3d::pddi->Clear(PDDI_BUFFER_COLOUR);

        //This is for fading in the font and shaders.
        int bright = 255;
        if (a < fadeFrames) bright = (a * 255) / fadeFrames;
        if (bright > 255) bright = 255;
        tColour c(bright, bright, bright, 255);

        //Display font
        if (overlayText != NULL)
        {
            tColour colour = textColour;
            colour.SetAlpha(bright);

            thisFont->SetColour(colour);

            p3d::pddi->SetProjectionMode(PDDI_PROJECTION_ORTHOGRAPHIC);
            p3d::stack->Push();
            p3d::stack->LoadIdentity();

            p3d::stack->Translate(textPosX, textPosY, 1.5f);
            float scaleSize = 1.0f / 480.0f;  //This is likely good for 528 also.
            p3d::stack->Scale(scaleSize * fontScale, scaleSize * fontScale, 1.0f);

            if (textPosX != 0.0f || textPosY != 0.0f)
            {
                thisFont->DisplayText(unicodeText);
            }
            else
            {
                thisFont->DisplayText(unicodeText, 3);
            }

            p3d::stack->Pop();
        }

        p3d::pddi->EndFrame();
        p3d::context->SwapBuffers();

        ++a;

    } while (a <= fadeFrames);

    // [ps]: flush out this screen now.
    if (screenID == FadeToBlack)
    {
        p3d::pddi->Clear(PDDI_BUFFER_ALL);
    }

    p3d::pddi->SetCullMode(cm);
    p3d::pddi->SetProjectionMode(pm);

    //Should do this after a vsync.
    thisFont->Release();

    p3d::inventory->RemoveSectionElements(XBOX_SECTION);
    p3d::inventory->DeleteSection(XBOX_SECTION);
    p3d::inventory->PopSection();

    HeapMgr()->PopHeap(GMA_TEMP);
}


//=============================================================================
// UwpPlatform::DisplaySplashScreen
//=============================================================================
// Description: Comment
//
// Parameters:  ( const char* textureName, 
//                const char* overlayText = NULL, 
//                float fontScale = 1.0f, 
//                float textPosX = 0.0f, 
//                float textPosY = 0.0f, 
//                tColour textColour,
//                int fadeFrames = 3 )
//
// Return:      void 
//
//=============================================================================
void UwpPlatform::DisplaySplashScreen(const char* textureName,
    const char* overlayText,
    float fontScale,
    float textPosX,
    float textPosY,
    tColour textColour,
    int fadeFrames)
{
}


//=============================================================================
// UwpPlatform::OnDriveError
//=============================================================================
// Description: Comment
//
// Parameters:  ( radFileError error, const char* pDriveName, void* pUserData )
//
// Return:      bool 
//
//=============================================================================
bool UwpPlatform::OnDriveError(radFileError error, const char* pDriveName, void* pUserData)
{
    bool inFrame = p3d::context->InFrame();

    const int NUM_RADFILE_ERRORS = 13;
    unsigned int errorIndex = error;

#ifdef PAL
    switch (CGuiTextBible::GetCurrentLanguage())
    {
    case Scrooby::XL_FRENCH:
    {
        errorIndex += 1 * NUM_RADFILE_ERRORS;

        break;
    }
    case Scrooby::XL_GERMAN:
    {
        errorIndex += 2 * NUM_RADFILE_ERRORS;

        break;
    }
    case Scrooby::XL_SPANISH:
    {
        errorIndex += 3 * NUM_RADFILE_ERRORS;

        break;
    }
    default:
    {
        if (!CGuiTextBible::IsTextBibleLoaded())
        {
            switch (Language::GetHardwareLanguage())
            {
            case Language::FRENCH:
            {
                errorIndex += 1 * NUM_RADFILE_ERRORS;
                break;
            }
            case Language::GERMAN:
            {
                errorIndex += 2 * NUM_RADFILE_ERRORS;
                break;
            }
            case Language::SPANISH:
            {
                errorIndex += 3 * NUM_RADFILE_ERRORS;
                break;
            }
            }
        }
        break;
    }
    }
#endif // PAL

    rAssert(errorIndex < sizeof(ERROR_STRINGS) / sizeof(ERROR_STRINGS[0]));

    switch (error)
    {
    case Success:
    {
        if (mErrorState != NONE)
        {
            if (inFrame) p3d::context->EndFrame(true);
            DisplaySplashScreen(FadeToBlack);
            if (inFrame) p3d::context->BeginFrame();
            mErrorState = NONE;
            mPauseForError = false;
        }

        if (GetPresentationManager()->GetFMVPlayer()->IsPlaying())
        {
            GetPresentationManager()->GetFMVPlayer()->UnPause();
        }
        else
        {
            GetSoundManager()->ResumeAfterMovie();
        }
        return true;
        break;
    }
    case FileNotFound:
    {
        rAssert(pUserData != NULL);

        radFileRequest* request = static_cast<radFileRequest*>(pUserData);
        const char* fileName = request->GetFilename();

        //Get rid of the slashes.
        unsigned int i;
        unsigned int lastIndex = 0;
        for (i = 0; i < strlen(fileName); ++i)
        {
            if (fileName[i] == '\\')
            {
                lastIndex = i;
            }
        }

        unsigned int adjustedIndex = lastIndex == 0 ? lastIndex : lastIndex + 1;

        char adjustedName[32];
        strncpy(adjustedName, &fileName[adjustedIndex], (strlen(fileName) - lastIndex));
        adjustedName[strlen(fileName) - lastIndex] = '\0';

        char errorString[256];
        sprintf(errorString, "%s:\n%s", ERROR_STRINGS[errorIndex], adjustedName);
        if (inFrame) p3d::context->EndFrame(true);
        DisplaySplashScreen(Error, errorString, 1.0f, 0.0f, 0.0f, tColour(255, 255, 255), 0);
        if (inFrame) p3d::context->BeginFrame();
        mErrorState = P_ERROR;
        mPauseForError = true;

        if (GetPresentationManager()->GetFMVPlayer()->IsPlaying())
        {
            GetPresentationManager()->GetFMVPlayer()->Pause();
        }
        else
        {
            GetSoundManager()->StopForMovie();
        }
        return true;
    }
    case ShellOpen:
    case WrongMedia:
    case NoMedia:
    case HardwareFailure:
    {
        //This could be the wrong disc.
        if (inFrame) p3d::context->EndFrame(true);
        DisplaySplashScreen(Error, ERROR_STRINGS[errorIndex], 1.0f, 0.0f, 0.0f, tColour(255, 255, 255), 0);
        if (inFrame) p3d::context->BeginFrame();
        mErrorState = P_ERROR;
        mPauseForError = true;

        if (GetPresentationManager()->GetFMVPlayer()->IsPlaying())
        {
            GetPresentationManager()->GetFMVPlayer()->Pause();
        }
        else
        {
            GetSoundManager()->StopForMovie();
        }
        return true;
    }
    default:
    {
        //Others are not supported.
        rAssert(false);
    }
    }

    return false;
}

void UwpPlatform::OnControllerError(const char* msg)
{
    bool inFrame = p3d::context->InFrame();
    if (inFrame) p3d::context->EndFrame(true);

    DisplaySplashScreen(Error, msg, 0.7f, 0.0f, 0.0f, tColour(255, 255, 255), 0);
    if (inFrame) p3d::context->BeginFrame();

    mErrorState    = CTL_ERROR;
    mPauseForError = true;

    if (GetPresentationManager()->GetFMVPlayer()->IsPlaying())
    {
        GetPresentationManager()->GetFMVPlayer()->Pause();
    }
    else
    {
        GetSoundManager()->StopForMovie();
    }
}


//******************************************************************************
//
// Protected Member Functions
//
//******************************************************************************


//==============================================================================
// UwpPlatform::InitializeFoundationDrive
//==============================================================================
// Description: Get FTech ready to go.
//
// Parameters:	None.
//
// Return:      None.
//
// Constraints: The FTech systems must be initialized in a particular order.
//              Consult their documentation before changing.
//
//==============================================================================
void UwpPlatform::InitializeFoundationDrive()
{
    //
    // Get the default drive and hold it open for the life of the game.
    // This is a costly operation so we only want to do it once.
    //

    char defaultDrive[radFileDrivenameMax + 1];

    ::radGetDefaultDrive(defaultDrive);

    ::radDriveOpen(
        &mpIRadDrive,
        defaultDrive,
        NormalPriority, // Default
        GMA_PERSISTENT
    );

    rAssert(mpIRadDrive != NULL);

    mpIRadDrive->RegisterErrorHandler(this, NULL);
}


//==============================================================================
// UwpPlatform::ShutdownFoundation
//==============================================================================
// Description: Shut down Foundation Tech.
//
// Parameters:	None.
//
// Return:      None.
//
// Constraints: The FTech systems must be terminated in the reverse order that
//              they were initialized in.
//
//==============================================================================
void UwpPlatform::ShutdownFoundation()
{
    //
    // Release the drive we've held open since the begining.
    //
    mpIRadDrive->Release();
    mpIRadDrive = NULL;

    //
    // Shutdown the systems in the reverse order.
    //
    ::radMovieTerminate2();
    ::radDriveUnmount();
    ::radLoadTerminate();
    ::radFileTerminate();
    ::radDbgWatchTerminate();
    ::radDbgComTargetTerminate();
    ::radTimeTerminate();
    ::radPlatformTerminate();
    ::radMemoryTerminate();
    ::radThreadTerminate();
}


//==============================================================================
// UwpPlatform::InitializePure3D
//==============================================================================
// Description: Get Pure3D ready to go.
//
// Parameters:	None.
//
// Return:      None.
//
//==============================================================================
void UwpPlatform::InitializePure3D()
{
    MEMTRACK_PUSH_GROUP("UwpPlatform");
    
    //
    // Initialise Pure3D platform object.
    // This call differs between different platforms.  The Win32 version,
    // for example requires the application instance to be passed in.
    //
    mpPlatform = tPlatform::Create( mWnd );
    rAssert(mpPlatform != NULL);
    
    //
    // Initialiase the Pure3D context object.
    // We have to create on of these for every window, and for every PDDI
    // instance we use for rendering.  Since almost every application only
    // uses one window and PDDI library at a time, we one need to create one
    // context.
    //
#pragma region Initialize Context
    tContextInitData init;

    //
    // This is the window we want to render into.
    //
    init.window = mWnd;

    //
    // Set the fullscreen/window mode.
    //
    init.displayMode = PDDI_DISPLAY_FULLSCREEN;

    //
    // All applications should supply PDDI_BUFFER_COLOUR.  PDDI_BUFFER_DEPTH
    // specifies that we also want to allocate a Z-buffer.  Some applications
    // may want to also specifiy PDDI_BUFFER_STENCIL to allocate a stencil
    // buffer.
    //
    init.bufferMask = PDDI_BUFFER_COLOUR | PDDI_BUFFER_DEPTH | PDDI_BUFFER_STENCIL;
    init.enableSnapshot = true;

    //
    // These values only take effect in fullscreen mode.  In windowed mode, the
    // dimensions of the window define the rendering area.  We'll define them
    // anyway for completeness sake.
    //
    
    // TODO(3UR): commented for now because why is this in sdl_create_window but also here
    //init.xsize = 1920;
    //init.ysize = 1080;
    
    //
    // Depth of the rendering buffer.  Again, this value only works in
    // fullscreen mode.  In window mode, the depth of the desktop is used.
    // This value should be either 16 or 32.  Depths of 4, 8, and 24 are not
    // supported by most 3D hardware, and not by Pure3D.
    //
    init.bpp = WindowBPP;
    
    // TODO: Investigate VSync - 3UR: I set this to true if there is issues OK
    init.lockToVsync = true;

    //
    // Create the context.
    //
    mpContext = mpPlatform->CreateContext(&init);
    rAssert(mpContext != NULL);

    //
    // Assign this context to the platform.
    //
    mpPlatform->SetActiveContext(mpContext);
    p3d::pddi->EnableZBuffer(true);
#pragma endregion Initialize Context

    //
    // This call installs chunk handlers for all the primary chunk types that
    // Pure3D supports.  This includes textures, materials, geometries, and the
    // like.
    //
    //    p3d::InstallDefaultLoaders();
    P3DASSERT(p3d::context);
    tP3DFileHandler* p3d = new(GMA_PERSISTENT) tP3DFileHandler;
    //    p3d::loadManager->AddHandler(p3d, "p3d");
    p3d::context->GetLoadManager()->AddHandler(p3d, "p3d");
    p3d::context->GetLoadManager()->AddHandler(new(GMA_PERSISTENT) tPNGHandler, "png");
    
    p3d::context->GetLoadManager()->AddHandler(new(GMA_PERSISTENT) tBMPHandler, "p3d");
    p3d::context->GetLoadManager()->AddHandler(new(GMA_PERSISTENT) tPNGHandler, "p3d");
    p3d::context->GetLoadManager()->AddHandler(new(GMA_PERSISTENT) tTargaHandler, "p3d");

    //    p3d->AddHandler(new tGeometryLoader);
    //    GeometryWrappedLoader* pGWL = new GeometryWrappedLoader;
    GeometryWrappedLoader* pGWL =
        (GeometryWrappedLoader*)GetAllWrappers()->mpLoader(AllWrappers::msGeometry);
    pGWL->SetRegdListener(GetRenderManager(), 0);
    p3d->AddHandler(pGWL);

    StaticEntityLoader* pSEL =
        (StaticEntityLoader*)GetAllWrappers()->mpLoader(AllWrappers::msStaticEntity);
    pSEL->SetRegdListener(GetRenderManager(), 0);
    p3d->AddHandler(pSEL);

    StaticPhysLoader* pSPL =
        (StaticPhysLoader*)GetAllWrappers()->mpLoader(AllWrappers::msStaticPhys);
    pSPL->SetRegdListener(GetRenderManager(), 0);
    p3d->AddHandler(pSPL);

    TreeDSGLoader* pTDL =
        (TreeDSGLoader*)GetAllWrappers()->mpLoader(AllWrappers::msTreeDSG);
    pTDL->SetRegdListener(GetRenderManager(), 0);
    p3d->AddHandler(pTDL);

    FenceLoader* pFL =
        (FenceLoader*)GetAllWrappers()->mpLoader(AllWrappers::msFenceEntity);
    pFL->SetRegdListener(GetRenderManager(), 0);
    p3d->AddHandler(pFL);

    IntersectLoader* pIL =
        (IntersectLoader*)GetAllWrappers()->mpLoader(AllWrappers::msIntersectDSG);
    pIL->SetRegdListener(GetRenderManager(), 0);
    p3d->AddHandler(pIL);

    AnimCollLoader* pACL =
        (AnimCollLoader*)GetAllWrappers()->mpLoader(AllWrappers::msAnimCollEntity);
    pACL->SetRegdListener(GetRenderManager(), 0);
    p3d->AddHandler(pACL);

    AnimDSGLoader* pAnimDSGLoader =
        (AnimDSGLoader*)GetAllWrappers()->mpLoader(AllWrappers::msAnimEntity);
    pAnimDSGLoader->SetRegdListener(GetRenderManager(), 0);
    p3d->AddHandler(pAnimDSGLoader);


    DynaPhysLoader* pDPL =
        (DynaPhysLoader*)GetAllWrappers()->mpLoader(AllWrappers::msDynaPhys);
    pDPL->SetRegdListener(GetRenderManager(), 0);
    p3d->AddHandler(pDPL);

    InstStatPhysLoader* pISPL =
        (InstStatPhysLoader*)GetAllWrappers()->mpLoader(AllWrappers::msInstStatPhys);
    pISPL->SetRegdListener(GetRenderManager(), 0);
    p3d->AddHandler(pISPL);

    InstStatEntityLoader* pISEL =
        (InstStatEntityLoader*)GetAllWrappers()->mpLoader(AllWrappers::msInstStatEntity);
    pISEL->SetRegdListener(GetRenderManager(), 0);
    p3d->AddHandler(pISEL);

    LocatorLoader* pLL =
        (LocatorLoader*)GetAllWrappers()->mpLoader(AllWrappers::msLocator);
    pLL->SetRegdListener(GetRenderManager(), 0);
    p3d->AddHandler(pLL);

    RoadLoader* pRL =
        (RoadLoader*)GetAllWrappers()->mpLoader(AllWrappers::msRoadSegment);
    pRL->SetRegdListener(GetRenderManager(), 0);
    p3d->AddHandler(pRL);

    PathLoader* pPL =
        (PathLoader*)GetAllWrappers()->mpLoader(AllWrappers::msPathSegment);
    pPL->SetRegdListener(GetRenderManager(), 0);
    p3d->AddHandler(pPL);

    WorldSphereLoader* pWSL =
        (WorldSphereLoader*)GetAllWrappers()->mpLoader(AllWrappers::msWorldSphere);
    pWSL->SetRegdListener(GetRenderManager(), 0);
    p3d->AddHandler(pWSL);

    LensFlareLoader* pLSL =
        (LensFlareLoader*)GetAllWrappers()->mpLoader(AllWrappers::msLensFlare);
    pLSL->SetRegdListener(GetRenderManager(), 0);
    p3d->AddHandler(pLSL);

    BillboardWrappedLoader* pBWL =
        (BillboardWrappedLoader*)GetAllWrappers()->mpLoader(AllWrappers::msBillboard);
    pBWL->SetRegdListener(GetRenderManager(), 0);
    p3d->AddHandler(pBWL);


    InstParticleSystemLoader* pInstParticleSystemLoader =
        (InstParticleSystemLoader*)GetAllWrappers()->mpLoader(AllWrappers::msInstParticleSystem);
    pInstParticleSystemLoader->SetRegdListener(GetRenderManager(), 0);
    p3d->AddHandler(pInstParticleSystemLoader);

    BreakableObjectLoader* pBreakableObjectLoader =
        (BreakableObjectLoader*)GetAllWrappers()->mpLoader(AllWrappers::msBreakableObject);
    pBreakableObjectLoader->SetRegdListener(GetRenderManager(), 0);
    p3d->AddHandler(pBreakableObjectLoader);

    AnimDynaPhysLoader* pAnimDynaPhysLoader =
        (AnimDynaPhysLoader*)GetAllWrappers()->mpLoader(AllWrappers::msAnimDynaPhys);
    pAnimDynaPhysLoader->SetRegdListener(GetRenderManager(), 0);
    p3d->AddHandler(pAnimDynaPhysLoader);

    AnimDynaPhysWrapperLoader* pAnimWrapperLoader =
        (AnimDynaPhysWrapperLoader*)GetAllWrappers()->mpLoader(AllWrappers::msAnimDynaPhysWrapper);
    pAnimWrapperLoader->SetRegdListener(GetRenderManager(), 0);
    p3d->AddHandler(pAnimWrapperLoader);

    p3d->AddHandler(new(GMA_PERSISTENT) tTextureLoader);
    p3d->AddHandler(new(GMA_PERSISTENT) tSetLoader);
    p3d->AddHandler(new(GMA_PERSISTENT) tShaderLoader);
    p3d->AddHandler(new(GMA_PERSISTENT) tCameraLoader);
    p3d->AddHandler(new(GMA_PERSISTENT) tGameAttrLoader);
    p3d->AddHandler(new(GMA_PERSISTENT) tLightLoader);

    p3d->AddHandler(new(GMA_PERSISTENT) tLocatorLoader);
    p3d->AddHandler(new(GMA_PERSISTENT) tLightGroupLoader);
    p3d->AddHandler(new(GMA_PERSISTENT) tImageLoader);
    p3d->AddHandler(new(GMA_PERSISTENT) tTextureFontLoader);
    p3d->AddHandler(new(GMA_PERSISTENT) tImageFontLoader);
    p3d->AddHandler(new(GMA_PERSISTENT) tSpriteLoader);
    //p3d->AddHandler(new(GMA_PERSISTENT) tBillboardQuadGroupLoader);
    p3d->AddHandler(new(GMA_PERSISTENT) tSkeletonLoader);
    p3d->AddHandler(new(GMA_PERSISTENT) tPolySkinLoader);
    p3d->AddHandler(new(GMA_PERSISTENT) tCompositeDrawableLoader);
    p3d->AddHandler(new(GMA_PERSISTENT) tVertexAnimKeyLoader);
    p3d->AddHandler(new(GMA_PERSISTENT) tAnimationLoader);
    p3d->AddHandler(new(GMA_PERSISTENT) tFrameControllerLoader);
    p3d->AddHandler(new(GMA_PERSISTENT) tMultiControllerLoader);
    p3d->AddHandler(new(GMA_PERSISTENT) tAnimatedObjectFactoryLoader);
    p3d->AddHandler(new(GMA_PERSISTENT) tAnimatedObjectLoader);
    p3d->AddHandler(new(GMA_PERSISTENT) tParticleSystemFactoryLoader);
    p3d->AddHandler(new(GMA_PERSISTENT) tParticleSystemLoader);
    p3d->AddHandler(new(GMA_PERSISTENT) tLensFlareGroupLoader);
    p3d->AddHandler(new(GMA_PERSISTENT) sg::Loader);

    p3d->AddHandler(new(GMA_PERSISTENT) tExpressionGroupLoader);
    p3d->AddHandler(new(GMA_PERSISTENT) tExpressionMixerLoader);
    p3d->AddHandler(new(GMA_PERSISTENT) tExpressionLoader);

    //ATCloader, hope this doesnt blow up
    p3d->AddHandler(new(GMA_PERSISTENT) ATCLoader);

    //p3d->AddHandler(new p3d::tIgnoreLoader);

    tSEQFileHandler* sequencerFileHandler = new(GMA_PERSISTENT) tSEQFileHandler;
    p3d::loadManager->AddHandler(sequencerFileHandler, "seq");

    // sim lib
    sim::InstallSimLoaders();

    p3d->AddHandler(new(GMA_PERSISTENT) CameraDataLoader, SRR2::ChunkID::FOLLOWCAM);
    p3d->AddHandler(new(GMA_PERSISTENT) CameraDataLoader, SRR2::ChunkID::WALKERCAM);
    p3d->AddHandler(new(GMA_PERSISTENT) IntersectionLoader);
    //p3d->AddHandler(new(GMA_PERSISTENT) RoadLoader);    
    p3d->AddHandler(new(GMA_PERSISTENT) RoadDataSegmentLoader);
    p3d->AddHandler(new(GMA_PERSISTENT) CStatePropDataLoader);
    MEMTRACK_POP_GROUP("UwpPlatform");
}


//==============================================================================
// UwpPlatform::ShutdownPure3D
//==============================================================================
// Description: Clean up and shut down Pure3D.
//
// Parameters:	None.
// 
// Return:      None.
//
//==============================================================================
void UwpPlatform::ShutdownPure3D()
{
    //
    // Clean-up the Pure3D Inventory
    //
    p3d::inventory->RemoveAllElements();
    p3d::inventory->DeleteAllSections();

    //
    // Clean-up the space taken by the Pure 3D context.
    //
    if (mpContext != NULL)
    {
        mpPlatform->DestroyContext(mpContext);
        mpContext = NULL;
    }

    //
    // Clean-up the space taken by the Pure 3D platform.
    //
    if (mpPlatform != NULL)
    {
        tPlatform::Destroy(mpPlatform);
        mpPlatform = NULL;
    }
}


//******************************************************************************
//
// Private Member Functions
//
//******************************************************************************

//==============================================================================
// UwpPlatform::UwpPlatform
//==============================================================================
// Description: Constructor.
//
// Parameters:	None.
//
// Return:      N/A.
//
//==============================================================================
UwpPlatform::UwpPlatform() :
    mpPlatform(NULL),
    mpContext(NULL)
{
}


//==============================================================================
// UwpPlatform::~UwpPlatform
//==============================================================================
// Description: Destructor.
//
// Parameters:	None.
//
// Return:      N/A.
//
//==============================================================================
UwpPlatform::~UwpPlatform()
{
    HeapManager::DestroyInstance();
}