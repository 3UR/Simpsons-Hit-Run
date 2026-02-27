//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _PLATFORM_HPP
#define _PLATFORM_HPP

#include <p3d/platform/sgi/plat_types.hpp>
#include <pddi/pddi.hpp>

const int maxContexts = 4;

class tContext;
class tFile;
class tPolySkin;

class tContextInitData
{
public:
    void*           hwnd;          // window handle
    int             xsize;         // x resolution (fullscreen only)
    int             ysize;         // y resolution (fullscreen only)
    int             bpp;           // bits per pixel
    pddiDisplayMode displayMode;   // window/fullscreen mode
    int             nColourBuffer; // number of frame buffers
    unsigned        bufferMask;    // auxilliary buffer mask
    bool            forceSoftware; // disable hardware acceleration
    bool            forcePrimary;  // forcePrimary display
    char            PDDIlib[128];  // PDDI library path

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
    static tPlatform* Create(void* instance, void* (*m)(unsigned), void (*f)(void*));
    static void Destroy(tPlatform*); 
    static tPlatform* GetPlatform(void);

    // context creation/destruction
    tContext* CreateContext(tContextInitData*);
    void DestroyContext(tContext*);

    // active context control
    void SetActiveContext(tContext*);
    tContext* GetActiveContext(void)  { return currentContext; }

    // files
    tFile* OpenFile(char* filename);

    // polyskins
    tPolySkinLoader* CreatePolySkinLoader(void);
  
    // Platform specific funtions
    bool ProcessWindowsMessage(void* win, unsigned msg, unsigned wparam, int lparam);

protected:
    tPlatform(void* instance);
    ~tPlatform();

    static tPlatform* InternalCreate(void* instance);
    static tPlatform* currentPlatform;

    void* instance;
    tContext* currentContext;

    int nContexts;
    tPlatformContext contexts[maxContexts];
};

#endif

