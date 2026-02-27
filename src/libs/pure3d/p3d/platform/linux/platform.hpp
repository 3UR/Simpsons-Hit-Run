//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _PLATFORM_HPP
#define _PLATFORM_HPP

#include <p3d/platform/linux/plat_types.hpp>
#include <pddi/pddi.hpp>

#include <malloc.h>
#include <pthread.h>

static const int P3D_MAX_CONTEXTS = 4;
static const int P3D_MAX_TASKS = 4;

class tContext;
class tFile;
class tPolySkinLoader;
class tTask;

class tContextInitData
{
public:
    int               xsize;         // x resolution (fullscreen only)
    int               ysize;         // y resolution (fullscreen only)
    int               bpp;           // bits per pixel
    pddiDisplayMode   displayMode;   // window/fullscreen mode
    unsigned          bufferMask;    // auxilliary buffer mask
    Display*          xdisplay;      // XWindows display
    Window            xwindow;       // X Window

    tContextInitData();
};

class tPlatformContext
{
public :
    tContext* context;
    void* windowHandle;
    void* pddiLib;

    tPlatformContext() { context = 0; windowHandle = 0; pddiLib = 0; }
};

class tPlatform
{
public:
    // platform creation/destruction
    static tPlatform* Create();
    static void Destroy(tPlatform*); 
    static tPlatform* GetPlatform(void);

    // context creation/destruction
    tContext* CreateContext(tContextInitData*);
    void DestroyContext(tContext*);

    // active context control
    void SetActiveContext(tContext*);
    tContext* GetActiveContext(void)  { return currentContext; }

    // files
    tFile* OpenFile(const char* filename);

    //tasks
    void AddTask(tTask*);
    void RemoveTask(tTask*);
    void CycleTasks(void);

    // polyskins
    tPolySkinLoader* CreatePolySkinLoader(void);

protected:
    tPlatform();
    ~tPlatform();

    static tPlatform* InternalCreate();
    static tPlatform* currentPlatform;

    tContext* currentContext;

    int nContexts;
    tPlatformContext contexts[P3D_MAX_CONTEXTS];

    struct taskEntry
    {
        tTask* task;
        pthread_t handle;
        bool active;
    } taskEntries[P3D_MAX_TASKS];
    unsigned taskCurrent;
};

#endif

