//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _PDDIXBOX_HPP
#define _PDDIXBOX_HPP

#include <pddi/xbox/direct3d.hpp>
#include <pddi/pddienum.hpp>

class d3dContext;
class pddiBaseShader;

// machine dependent types
typedef __int64        PDDI_S64;
typedef __int64        PDDI_U64;
typedef int            PDDI_S32;
typedef unsigned       PDDI_U32;
typedef char           PDDI_S8;
typedef unsigned char  PDDI_U8;
typedef short          PDDI_S16;
typedef unsigned short PDDI_U16;

//-------------------------------------------------------------------
// PDDI DLL initialization
// pddiCreate() is the only function that needs to be imported by an
// application loading a PDDI DLL. This function will create a
// pddiDevice object for the particular renderer that the DLL
// supports.
//-------------------------------------------------------------------

class pddiDevice;

// prototype for the initialization function through implicit linking
extern "C" int pddiCreate(int versionMajor, int versionMinor, pddiDevice** dev);

// the xbox version of PDDI still uses fully virtualized interfaces (but shouldn't)
#define PDDI_INTERFACE virtual
#define PDDI_PURE = 0
#define PDDI_VIRTUAL

// display init
class pddiDisplayInit
{
public:
    enum AntiAlias 
    {
        NONE                 = 0x0011,
        MULTISAMPLE_2_LINEAR = 0x1021,
        MULTISAMPLE_2_QUINCUNX = 0x1121,
        SUPERSAMPLE_2_HORIZONTAL_LINEAR = 0x2021,
        SUPERSAMPLE_2_VERTICAL_LINEAR = 0x2012,
        MULTISAMPLE_4_NEAR = 0x1022,
        MULTISAMPLE_4_GAUSSIAN = 0x1222,
        SUPERSAMPLE_4_LINEAR = 0x2022,
        SUPERSAMPLE_4_GAUSSIAN = 0x2222,
        MULTISAMPLE_9_GAUSSIAN = 0x1233,
        SUPERSAMPLE_9_GAUSSIAN = 0x2233
    };

    enum PreFilter 
    { 
        DEFAULT  =                0x00000,
        X1R5G5B5 =                0x10000, 
        R5G6B5   =                0x20000, 
        X8R8G8B8 =                0x30000, 
        A8R8G8B8 =                0x40000 
    };

    pddiDisplayInit()
    {
        xsize = 640;
        ysize = 480;
        bpp = 32;
        bufferMask = PDDI_BUFFER_COLOUR | PDDI_BUFFER_DEPTH | PDDI_BUFFER_STENCIL;;
        nColourBuffer = 2;
        antiAlias = NONE;
        preFilter = DEFAULT;
        lockToVsync = false;
        asyncSwap = true;
        enableSnapshot = false;
        allowHDTV = false;
        allowWidescreen = false;
    }

    int               xsize;         // x resolution (fullscreen only)
    int               ysize;         // y resolution (fullscreen only)
    int               bpp;           // bits per pixel
    int               nColourBuffer; // number of frame buffers
    unsigned          bufferMask;    // auxilliary buffer mask

    AntiAlias         antiAlias;
    PreFilter         preFilter;

    bool lockToVsync;
    bool asyncSwap;
    bool enableSnapshot;
    bool allowHDTV;
    bool allowWidescreen;
};

//-------------------------------------------------------------------
// pddiPrimStream - immediate more rendering
//-------------------------------------------------------------------
//-------------------------------------------------------------------

#include <pddi/xbox/primstream.hpp>
#define pddiPrimStream d3dPrimStream

//-------------------------------------------------------------------
// types and defines for non-console PDDI libraries
// assumes OpenGL-style rendering

// alpha blending modes
enum pddiBlendFactor
{
    PDDI_BF_ZERO,
    PDDI_BF_ONE,
    PDDI_BF_SRC,
    PDDI_BF_ONE_MINUS_SRC,
    PDDI_BF_DEST,
    PDDI_BF_ONE_MINUS_DEST,
    PDDI_BF_SRCALPHA,
    PDDI_BF_ONE_MINUS_SRCALPHA,
    PDDI_BF_DESTALPHA,
    PDDI_BF_ONE_MINUS_DESTALPHA,
    PDDI_BF_SRCALPHASATURATE
};

class pddiBlendController : public pddiObject
{
public:
    virtual void SetBlend(pddiBlendFactor src, pddiBlendFactor dest) = 0;
    virtual void GetBlend(pddiBlendFactor* src, pddiBlendFactor* dest) = 0;
};

#endif 

