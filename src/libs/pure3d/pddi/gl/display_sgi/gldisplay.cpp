//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include<gl.hpp>
#ifdef RAD_WIN32
    #include <pddi/gl/display_win32/gldisplay.hpp>
    #include <pddi/gl/display_win32/gl.hpp>
#endif

#ifdef RAD_LINUX
    #include <pddi/gl/display_linux/gldisplay.hpp>
    #include <pddi/gl/display_linux/gl.hpp>
#endif


#include<stdio.h>
#include<string.h>

static bool CheckExtension( char *extName )
{
    char *p = (char *) glGetString(GL_EXTENSIONS);
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

class pglSurface : public pddiSurface
{
    friend class pglDisplay;

public:
    pglSurface(int x, int y, pglDisplay* display);
    ~pglSurface();

    pddiPixelFormat GetPixelFormat(void) {return PDDI_PIXEL_ARGB8888;}
    int GetWidth(void) { return width;}
    int GetHeight(void) { return height;}
    int GetDepth(void) { return 32;}

    pddiLockInfo* Lock(pddiLockType lock = PDDI_LOCK_READWRITE);
    void Unlock(void);

private:
    pglDisplay* display;
    int width, height;
    unsigned* bits;
    pddiLockInfo lock;
};

pglDisplay ::pglDisplay()
{
    mode = PDDI_DISPLAY_WINDOW;
    winWidth = 640;
    winHeight = 480;
    winBitDepth = 32;

    context = NULL;

    extBGRA = false;
	m_ForceVSync = false;
}

pglDisplay ::~pglDisplay()
{
}

#define KEYPRESSED(x) (GetKeyState((x)) & (1<<(sizeof(int)*8)-1))

bool pglDisplay ::InitDisplay(int x, int y, int bpp)
{
    //hRC = wglCreateContext( hDC );
    //wglMakeCurrent( hDC, hRC );

    return InitDisplay(x, y, bpp, PDDI_DISPLAY_WINDOW, 2, PDDI_BUFFER_COLOUR);
}

bool pglDisplay ::InitDisplay(int x, int y, int bpp, pddiDisplayMode m, int colourBufferCount, unsigned bufferMask)
{
    mode = m;

    if(mode == PDDI_DISPLAY_FULLSCREEN)
    {
        winWidth = x;
        winHeight = y;
    }
    else
    {
        winWidth = x;
        winHeight = y;
    }

    char* glVendor   = (char*)glGetString(GL_VENDOR);
    char* glRenderer = (char*)glGetString(GL_RENDERER);
    char* glVersion  = (char*)glGetString(GL_VERSION);
    char* glExtensions = (char*)glGetString(GL_EXTENSIONS);

    extBGRA = CheckExtension("GL_EXT_bgra");

    //sprintf(userDisplayInfo[0].description,"OpenGL - Vendor: %s, Renderer: %s, Version: %s",glVendor,glRenderer,glVersion);

    return true;
}

unsigned pglDisplay ::GetFreeVideoMem()
{
    return unsigned(-1);
}

unsigned pglDisplay ::GetFreeTextureMem()
{
    return unsigned(-1);
}

unsigned pglDisplay ::GetBufferMask()
{
    return unsigned(-1);
}

int pglDisplay ::GetHeight()
{
    return winHeight;
}

int pglDisplay ::GetWidth()
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

void pglDisplay ::SwapBuffers(bool async)
{
    pddiDisplay::SwapBuffers();
}

    
bool pglDisplay::ReadPixels(pddiSurface* dest, pddiRect* srcRect)
{
    glReadPixels(srcRect->left, winHeight - srcRect->bottom,  dest->GetWidth(), dest->GetHeight(),
                     ((pglSurface*)dest)->lock.native ? GL_BGRA_EXT : GL_RGBA, GL_UNSIGNED_BYTE, ((pglSurface*)dest)->bits);  

    ((pglSurface*)dest)->lock.pitch = (((pglSurface*)dest)->width * 4);
    ((pglSurface*)dest)->lock.bits = ((pglSurface*)dest)->bits;
    return true;
}

bool pglDisplay::WritePixels(pddiSurface* src, pddiRect* destRect)
{
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glOrtho(0,GetWidth(),0,GetHeight(),0,-1);
    glViewport(0, 0, GetWidth(),GetHeight());

    glDisable(GL_SCISSOR_TEST);

/*   glScissor(destRect->left,display->GetHeight()-destRect->bottom, 
                 destRect->right - destRect->left + 1,destRect->bottom - destRect->top + 1);

    glEnable(GL_SCISSOR_TEST);*/
    glPixelZoom(float(destRect->right - destRect->left) / float(src->GetWidth()),
                    float(destRect->bottom - destRect->top) / float(src->GetHeight()));

    glRasterPos2f(float(destRect->left),winHeight-float(destRect->bottom));

    GLboolean olddepth;
    glGetBooleanv(GL_DEPTH_TEST,&olddepth);

    GLboolean oldalpha;
    glGetBooleanv(GL_ALPHA_TEST,&oldalpha);

    if(olddepth)
        glDisable(GL_DEPTH_TEST);

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_ALPHA_TEST);

    glDrawPixels(src->GetWidth(),src->GetHeight(),((pglSurface*)src)->lock.native ? GL_BGRA_EXT : GL_RGBA,GL_UNSIGNED_BYTE,((pglSurface*)src)->bits);

    if(olddepth)
        glEnable(GL_DEPTH_TEST);

    if(!oldalpha)
        glDisable(GL_ALPHA_TEST);
    else
        glEnable(GL_ALPHA_TEST);

    glEnable(GL_SCISSOR_TEST);
    glPixelZoom(1.0f,1.0f);

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    return true;
}

