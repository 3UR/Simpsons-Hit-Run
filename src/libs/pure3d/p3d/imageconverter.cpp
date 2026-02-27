//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include <p3d/imageconverter.hpp>
#include <p3d/utility.hpp>
#include <p3d/image.hpp>
#include <p3d/texture.hpp>

#include <string.h>

//-------------------------------------------------------------------
tImageConverter::tImageConverter()
{
    //
}

tImageConverter::~tImageConverter()
{
    //
}

//-------------------------------------------------------------------
tTexture* tImageConverter::ImageToTexture(tImage* image)
{
    P3DASSERT(image);
    tTexture* texture = new tTexture;
    texture->CopyName(image);
    bool alpha = image->HasAlpha();

    pddiTextureType requestedType = PDDI_TEXTYPE_RGB;

    if(image->GetDepth() == 8)
    {
        requestedType = PDDI_TEXTYPE_PALETTIZED;
    }

    if(!texture->Create(image->GetWidth(), image->GetHeight(), image->GetDepth(), image->GetAlphaDepth(), 0, requestedType, PDDI_USAGE_STATIC))
    {
        int lastError = p3d::device->GetLastError();

        switch(lastError)
        {
            case PDDI_OK:
            break;
            case PDDI_TEX_NOT_POW_2:
                P3DVERIFY(false, "Texture error: texture width and height are not powers of two, ", image->GetNameDangerous() );
                texture->Release();
                texture = NULL;
            break;
            case PDDI_TEX_TOO_BIG:
                P3DVERIFY(false, "Texture error: texture width or height is too large, ", image->GetNameDangerous() );
                texture->Release();
                texture = NULL;
            break;
            default:
                P3DVERIFY(false, "PDDI can't create texture: unknown error, ", image->GetNameDangerous() );
                texture->Release();
                texture = NULL;
            break;
        }
    }


    if(texture)
    {
        pddiTexture* pdditexture = texture->GetTexture();
        P3DASSERT(pdditexture);

        UpdateSurface(image, pdditexture->Lock(0));
        pdditexture->Unlock(0);

        tImage8* image8 = dynamic_cast<tImage8*>(image);
        if(image8 && ((pdditexture->GetPixelFormat() == PDDI_PIXEL_PAL8) ||
        (pdditexture->GetPixelFormat() == PDDI_PIXEL_PAL4)))
        {
            texture->SetPalette(256, (pddiColour*)image8->GetPalette());
        }
    }

    return texture;
}

//-------------------------------------------------------------------
tTexture* tImageConverter::ImageInToVolumeTexture(tImage** images, int numImages, tTexture* texture, int mipLevel)
{
#ifdef RAD_UWP
    P3DASSERT(texture);

    pddiTexture* pdditexture = texture->GetTexture();
    pddiLockInfo* lockInfo = pdditexture->Lock(mipLevel);
    for (int i = 0; i < numImages; i++)
    {
        UpdateVolumeTexture(images[i], lockInfo, i);
    }
    pdditexture->Unlock(mipLevel);

    return texture;
#else
    return NULL;
#endif
}

