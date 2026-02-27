//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _GLDISPLAY_HPP_
#define _GLDISPLAY_HPP_
#include <pddi/pddi.hpp>

class pglContext;
class pglWrapper;

class pglDisplay : public pddiDisplay
{
public:
    pglDisplay();
    ~pglDisplay();

    // cross-platform functions
    bool InitDisplay(int x, int y, int bpp, pddiDisplayMode mode, int colourBufferCount, unsigned bufferMask);
    bool InitDisplay(int x, int y, int bpp);

    int GetHeight(void);
    int GetWidth(void);
    int GetDepth(void);
    pddiDisplayMode GetDisplayMode(void);
    int GetNumColourBuffer(void);
    unsigned GetBufferMask(void);

    unsigned GetFreeVideoMem(void);
    unsigned GetFreeTextureMem(void);

    void SwapBuffers(bool async = true);
    
    bool ReadPixels(pddiSurface*, pddiRect* srcRect);
    bool WritePixels(pddiSurface*, pddiRect* destRect);

    pddiSurface* NewSurface(int x, int y);

    // linux specific functions

    // internal functions
    void SetContext(pglContext* c) {context = c;}
    bool ExtBGRA(void) { return extBGRA;}

    static unsigned FillDisplayModes(pddiModeInfo*);

private:
    pddiDisplayMode mode;
    int winWidth;
    int winHeight;
    int winBitDepth;

    pglContext* context;

    bool extBGRA;
};

// hack for now.  Ask Nigel how to fix this

#define GL_BGRA_EXT GL_RGBA

#endif

