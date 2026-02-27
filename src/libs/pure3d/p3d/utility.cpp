//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include <p3d/utility.hpp>
#include <p3d/fileftt.hpp>

#include <stdlib.h>
#include <string.h>
#include <typeinfo>

bool gFruitless;

namespace p3d
{

tPlatform*           platform;
tContext*            context;
tInventory*          inventory;
tMatrixStack*        stack;
tLoadManager*        loadManager;
pddiRenderContext*   pddi;
pddiDevice*          device;
pddiDisplay*         display;


// generate a human readable name for a type
bool ClassName(tEntity* e, char* buf, unsigned size)
{
    if(!e)
        return false;

    char n[256]; // temporary buffer to huld name
    char* name = n; // current position that we think the name starts

    // grab the base typeid string
    strcpy(name, typeid(*e).name());

    // remove leading whitespace
    while (*name == ' ')
        name++;

    // pull the 'class' off the beginning, if any
    if((*name == 'c') && (*(name+1) == 'l') && (*(name+2) == 'a') && (*(name+3) == 's') && (*(name+4) == 's'))
        name += 5;

    // suck back more whitespace
    while (*name == ' ')
        name++;

    // remove leading numbers (PS2)
    while((*name >= '0') && (*name <= '9'))
        name++;

    // remove leading namespace
    char* walk = name;
    while(*walk)
    {
        if((*walk == ':') && (*(walk+1) == ':'))
            name = walk+2;
        walk++;
    }

    // remove namespace 
    // (PS2, for some reason uses '10' as seperator, hopefully no one tries this on a class with '10' in the name)
    {
    char* walk = name;
    while(*walk)
    {
        if((*walk == '1') && (*(walk+1) == '0'))
            name = walk+2;
        walk++;
    }
    }


    // pull off Pure3D 't' prefix
    if(*name == 't')
        name++;

    // blast it to the destination buffer
    if(strlen(name) < (size - 1))
    {
        strcpy(buf, name);
        return true;
    }

    // buffer not big enough, fail
    buf[0] = 0;
    return false;
}

tFile* openFile(const char* name, bool sync)
{
    return new tFileFTT(name, sync);
}

#ifdef P3D_DEBUG_MESSAGES

void print(const char* s)
{
    #ifdef P3D_DEBUG_FTT_MESSAGES
    rDebugString(s);
    #else
    #if defined(RAD_WIN32) || defined(RAD_UWP)
       OutputDebugString(s);
    #else
       ::printf("%s", s);
    #endif
    #endif
}


// use this function just like printf
// e.g. p3d::printf("x = %f\n", fX);
// VC: Also, double clicking on  the line in the output window will take you
// to the calling print command in the source code.
// oct30/2001 amb - this function couldnt be inlined because use of va_args
void printf(const char* s, ...)
{
    va_list va_alist;
    char    printfstr[ 1024 ];

    va_start( va_alist, s ); 

    #if (defined RAD_WIN32 || defined RAD_UWP)
    _vsnprintf( printfstr, 1024, s, va_alist );
    #endif

    va_end( va_alist ); 

    print( printfstr );
};
#else
void printf( const char* s, ... )
{
    //no nothing in release
}
#endif // P3D_DEBUG_MESSAGES




} // namespace p3d