tTexture* tImageConverter::ImageToVolumeTexture(tImage** images, int numImages, int numMipMaps, pddiTextureUsageHint usage)
{
#ifdef RAD_UWP

    P3DASSERT(images);
    tTexture* texture = new tTexture;
    texture->CopyName(images[0]);

    bool alpha = images[0]->HasAlpha();


    int width = images[0]->GetWidth();
    int height = images[0]->GetHeight();
    int alphaDepth = images[0]->GetAlphaDepth();
    int bpp;
    pddiTextureType textureType;

    if(images[0]->GetDepth() == 8)
    {
        bpp = 8;
        textureType = PDDI_TEXTYPE_PALETTIZED;
    }
    else
    {
        bpp = 32;
        textureType = PDDI_TEXTYPE_RGB;
    }

    if(!texture->CreateVolume(width, height, numImages, bpp, alphaDepth, numMipMaps, textureType, usage))
    {
        int lastError = p3d::device->GetLastError();

        switch(lastError)
        {
            case PDDI_OK:
            break;
            case PDDI_TEX_NOT_POW_2:
                P3DVERIFY(false, "Texture error: texture width and height are not powers of two, ", images[0]->GetName() );
                texture->Release();
                texture = NULL;
            break;
            case PDDI_TEX_TOO_BIG:
                P3DVERIFY(false, "Texture error: texture width or height is too large, ", images[0]->GetName() );
                texture->Release();
                texture = NULL;
            break;
            default:
                P3DVERIFY(false, "PDDI can't create texture: unknown error, ", images[0]->GetName() );
                texture->Release();
                texture = NULL;
            break;
        }
    }

    pddiTexture* pdditexture = texture->GetTexture();

    if(texture)
    {
        pddiLockInfo* lockInfo = pdditexture->Lock(0);
        for (int i = 0; i < numImages; i++)
        {
            UpdateVolumeTexture(images[i], lockInfo, i);
        }
        pdditexture->Unlock(0);
        tImage8* image8 = dynamic_cast<tImage8*>(images[0]);
        if(image8)
        {
            texture->SetPalette(256, (pddiColour*)image8->GetPalette());
        }
    }

    return texture;
#else
    return NULL;
#endif
}

//-------------------------------------------------------------------
void tImageConverter::UpdateTexture(tImage* image, tTexture* texture, int mip)
{
    if(texture)
    {
        UpdateSurface(image, texture->GetTexture()->Lock(mip));
        texture->GetTexture()->Unlock(mip);

        tImage8* image8 = dynamic_cast<tImage8*>(image);
        if(image8)
        {
            texture->SetPalette(256, (pddiColour*)image8->GetPalette());
        }
    }
}

//-------------------------------------------------------------------
void tImageConverter::UpdateSurface(tImage* image, pddiLockInfo* lock)
{
    P3DASSERT(image);
    P3DASSERT(lock);
    pddiPixelFormat format = lock->format;
    
    // we can't update a texture with an image of a different size
    if( (image->GetWidth() != (lock->width)) ||
         (image->GetHeight() != (lock->height)) )
    {
        p3d::printf("Pure3D : tImageConverter::UpdateTexture() src size != dest size, image : %s\n", image->GetNameDangerous());
        return;
    }

    switch(format)
    {
        case PDDI_PIXEL_PAL8:         
            FillLockPAL8(image, lock);
         break;

        case PDDI_PIXEL_RGB565:
        case PDDI_PIXEL_RGB555:
        case PDDI_PIXEL_ARGB1555:
        case PDDI_PIXEL_ARGB4444:
            FillLockRGB16(image, lock);
        break;

        case PDDI_PIXEL_ARGB8888:
            FillLockRGB32(image, lock);
        break;

        default:
            P3DVERIFY(false, "Pixel format for texture is not supported, ", image->GetNameDangerous());
        break;
    }
}

//-------------------------------------------------------------------
void tImageConverter::UpdateVolumeTexture(tImage* image, pddiLockInfo* lock, int depth)
{
#ifdef RAD_UWP   
    P3DASSERT(image);
    P3DASSERT(lock);
    pddiPixelFormat format = lock->format;
    
    // we can't update a texture with an image of a different size
    if( (image->GetWidth() != (lock->width)) ||
         (image->GetHeight() != (lock->height)) )
    {
        p3d::printf("Pure3D : tImageConverter::UpdateVolumeTexture() src size != dest size, image : %s\n", image->GetName());
        return;
    }

    if (depth >= lock->volDepth)
    {
        p3d::printf("Pure3D : tImageConverter::UpdateVolumeTexture() depth > volume depth , image : %s", image->GetName());
        return;
    }

    void* origBits = lock->bits;
    lock->bits = (unsigned char*)lock->bits + lock->slice*depth;
    
    switch(format)
    {
        case PDDI_PIXEL_PAL8:
            FillLockPAL8(image, lock);
        break;

        case PDDI_PIXEL_RGB565:
        case PDDI_PIXEL_RGB555:
        case PDDI_PIXEL_ARGB1555:
        case PDDI_PIXEL_ARGB4444:
            FillLockRGB16(image, lock);
        break;

        case PDDI_PIXEL_ARGB8888:
            FillLockRGB32(image, lock);
        break;

        default:
            P3DVERIFY(false, "Pixel format for texture is not supported, ", image->GetName());
        break;
    }

    lock->bits = (unsigned char*)origBits;

#endif
}