pddiSurface* pglDisplay ::NewSurface(int x, int y)
{
    return new pglSurface(x,y,this);
}

pglSurface::pglSurface(int x, int y, pglDisplay* d)
{
    display = d;

    width = x;
    height = y;
    bits = new unsigned[x*y];

    lock.width = x;
    lock.height = y;
    lock.pitch = -(x * 4);
    lock.bits = bits + (width*(height-1));
    lock.depth = 32;
    lock.format = PDDI_PIXEL_ARGB8888;

    if(d->ExtBGRA())
    {
        lock.native = true;
        lock.rgbaLShift[0] = lock.rgbaRShift[0] =
        lock.rgbaLShift[1] = lock.rgbaRShift[1] =
        lock.rgbaLShift[2] = lock.rgbaRShift[2] =
        lock.rgbaLShift[3] = lock.rgbaRShift[3] = 0;

        lock.rgbaMask[0] = 0x00ff0000;
        lock.rgbaMask[1] = 0x0000ff00;
        lock.rgbaMask[2] = 0x000000ff;
        lock.rgbaMask[3] = 0xff000000;
    }
    else
    {
        lock.native = false;
        lock.rgbaRShift[0] = 16;
        lock.rgbaLShift[2] = 16;

        lock.rgbaLShift[0] = 
        lock.rgbaLShift[1] = lock.rgbaRShift[1] =
        lock.rgbaRShift[2] =
        lock.rgbaLShift[3] = lock.rgbaRShift[3] = 0;

        lock.rgbaMask[0] = 0x000000ff;
        lock.rgbaMask[1] = 0x0000ff00;
        lock.rgbaMask[2] = 0x00ff0000;
        lock.rgbaMask[3] = 0xff000000;
    }
}

pglSurface::~pglSurface()
{
    delete bits;
}

pddiLockInfo*  pglSurface::Lock(pddiLockType l)
{
    return &lock;
}

void pglSurface::Unlock()
{
}

unsigned pglDisplay::FillDisplayModes(pddiModeInfo* displayModes)
{
    int i = 0;
    int nModes = 0;

    return nModes;
}
    


