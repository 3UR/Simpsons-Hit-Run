//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include <p3d/platform/win32/platform.hpp>
#include <p3d/platform/win32/plat_filemap.hpp>
#include <p3d/utility.hpp>
#include <p3d/memory.hpp>

#include <constants/version.hpp>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

tContextInitData::tContextInitData()
{
    window         = NULL;
    xsize          = 640;
    ysize          = 480;
    bpp            = 32;
    displayMode    = PDDI_DISPLAY_WINDOW;
    nColourBuffer  = 2;
    bufferMask     = PDDI_BUFFER_COLOUR | PDDI_BUFFER_DEPTH;
    adapterID      = 0;
    nSamples       = 0;
    PDDIlib[0]     = 0;
};

tPlatform* tPlatform::currentPlatform = NULL;

tPlatform::tPlatform(void* inst)
{
    instance = inst;
    currentContext = NULL;
    nContexts = 0;
}

tPlatform::~tPlatform()
{
}

tPlatform* tPlatform::Create(void* instance)
{
    p3d::UsePermanentMem(true);

    if (!currentPlatform)
    {
        currentPlatform = new tPlatform(instance);
        p3d::platform = currentPlatform;
    }

    p3d::UsePermanentMem(false);
    return currentPlatform;
}

void tPlatform::Destroy(tPlatform* plat)
{
    P3DASSERT(plat == currentPlatform);
    delete currentPlatform;
    currentPlatform = NULL;
}

tContext* tPlatform::CreateContext(tContextInitData* d)
{
    P3DASSERT(nContexts < P3D_MAX_CONTEXTS);

    pddiDevice* device;
    pddiDisplay* display;
    pddiRenderContext* context;
    pddiDisplayInfo* displayInfo;

    p3d::printf("Pure3D v%s, released %s\n", ATG_VERSION, ATG_RELEASE_DATE);

    p3d::UsePermanentMem(true);

    int success = pddiCreate(PDDI_VERSION_MAJOR, PDDI_VERSION_MINOR, &device);
    if (success != PDDI_OK)
    {
        if (success == PDDI_VERSION_ERROR)
        {
            P3DVERIFY(0, "Cannot initialize PDDI library due to an incorrect version");
        }
        else
        {
            P3DVERIFY(0, "Cannot initialize PDDI library, unknown error.");
        }
    }

    tDebug::CapturePDDIMessages(device);

    display = device->NewDisplay(d->adapterID);
    display->SetWindow(d->window);
    display->InitDisplay(d);
    // build pddiRenderContext
    context = device->NewRenderContext(display);
    P3DVERIFY(context != NULL, "NewRenderContext() failed");

    pddiLibInfo libInfo;
    device->GetLibraryInfo(&libInfo);
    displayInfo = display->GetDisplayInfo();

    p3d::printf("tContext created: PDDI v%d.%d (build %d) '%s', '%s'\n",
        libInfo.versionMajor,
        libInfo.versionMinor,
        libInfo.versionBuild,
        libInfo.description,
        displayInfo->description);

    for (int find = 0; find < P3D_MAX_CONTEXTS; find++)
    {
        if (!contexts[find].context)
        {
            contexts[find].context = new tContext(device, display, context);
            contexts[find].window = d->window;

            if (!currentContext)
                SetActiveContext(contexts[find].context);

            contexts[find].context->Setup();

            nContexts++;

            p3d::UsePermanentMem(false);

            return contexts[find].context;
        }
    }

    p3d::UsePermanentMem(false);

    return NULL;
}

void tPlatform::DestroyContext(tContext* context)
{
    int foundHandle = -1;
    for (int i = 0; i < nContexts; i++)
        if (contexts[i].context == context)
            foundHandle = i;

    P3DASSERT(foundHandle != -1);

    context->Shutdown();

    contexts[foundHandle].window = NULL;
    delete context;
    contexts[foundHandle].context = NULL;
    contexts[foundHandle].pddiLib = NULL;
    nContexts--;

    if (currentContext == context)
    {
        currentContext = NULL;
    }
}

void tPlatform::SetActiveContext(tContext* context)
{
    P3DASSERT(context);
    currentContext = context;
    p3d::context = context;
    p3d::inventory = context->GetInventory();
    p3d::stack = context->GetMatrixStack();
    p3d::loadManager = context->GetLoadManager();
    p3d::pddi = context->GetContext();
    p3d::device = context->GetDevice();
    p3d::display = context->GetDisplay();
}

#ifdef WIN32
tFile* tPlatform::OpenFile(const char* filename)
{
    tWin32FileMap* file = new tWin32FileMap(filename);
    if (!file->IsOpen())
    {
        file->Release();
        return NULL;
    }
    return file;
}
#endif

#if 0
P3D_U64 tPlatform::GetTimeFreq(void)
{
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    return frequency.QuadPart;
}

P3D_U64 tPlatform::GetTime(void)
{
    LARGE_INTEGER currentTime;
    QueryPerformanceCounter(&currentTime);
    return currentTime.QuadPart;
}
#endif

//------------------------------------------------------------------------
bool tPlatform::ProcessWindowsMessage(SDL_Window* win, const SDL_WindowEvent* event)
{
    if(!this)
        return false;

    for(int i = 0; i < nContexts; i++)
        //if(contexts[i].windowHandle == win)
            return contexts[i].context->RenderDisplay->ProcessWindowMessage(win, event) != 0;

    return false;
}

tPlatform* tPlatform::GetPlatform(void)
{
    return currentPlatform;
}