//-------------------------------------------------------------------
void tImageConverter::FillLockPAL8(tImage* img, pddiLockInfo* lock)
{
    P3DASSERT(img->GetDepth() == 8);
    tImage8* image = (tImage8*)img;
    unsigned char* dest = (unsigned char*)lock->bits;   
    unsigned char* src = image->GetColourChannel();
    P3DASSERT(src);

    for (int j = 0; j < lock->height; j++)
    {
        memcpy(dest, src, lock->width);
        src += lock->width;
        dest += lock->pitch;
    }
}

//-------------------------------------------------------------------
static void ConvertImage32To16(pddiColour* src, P3D_U16* dest, pddiLockInfo* lock)
{
    int slop = (lock->pitch - lock->width*2) / 2;
    int y = lock->height;

    while(y)
    {
        int x = lock->width;
        while(x)
        {
            *dest = (P3D_U16)lock->MakeColour(*src);
            src++;
            dest++;
            x--;
        }
        dest += slop;
        y--;
    }
}

static void ConvertImageGenericTo16(tImage* image, P3D_U16* dest, pddiLockInfo* lock)
{
    int slop = (lock->pitch - lock->width*2) / 2;
    int y = 0;

    while(y < lock->height)
    {
        int x = 0;
        while(x < lock->width)
        {
            *dest++ = (P3D_U16)lock->MakeColour(image->GetColour(x, y));
            x++;
        }
        dest += slop;
        y++;
    }
}

//-------------------------------------------------------------------
static void ConvertImage32To32(pddiColour* src, P3D_U32* dest, pddiLockInfo* lock)
{
    int y = lock->height;

    if(lock->native)
    {
        char* buf = (char*)dest;
        while(y--)
        {
            memcpy(buf, src, lock->width*4);
            src += lock->width;
            buf += lock->pitch;
        }
    }
    else
    {
        int slop = (lock->pitch - lock->width*4) / 4;
        while(y)
        {
            int x = lock->width;
            while(x)
            {
                *dest = lock->MakeColour(*src);
                src++;
                dest++;
                x--;
            }
            dest += slop;
            y--;
        }
    }
}

//-------------------------------------------------------------------
static void ConvertImageGenericTo32(tImage* image, P3D_U32* dest, pddiLockInfo* lock)
{
    int slop = (lock->pitch - lock->width*4) / 4;
    int y = 0;

    while(y < lock->height)
    {
        int x = 0;
        while(x < lock->width)
        {
            *dest++ = (P3D_U32)lock->MakeColour(image->GetColour(x, y));
            x++;
        }
        dest += slop;
        y++;
    }
}

//-------------------------------------------------------------------
void tImageConverter::FillLockRGB16(tImage* image, pddiLockInfo* lock)
{
    P3D_U16* dest = (P3D_U16*)lock->bits;

    if(image->GetDepth() == 32)
    {
        pddiColour* src = (pddiColour*)image->GetColourChannel();
        ConvertImage32To16(src, dest, lock);
    }
    else
    {
        ConvertImageGenericTo16(image, dest, lock);
    }

}

//-------------------------------------------------------------------
void tImageConverter::FillLockRGB32(tImage* image, pddiLockInfo* lock)
{
    P3D_U32* dest = (P3D_U32*)lock->bits;

    if(image->GetDepth() == 32)
    {
        pddiColour* src = (pddiColour*)image->GetColourChannel();
        ConvertImage32To32(src, dest, lock);
    }
    else
    {
        ConvertImageGenericTo32(image, dest, lock);
    }
}


