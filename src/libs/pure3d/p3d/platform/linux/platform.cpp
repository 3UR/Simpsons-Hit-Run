//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include <p3d/platform/linux/platform.hpp>
#include <p3d/platform/linux/plat_filemap.hpp>
#include <p3d/error.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <p3d/context.hpp>
#include <p3d/utility.hpp>
#include <constants/version.hpp>

//#include <pthread.h>

tContextInitData::tContextInitData()
{
    xsize          = 640;
    ysize          = 480;
    bpp            = 32;
    displayMode    = PDDI_DISPLAY_WINDOW;
    nColourBuffer  = 2;
    bufferMask     = PDDI_BUFFER_COLOUR | PDDI_BUFFER_DEPTH | PDDI_BUFFER_STENCIL;
    xdisplay       = NULL;
};

tPlatform* tPlatform::currentPlatform = NULL;

tPlatform::tPlatform()
{
    currentContext = NULL;
    nContexts = 0;

    for(int i = 0; i < P3D_MAX_TASKS; i++)
    {
        taskEntries[i].handle = 0;
        taskEntries[i].task = NULL;
        taskEntries[i].active = false;
    }

    taskEntries[0].handle = pthread_self();
    taskEntries[0].active = true;

    P3DASSERT(taskEntries[0].handle);
    taskCurrent = 0;
}

tPlatform::~tPlatform()
{
    /* */
}

tPlatform* 
tPlatform::Create()
{
    if(!currentPlatform)
    {
        p3d::UsePermanentMem( true );
        currentPlatform = new tPlatform();
        p3d::platform = platform;
        p3d::UsePermanentMem( false );
    }
    return currentPlatform;
}

void
tPlatform::Destroy(tPlatform* plat)
{
    P3DASSERT(plat == currentPlatform);
    delete currentPlatform;
    currentPlatform = NULL;
}

tContext* 
tPlatform::CreateContext(tContextInitData* d)
{
    P3DASSERT(nContexts < P3D_MAX_CONTEXTS);

    PDDICREATEPROC PddiCreate;
    pddiDevice* device;
    pddiDisplay* display;
    pddiRenderContext* context;
    pddiDisplayInfo* displayInfo;

    p3d::printf("Pure3D v%s, released %s\n", ATG_VERSION, ATG_RELEASE_DATE);

    p3d::UsePermanentMem( true );
    PddiCreate = pddiCreate;

    int success = PddiCreate(PDDI_VERSION_MAJOR, PDDI_VERSION_MINOR, &device);
    if(success != PDDI_OK)
    {
        if(success == PDDI_VERSION_ERROR)
        {
            P3DVERIFY(0, "Cannot initialize PDDI library due to an incorrect version");
        }
        else
        {
            P3DVERIFY(0, "Cannot initialize PDDI library, unknown error.");
        }
    }

    tDebug::CapturePDDIMessages(device);

    display = device->NewDisplay(0);
    display->SetXDisplay( (void*) d->xdisplay );
    display->SetXWindow( (void*) d->xwindow );
    display->InitDisplay( d->xsize, d->ysize, d->bpp,
                                 d->displayMode, d->nColourBuffer, d->bufferMask, d->nSamples);
    // build pddiRenderContext
    context = device->NewRenderContext(display);
    P3DVERIFY(context != NULL, "NewRenderContext() failed");

    pddiLibInfo libInfo;
    device->GetLibraryInfo(&libInfo);
    displayInfo = display->GetDisplayInfo();

    p3d::log() <<
        "tContext created: PDDI v" <<
        libInfo.versionMajor << "." <<
        libInfo.versionMinor <<
        " (build " << libInfo.versionBuild << ") '" <<
        libInfo.description << "', '" <<
        displayInfo->description << "'." << log_endl;

    for(int find = 0; find < P3D_MAX_CONTEXTS; find++)
    {
        if(!contexts[find].context)
        {
            contexts[find].context = new tContext(device,display,context);

            if(!currentContext)
                SetActiveContext(contexts[find].context);

            contexts[find].context->Setup();

            nContexts++;

            p3d::UsePermanentMem( false );

            return contexts[find].context;
        }
    }

    p3d::UsePermanentMem( false );
    return NULL;
}

void 
tPlatform::DestroyContext(tContext* context)
{
    int foundHandle = -1;
    for(int i = 0; i < nContexts; i++)
        if(contexts[i].context == context)
            foundHandle = i;

    P3DASSERT(foundHandle != -1);

    context->Shutdown();

    contexts[foundHandle].windowHandle = NULL;
    delete context;
    contexts[foundHandle].context = NULL;
    nContexts--;
}
    
void 
tPlatform::SetActiveContext(tContext* context)
{
    currentContext = context;
    p3d::context = context;
    p3d::inventory = context->GetInventory();
    p3d::stack = context->GetMatrixStack();
    p3d::loadManager = context->GetLoadManager();
    p3d::pddi = context->GetContext();
    p3d::device = context->GetDevice();
    p3d::display = context->GetDisplay();
}

tFile* 
tPlatform::OpenFile(const char* filename)
{
    tLinuxFileMap* file = new tLinuxFileMap(filename);
    if(!file->IsOpen())
    {
        file->Release();
        return NULL;
    }
    return file;
}

tPlatform*
tPlatform::GetPlatform(void) 
{ 
    return currentPlatform; 
}

void tPlatform::AddTask(tTask* t)
{
    //...
}

void tPlatform::RemoveTask(tTask* t)
{
    //...
}

void tPlatform::CycleTasks(void)
{
    //...
}

