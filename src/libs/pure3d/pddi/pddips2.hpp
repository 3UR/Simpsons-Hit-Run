//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _PDDIPS2_HPP
#define _PDDIPS2_HPP

#include <pddi/pddienum.hpp>

class pddiDevice;
class ps2Context;
class ps2Shader;

// prototype for the initialization function through implicit linking
extern "C" int pddiCreate(int versionMajor, int versionMinor, pddiDevice** dev);

// prototype for the initialization function through LoadLibrary
typedef int (*PDDICREATEPROC)(int, int, pddiDevice**);

// machine dependent types
typedef long           PDDI_S64;
typedef unsigned long  PDDI_U64;
typedef int            PDDI_S32;
typedef unsigned       PDDI_U32;
typedef char           PDDI_S8;
typedef unsigned char  PDDI_U8;
typedef short          PDDI_S16;
typedef unsigned short PDDI_U16;

#define PDDI_INTERFACE virtual
#define PDDI_PURE =0
#define PDDI_VIRTUAL virtual

// display initialization structure
class pddiDisplayInit
{
public:
    int      xsize;          // x resolution (y is chosen automatically) 
    int      pal;            // set to true to enable PAL refresh rate
    int      dtv480;
    int      lockToVsync;    // should we wait for vsync to do framebuffer swap
    int      asyncSwap;      // does framebuffer swap happen asyncronously (implies lockToVsync)
    int      pad0,pad1,pad2; // padding to make it 32bytes
};

#include <pddi/ps2/ps2streamprim.hpp>

#define pddiPrimStream ps2PrimStream

#endif /* _PDDIPS2_HPP */