tImage* tImageConverter::LockToImage(pddiLockInfo* lock)
{
    tImage32* image = new tImage32;
    image->SetSize(lock->width, lock->height);
    UpdateImage(lock, image);
    return image;
}

void tImageConverter::UpdateImage(pddiLockInfo* lock, tImage* image)
{
    P3DASSERT(dynamic_cast<tImage32*>(image));
    P3DASSERT((lock->depth == 16) || (lock->depth == 32));

    int width = image->GetWidth();
    int height = image->GetHeight();
    int rFrac = lock->rgbaLShift[0] + ((8 - lock->rgbaRShift[0]) - lock->rgbaRShift[0]);
    int gFrac = lock->rgbaLShift[1] + ((8 - lock->rgbaRShift[1]) - lock->rgbaRShift[1]);
    int bFrac = lock->rgbaLShift[2] + ((8 - lock->rgbaRShift[2]) - lock->rgbaRShift[2]);

    switch(lock->depth)
    {
        case 16:
            {
                int stride = lock->pitch/2;
                unsigned short* buf = (unsigned short*)lock->bits;   
                unsigned* imgbits = (unsigned*)image->GetColourChannel();

                for(int y=0; y < height; y++)
                {
                    for(int x=0; x < width; x++)
                    {
                        unsigned pixel = buf[x];
                        unsigned ri = pixel & lock->rgbaMask[0];
                        unsigned r = ((ri >> lock->rgbaLShift[0]) << lock->rgbaRShift[0]) | (ri >> rFrac);
                        unsigned gi = pixel & lock->rgbaMask[1];
                        unsigned g = ((gi >> lock->rgbaLShift[1]) << lock->rgbaRShift[1]) | (gi >> gFrac);
                        unsigned bi = pixel & lock->rgbaMask[2];
                        unsigned b = ((bi >> lock->rgbaLShift[2]) << lock->rgbaRShift[2]) | (bi >> bFrac);
                        *imgbits++ = 0xff000000 | r | g | b;
                    }
                    buf += stride;
                }
            }
            break;

        case 32:
            {
                int stride = lock->pitch/4;
                unsigned* buf = (unsigned*)lock->bits;   
                unsigned* imgbits = (unsigned*)image->GetColourChannel();

                for(int y=0; y < height; y++)
                {
                    for(int x=0; x < width; x++)
                    {
                        unsigned pixel = buf[x];
                        unsigned ri = pixel & lock->rgbaMask[0];
                        unsigned r = ((ri >> lock->rgbaLShift[0]) << lock->rgbaRShift[0]) | (ri >> rFrac);
                        unsigned gi = pixel & lock->rgbaMask[1];
                        unsigned g = ((gi >> lock->rgbaLShift[1]) << lock->rgbaRShift[1]) | (gi >> gFrac);
                        unsigned bi = pixel & lock->rgbaMask[2];
                        unsigned b = ((bi >> lock->rgbaLShift[2]) << lock->rgbaRShift[2]) | (bi >> bFrac);
                        *imgbits++ = 0xff000000 | r | g | b;
                    }
                    buf += stride;
                }
            }
            break;

        default:
            break;
    }
}

/*

//-------------------------------------------------------------------
int tImageConverter::NumMipmapLevel(int width, int height)
{
    int nLOD = 0;
    int w = width; 
    int h = height;

    while(w>0 && h>0)
    {
        w >>= 1; h >>= 1;
        nLOD++;
    }
    return(nLOD);
}

//-------------------------------------------------------------------
int tImageConverter::ComputeTextureSize(int x, int y, int depth, int nMip, pddiTextureType type)
{
    int size = x*y*(depth/8);

    if(nMip > 0)
    {
        size = ((size*4)/3);
        size++;
    }
    return size;
}

*/

