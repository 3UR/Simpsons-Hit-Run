//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================



#include <p3d/mipmapfilter.hpp>
#include <pddi/pdditype.hpp>

//-------------------------------------------------------------------
// | 1 1 |
// | 1 1 |
class Filter_1111
{
public:
    static unsigned Filter(unsigned c1, unsigned c2, unsigned c3, unsigned c4)
    {
        c1 = (c1 & 0xfefefefe) >> 1;
        c2 = (c2 & 0xfefefefe) >> 1;
        c3 = (c3 & 0xfefefefe) >> 1;
        c4 = (c4 & 0xfefefefe) >> 1;
        unsigned ca = c1+c2;
        unsigned cb = c3+c4;
        ca = (ca & 0xfefefefe) >> 1;
        cb = (cb & 0xfefefefe) >> 1;
        return ca+cb;
    }
};

//-------------------------------------------------------------------
// | 4 1 |
// | 1 2 |
class Filter_4112
{
public:
    static unsigned Filter(pddiColour c1, pddiColour c2, pddiColour c3, pddiColour c4)
    {
        /*
        unsigned r = ((c1.r<<2) + c2.r + c3.r + (c4.r<<1)) >> 3;
        unsigned g = ((c1.g<<2) + c2.g + c3.g + (c4.g<<1)) >> 3;
        unsigned b = ((c1.b<<2) + c2.b + c3.b + (c4.b<<1)) >> 3;
        unsigned a = ((c1.a<<2) + c2.a + c3.a + (c4.a<<1)) >> 3;
        return P3DRGBA(r,g,b,a);*/
        return c1;
    }
};

//-------------------------------------------------------------------
// | 5 1 |
// | 1 1 |
class Filter_5111
{
public:
    static unsigned Filter(pddiColour c1, pddiColour c2, pddiColour c3, pddiColour c4)
    {
/*      unsigned r = ((c1.r + (c1.r<<2)) + c2.r + c3.r + c4.r) >> 3;
        unsigned g = ((c1.g + (c1.g<<2)) + c2.g + c3.g + c4.g) >> 3;
        unsigned b = ((c1.b + (c1.b<<2)) + c2.b + c3.b + c4.b) >> 3;
        unsigned a = ((c1.a + (c1.a<<2)) + c2.a + c3.a + c4.a) >> 3;
        return P3DRGBA(r,g,b,a);*/
        return c1;
    }
};

//-------------------------------------------------------------------
template <class T> void FilterImage(unsigned* src, unsigned* dest, int width, int height)
{
    int w = width / 2;
    int h = height / 2;

    for(int y=0; y < h; y++)
    {
        for(int x=0; x < w; x++)
        {
            *dest++ = T::Filter(*(pddiColour*)src, *(pddiColour*)(src+1), *(pddiColour*)(src+width), *(pddiColour*)(src+width+1));
            src += 2;
        }
        src += width;
    }
}

//-------------------------------------------------------------------
template <class T> void FilterImageAC(unsigned* src, unsigned* dest, int width, int height)
{
    int w = width / 2;
    int h = height / 2;

    for(int y=0; y < h; y++)
    {
        for(int x=0; x < w; x++)
        {
            *dest = T::Filter(*(pddiColour*)src, *(pddiColour*)(src+1), *(pddiColour*)(src+width), *(pddiColour*)(src+width+1));
            if((*dest & 0xff000000) < 0x7f000000) *dest &= 0x00ffffff;
            dest++;
            src += 2;
        }
        src += width;
    }
}

//-------------------------------------------------------------------
void tMipmapFilter_1111::BuildMipmapLevel(unsigned* src, unsigned* dest, int width, int height)
{
    FilterImage <Filter_1111>(src, dest, width, height);   
}

//-------------------------------------------------------------------
void tMipmapFilter_4112::BuildMipmapLevel(unsigned* src, unsigned* dest, int width, int height)
{
    FilterImage <Filter_4112>(src, dest, width, height);   
}

//-------------------------------------------------------------------
void tMipmapFilter_5111::BuildMipmapLevel(unsigned* src, unsigned* dest, int width, int height)
{
    FilterImage <Filter_5111>(src, dest, width, height);   
}

//-------------------------------------------------------------------
void tMipmapFilter_1111AC::BuildMipmapLevel(unsigned* src, unsigned* dest, int width, int height)
{
    FilterImageAC <Filter_1111>(src, dest, width, height);
}

//-------------------------------------------------------------------
void tMipmapFilter_4112AC::BuildMipmapLevel(unsigned* src, unsigned* dest, int width, int height)
{
    FilterImageAC <Filter_4112>(src, dest, width, height);
}

//-------------------------------------------------------------------
void tMipmapFilter_5111AC::BuildMipmapLevel(unsigned* src, unsigned* dest, int width, int height)
{
    FilterImageAC <Filter_5111>(src, dest, width, height);
}

