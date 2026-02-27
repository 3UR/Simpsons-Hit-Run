//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _IMAGECONVERTER_HPP
#define _IMAGECONVERTER_HPP

#include <p3d/p3dtypes.hpp>
#include <pddi/pddienum.hpp>

class pddiSurface;
class pddiTexture;
struct pddiLockInfo;
class tTexture;
class tImage;
class tImage32;
class tImage8;

class tImageConverter
{
public:
    tImageConverter();
    virtual ~tImageConverter();

    virtual tTexture*    ImageToTexture(tImage* image);
    virtual tTexture*    ImageToVolumeTexture(tImage** image, int numImages, int numMipMaps = 0, pddiTextureUsageHint usage = PDDI_USAGE_STATIC);
    virtual tTexture*    ImageInToVolumeTexture(tImage** image, int numImages, tTexture* texture, int mipLevel = 0);
    virtual tImage*      LockToImage(pddiLockInfo*);

    virtual void UpdateTexture(tImage* image, tTexture* texture, int mip = 0);
    virtual void UpdateVolumeTexture(tImage* image, pddiLockInfo* lock, int depth);
    virtual void UpdateSurface(tImage* image, pddiLockInfo* lock);
    virtual void UpdateImage(pddiLockInfo* lock, tImage* image);

private:

    void FillLockPAL8(tImage* img, pddiLockInfo* lock);
    void FillLockRGB16(tImage* image, pddiLockInfo* lock);
    void FillLockRGB32(tImage* image, pddiLockInfo* lock);
    void FillImage32(pddiLockInfo* lock, tImage32* image);
    void FillImage8(pddiLockInfo* lock, tImage8* image);
};

#endif /* _IMAGECONV_HPP */


