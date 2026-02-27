/*===========================================================================
    pdditype.hpp
    Created by Neall

    Copyright (c)1999 Radical Entertainment, Inc.
    All rights reserved.
===========================================================================*/

#ifndef _PDDITYPE_HPP
#define _PDDITYPE_HPP

#include <pddi/buildconfig.hpp>
#include <radmath/radmath.hpp>

#ifndef NULL
    #define NULL (0)
#endif

#ifdef WIN32
    #define DLLEXPORT __declspec(dllexport)
#else
    #define DLLEXPORT 
#endif

#define PDDI_STRING(x) #x
#define PDDI_STRINGIZE(x) PDDI_STRING(x)

//-------------------------------------------------------------------
// pddiObject
//-------------------------------------------------------------------
// Base class for other pddiObjects
class pddiObject
{
public:
    pddiObject();
    virtual void AddRef();
    virtual void Release();
    virtual int GetLastError();

protected:
    virtual ~pddiObject();

    int refCount;
    int lastError;
};

// 32 bit colour
struct DLLEXPORT pddiColour
{
    pddiColour() { /**/ }
    pddiColour(int r, int g, int b, int a = 0xff)
    {
        c = (unsigned)b | ((unsigned)g<<8) | ((unsigned)r<<16) | ((unsigned)a<<24);
    }
              
    void Set(int r, int g, int b, int a = 0xff)
    {
        c = (unsigned)b | ((unsigned)g<<8) | ((unsigned)r<<16) | ((unsigned)a<<24);
    }

    explicit pddiColour(unsigned colour)
    {
        c = colour;
    }

    operator unsigned() { return c; }

    void SetRed(int r)   { c = (c & 0xff00ffff) | ((r << 16) & 0x00ff0000) ; }
    void SetGreen(int g) { c = (c & 0xffff00ff) | ((g << 8) & 0x0000ff00) ; }
    void SetBlue(int b)  { c = (c & 0xffffff00) | ((b) & 0x000000ff) ; }
    void SetAlpha(int a) { c = (c & 0x00ffffff) | ((a << 24) & 0xff000000) ; }
    
    int Red() const   { return (int) ((c & 0x00ff0000) >> 16); }
    int Green() const { return (int) ((c & 0x0000ff00) >> 8); }
    int Blue() const  { return (int) ((c & 0x000000ff)); }
    int Alpha() const { return (int) ((c & 0xff000000) >> 24); }
    int Intensity() const { return (Red() + Green() + Blue()) / 3; }

    unsigned c;
};

typedef rmt::Vector pddiVector;
typedef rmt::Vector4 pddiVector4;
typedef rmt::Vector2 pddiVector2;
typedef rmt::Matrix pddiMatrix;

class DLLEXPORT pddiRect
{
public:
    pddiRect() {}
    pddiRect(int l, int t, int r, int b) { left = l; top = t; right = r; bottom = b; }
    int left;
    int top;
    int right;
    int bottom;

    void Set(int l, int t, int r, int b) { left = l; top = t; right = r; bottom = b; }
};

// system-dependent includes
#if defined(RAD_WIN32) || defined(RAD_UWP)
    #include <pddi/pddiwin32.hpp>
#endif

#ifdef RAD_LINUX
    #include <pddi/pddilinux.hpp>
#endif

#endif /* _PDDITYPE_HPP */