//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _MIPFILTER_HPP
#define _MIPFILTER_HPP

#include <p3d/p3dtypes.hpp>

class tMipmapFilter
{
public:
    virtual void BuildMipmapLevel(unsigned* src, unsigned* dest, int width, int height) = 0;
};

//| 1 1 |
//| 1 1 |
class tMipmapFilter_1111 : public tMipmapFilter
{
public:
    virtual void BuildMipmapLevel(unsigned* src, unsigned* dest, int width, int height);
};

//| 4 1 |
//| 1 2 |
class tMipmapFilter_4112 : public tMipmapFilter
{
public:
    virtual void BuildMipmapLevel(unsigned* src, unsigned* dest, int width, int height);
};

//| 5 1 |
//| 1 1 |
class tMipmapFilter_5111 : public tMipmapFilter
{
public:
    virtual void BuildMipmapLevel(unsigned* src, unsigned* dest, int width, int height);
};

//| 1 1 |
//| 1 1 | alpha component < 128 is clamped to 0
class tMipmapFilter_1111AC : public tMipmapFilter
{
public:
    virtual void BuildMipmapLevel(unsigned* src, unsigned* dest, int width, int height);
};

//| 4 1 |
//| 1 2 | alpha component < 128 is clamped to 0
class tMipmapFilter_4112AC : public tMipmapFilter
{
public:
    virtual void BuildMipmapLevel(unsigned* src, unsigned* dest, int width, int height);
};

//| 5 1 |
//| 1 1 | alpha component < 128 is clamped to 0
class tMipmapFilter_5111AC : public tMipmapFilter
{
public:
    virtual void BuildMipmapLevel(unsigned* src, unsigned* dest, int width, int height);
};

#endif /* _MIPFILTER_HPP */

