//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================

#include <pddi/gl/display_linux/gldisplay.hpp>
#include <pddi/gl/display_linux/gl.hpp>
#include <pddi/gl/glcon.hpp>

#include <stdio.h>
#include <string.h>
#include <GL/gl.h>
#include <GL/glx.h>

#include <unistd.h>
#include <sys/time.h>

bool pglDisplay::CheckExtension( char *extName )
{
    char *p = (char *) glGetString(GL_EXTENSIONS);

    if((p == NULL) || (extName == NULL))
        return false;
    
    char *end;
    int extNameLen;

    extNameLen = strlen(extName);
    end = p + strlen(p);

    while (p < end) {
        int n = strcspn(p, " ");
        if ((extNameLen == n) && (strncmp(extName, p, n) == 0)) {
             return true;
        }
        p += (n + 1);
    }
    return false;
}

pglDisplay::pglDisplay()
{
    mode = PDDI_DISPLAY_WINDOW;
    winWidth = 640;
    winHeight = 480;
    winBitDepth = 32;

    context = NULL;

    display = NULL;

    extBGRA = false;

    gammaR = gammaG = gammaB = 1.0f;

    reset = true;
	m_ForceVSync = false;
}

pglDisplay::~pglDisplay()
{
}

void  pglDisplay::SetXDisplay(void* dpy)
{
    display = (Display*)dpy;
}

void* pglDisplay::GetXDisplay()
{
    return (void*)display;
}

void pglDisplay::SetXWindow(void* win)
{
    window = (Window)win;
}

void* pglDisplay::GetXWindow()
{
    return (void*)window;
}

bool pglDisplay::InitDisplay(const pddiDisplayInit* init)
{
    return InitDisplay(init->xsize, init->ysize, init->bpp);
}

bool pglDisplay::InitDisplay(int x, int y, int bpp)
{
    reset = true;

    winWidth = x;
    winHeight = y;

    char* glVendor   = (char*)glGetString(GL_VENDOR);
    char* glRenderer = (char*)glGetString(GL_RENDERER);
    char* glVersion  = (char*)glGetString(GL_VERSION);
    char* glExtensions = (char*)glGetString(GL_EXTENSIONS);

    extBGRA = CheckExtension("GL_EXT_bgra");

    //sprintf(userDisplayInfo[0].description,"OpenGL - Vendor: %s, Renderer: %s, Version: %s",glVendor,glRenderer,glVersion);

    return true;
}

extern pddiDisplayInfo displayInfo[2];

pddiDisplayInfo* pglDisplay::GetDisplayInfo(void)
{
    return &displayInfo[0];
}

unsigned pglDisplay::GetFreeTextureMem()
{
    return unsigned(-1);
}

unsigned pglDisplay::GetBufferMask()
{
    return ~0U;
}

int pglDisplay::GetHeight()
{
    return winHeight;
}

int pglDisplay::GetWidth()
{
    return winWidth;
}

int pglDisplay::GetDepth()
{
    return winBitDepth;
}

pddiDisplayMode pglDisplay::GetDisplayMode(void)
{
    return mode;
}

int pglDisplay::GetNumColourBuffer(void)
{
    return 2;
}

void pglDisplay::GetGamma(float* r, float* g, float* b)
{
    *r = gammaR;
    *g = gammaG;
    *b = gammaB;
}

void pglDisplay::SetGamma(float r, float g, float b)
{
    gammaR = r;
    gammaG = g;
    gammaB = b;
}

void pglDisplay ::SwapBuffers()
{
    glXSwapBuffers( display, window );
}

unsigned pglDisplay::Screenshot(pddiColour* buffer, int nBytes)
{
    // not implemented under linux
    assert(0 && "PDDI: pddiDisplay::ScreenShot() - Not implemented under linux.");
}

unsigned pglDisplay::FillDisplayModes(pddiModeInfo* displayModes)
{
    int i = 0;
    int nModes = 0;

    return nModes;
}
    
void pglDisplay::BeginTiming()
{
    gettimeofday(&beginTime,NULL);
}

float pglDisplay::EndTiming()
{
    struct timeval tv;
    gettimeofday(&tv,NULL);
    // return time in milliseconds
    return (float)(tv.tv_sec - beginTime.tv_sec) * 1e3 + (float)(tv.tv_usec - beginTime.tv_usec) * 1e-3;
}

void pglDisplay::BeginContext(void)
{
}

void pglDisplay::EndContext(void)
{
}
