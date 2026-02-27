//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include <p3d/platform/sgi/platform.hpp>
#include <p3d/platform/sgi/plat_filemap.hpp>
#include <error.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <context.hpp>
#include <p3d/version.hpp>

static void* (*p3d_global_malloc)(unsigned) = NULL;
static void (*p3d_global_free)(void*) = NULL;

tContextInitData::tContextInitData()
{
        hwnd           = 0;
        xsize          = 640;
        ysize          = 480;
        bpp            = 16;
        displayMode    = PDDI_DISPLAY_WINDOW;
        nColourBuffer  = 2;
        bufferMask     = PDDI_BUFFER_COLOUR;
        forceSoftware  = true;
        forcePrimary   = true;
        PDDIlib[0]  = 0;
};

tPlatform* tPlatform::currentPlatform = NULL;

tPlatform::tPlatform(void* i)
{
    instance = i;
    currentContext = NULL;
    nContexts = 0;
}

tPlatform::~tPlatform()
{
}

tPlatform* tPlatform::Create(void* instance, void* (*m)(unsigned), void (*f)(void*))
{
    p3d_global_malloc = m;
    p3d_global_free   = f;

    if(!currentPlatform)
    {
        p3d::UsePermanentMem( true );

        currentPlatform = new tPlatform(instance);
        p3d::platform = currentPlatform;

        p3d::UsePermanentMem( false );
    }
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
    P3DASSERT(nContexts < maxContexts);

    PDDICREATEPROC PddiCreate;
    pddiDevice* device;
    pddiDisplay* display;
    pddiRenderContext* context;

    p3d::printf("Pure3D v%d.%d, build %d\n", PURE3D_VERSION_MAJOR, PURE3D_VERSION_MINOR, PURE3D_VERSION_BUILD);

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

    int chosenID = 0;
    int secondaryID = 0;

    pddiDisplayInfo* info;
    int nDisplay = device->GetDisplayInfo(&info);
    
    // find secondary display
    for(int i=0; i < nDisplay; i++)
    {
        if(!info[i].primary)
        {
            secondaryID = i;
            break;
        }
    }

    // build pddiDisplay   
    if(d->forcePrimary)
    {
        chosenID = 0;
    }
    else
    {
        chosenID = secondaryID;
    }

    display = device->NewDisplay(chosenID);
    display->InitDisplay( d->xsize, d->ysize, d->bpp,
                                 d->displayMode, d->nColourBuffer, d->bufferMask);
    
    // build pddiRenderContext
    context = device->NewRenderContext(display);
    P3DVERIFY(context != NULL, "NewRenderContext() failed");

    for(int find = 0; find < maxContexts; find++)
    {
        if(!contexts[find].context)
        {
            contexts[find].context = new tContext(device,display,context);
            contexts[find].pddiLib = PddiLib;
            contexts[find].windowHandle = d->hwnd;

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

void tPlatform::DestroyContext(tContext* context)
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
    //contexts[foundHandle].pddiLib = NULL;
    nContexts--;
}
    
void tPlatform::SetActiveContext(tContext* context)
{
    P3DASSERT( context );
    currentContext = context;
    p3d::context = context;
    p3d::inventory = context->GetInventory();
    p3d::stack = context->GetMatrixStack();
    p3d::loadManager = context->GetLoadManager();
    p3d::pddi = context->GetContext();
    p3d::device = context->GetDevice();
    p3d::display = context->GetDisplay();
}

tFileMap* tPlatform::OpenFile(char* filename)
{
    return new tLinuxFileMap(filename);
}

#include <p3d/anim/polyskingeneric.hpp>
tPolySkinLoader* tPlatform::CreatePolySkinLoader(void)
{
    return new tPolySkinGenericLoader;
}

//------------------------------------------------------------------------
bool tPlatform::ProcessWindowsMessage(void* win, unsigned msg, unsigned wparam, int lparam)
{
    if(!this)
        return false;
    
    // do nothing for now

    return false;
}

tPlatform* tPlatform::GetPlatform(void) 
{ 
    return currentPlatform; 
}

void* operator new(size_t sz) 
{
    return p3d_global_malloc(sz);
}

void operator delete(void* m) 
{
  p3d_global_free(m);
}

