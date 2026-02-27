//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _GLDISPLAY_HPP_
#define _GLDISPLAY_HPP_
#include <pddi/pddi.hpp>

#include <sys/time.h>
#include <GL/gl.h>
#include <GL/glx.h>

class pglContext;
class pglWrapper;

class pglDisplay : public pddiDisplay
{
public:
    pglDisplay();
    ~pglDisplay();

    // cross-platform functions
    bool InitDisplay(const pddiDisplayInit*);
    bool InitDisplay(int x, int y, int bpp);

    pddiDisplayInfo* GetDisplayInfo(void);

    int GetHeight(void);
    int GetWidth(void);
    int GetDepth(void);
    pddiDisplayMode GetDisplayMode(void);
    int GetNumColourBuffer(void);
    unsigned GetBufferMask(void);

    unsigned GetFreeTextureMem(void);

    void SwapBuffers();

    unsigned Screenshot(pddiColour* buffer, int nBytes);
    
    pddiSurface* NewSurface(int x, int y);

    // linux specific functions
    //long  ProcessXEvent();
    void  SetXDisplay(void* dpy);
    void* GetXDisplay();
    void  SetXWindow(void* win);
    void* GetXWindow();

    // internal functions
    void BeginTiming();
    float EndTiming();

    void SetContext(pglContext* c) {context = c;}
    bool ExtBGRA(void) { return extBGRA;}
    bool CheckExtension(char*);
    bool HasReset(void) { return reset; }

    static unsigned FillDisplayModes(pddiModeInfo*);

    void BeginContext(void);
    void EndContext(void);

    void SetGamma(float r, float g, float b);
    void GetGamma(float* r, float* g, float* b);


private:
    pddiDisplayMode mode;
    int winWidth;
    int winHeight;
    int winBitDepth;

    pglContext* context;

    unsigned short initialGammaRamp[3][256];
    float gammaR,gammaG,gammaB;

    Display* display;
    Window   window;
    
    bool extBGRA;
    bool reset;

    struct timeval beginTime;
};

// hack for now.  Ask Nigel how to fix this

#define GL_BGRA_EXT GL_RGBA

#endif

