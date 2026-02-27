//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _PDDIWIN32_HPP
#define _PDDIWIN32_HPP

#include <cstdint>
#include <pddi/pddienum.hpp>

// machine dependent types
typedef int64_t  PDDI_S64;
typedef uint64_t PDDI_U64;
typedef int32_t  PDDI_S32;
typedef uint32_t PDDI_U32;
typedef int8_t   PDDI_S8;
typedef uint8_t  PDDI_U8;
typedef int16_t  PDDI_S16;
typedef uint16_t PDDI_U16;

//-------------------------------------------------------------------
// PDDI DLL initialization
// pddiCreate() is the only function that needs to be imported by an
// application loading a PDDI DLL. This function will create a
// pddiDevice object for the particular renderer that the DLL
// supports.
//-------------------------------------------------------------------

class pddiDevice;

// prototype for the initialization function through implicit linking
int pddiCreate(int versionMajor, int versionMinor, pddiDevice** dev);

// prototype for the initialization function through LoadLibrary
typedef int (*PDDICREATEPROC)(int, int, pddiDevice**);

// the Win32 version of PDDI uses fully virtualized interfaces
#define PDDI_INTERFACE virtual
#define PDDI_PURE = 0
#define PDDI_VIRTUAL

class pddiDisplayInit
{
public:
    pddiDisplayInit()
    {
        xsize = 640;
        ysize = 480;
        bpp = 32;
        bufferMask = PDDI_BUFFER_COLOUR | PDDI_BUFFER_DEPTH | PDDI_BUFFER_STENCIL;;
        nColourBuffer = 2;
        nSamples = 0;
        displayMode = PDDI_DISPLAY_WINDOW;
        lockToVsync = false;
        asyncSwap = true;
        enableSnapshot = false;
    }

    int xsize;         // x resolution (fullscreen only)
    int ysize;         // y resolution (fullscreen only)
    int bpp;           // bits per pixel
    unsigned bufferMask;
    int nColourBuffer;
    int nSamples;
    pddiDisplayMode displayMode;
    bool lockToVsync;
    bool asyncSwap;
    bool enableSnapshot;
};

#include <pddi/pddipc.hpp>


#endif /* _PDDIWIN32_HPP */