//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include <p3d/image.hpp>
#include <p3d/imagefactory.hpp>
#include <constants/chunkids.hpp>
#include <p3d/chunkfile.hpp>
#include <p3d/file.hpp>
#include <p3d/inventory.hpp>
#include <p3d/error.hpp>
#include <p3d/utility.hpp>

#include <string.h>

// return the average of four colours
unsigned p3dAverageRGBA4(unsigned c1, unsigned c2, unsigned c3, unsigned c4);

// functions used by tImage::CopyRegion()
static void p3dBlockCopy(unsigned char* src, unsigned char* dest, int width, int height, int srcStride, int destStride);
static void ImageBlockCopy8(tImage8* srcImg, unsigned char* destBase, int x0, int y0, int width, int height, int destStride);
static void ImageBlockCopy8Mask(tImage8* srcImg, unsigned char* destBase, int x0, int y0, int width, int height, int destStride, unsigned char mask);
static void BlockCopy32Mask(unsigned* src, unsigned* dest, int width, int height, int srcStride, int destStride, unsigned mask);
static void ImageBlockCopy32(tImage* src, unsigned* dest, int x0, int y0, int width, int height, int destStride);
static void ImageBlockCopy32Mask(tImage* src, unsigned* destBase, int x0, int y0, int width, int height, int destStride, unsigned mask);

tImage::tImage()
{
    image = NULL;
    width = 0;
    height = 0;
    depth = 0;
    key = 0;
    keyEnabled = false;
    alphaDepth = 8;
    hasAlpha = false;
}

tImage::tImage(tImage* img)
{
    CopyName(img);
    image = NULL;
    alphaDepth = img->alphaDepth;
    key = img->GetColourKey();
    keyEnabled = img->GetColourKeyStatus();
    width = img->GetWidth();
    height = img->GetHeight();
}

tImage::~tImage()
{
    delete [] image;
}

void tImage::SetSize(int x, int y)
{
    width = x;
    height = y;
}

void tImage::CopyRegion(tImage* src, int dx, int dy)
{
    CopyRegion(src, 0, 0, src->GetWidth(), src->GetHeight(), dx, dy);
}

void tImage::Invert(void)
{
    unsigned byteDepth = depth >> 3;
    unsigned char* tmp = (unsigned char*)p3d::MallocTemp(width*byteDepth);

    unsigned char* pix = image;
    unsigned char* pix2 = image;
    unsigned step = byteDepth * width;
    pix2 += step * (height - 1);

    for(int i = 0; i < (height >> 1); i++)
    {
        memcpy(tmp, pix2, step);
        memcpy(pix2, pix, step);
        memcpy(pix, tmp, step);
        pix += step;
        pix2 -= step;
    }

    p3d::FreeTemp(tmp);
}

//-------------------------------------------------------------------
tImage8::tImage8()
{
    depth = 8;
    memset(palette, 0xff000000, sizeof(unsigned)*256);
    usedColours = 256;
}

tImage8::tImage8(tImage* img) : tImage(img)
{
    depth = 8;
    memset(palette, 0xff000000, sizeof(unsigned)*256);

    SetSize(img->GetWidth(), img->GetHeight());
    bool saveCC = GetColourKeyStatus();
    EnableColourKey(false);
    tImage::CopyRegion(img, 0, 0);
    EnableColourKey(saveCC);

    tImage8* img8 = dynamic_cast<tImage8*>(img);
    usedColours = img8 ? img8->GetUsedColours() : 256;
}

tImage8::~tImage8()
{
    /* */
}

void tImage8::SetPixelIndex(int x, int y, unsigned index)
{
    *(image + ((width*y)+x)) = (unsigned char)index;
}

unsigned tImage8::GetPixelIndex(int x, int y)
{
    unsigned char index = (unsigned char)(*(image + ((width*y)+x)));
    return (unsigned)index;
}

void tImage8::SetImage(int w, int h, unsigned char *newImage)
{
    SetSize(w,h);   
    memcpy(image,newImage,sizeof(unsigned char)*w*h);
}

void tImage8::SetSize(int x, int y)
{
    width = x;
    height = y;

    delete [] image;
    image = new unsigned char[width*height];
}

void tImage8::ReSize(int x, int y)
{
    int size = sizeof(unsigned char);
    int minWidth = (width < x) ? width : x;
    int minHeight = (height < y) ? height : y;
    unsigned char *newImage, *newTemp, *oldTemp;

    newImage = new unsigned char[x*y];
    memset (newImage,0,size*x*y);
    if (image)
    {
        newTemp = newImage;
        oldTemp = image;
        minWidth *= size;

        for (int i = 0; i < minHeight; i++)
        {
            memcpy(newTemp,oldTemp,minWidth);
            newTemp+=size*x;
            oldTemp+=size*width;
        }   

        width = x;
        height = y;
        delete [] image;
    }
    image = newImage;
}

unsigned tImage8::GetColour(int x, int y)
{
    unsigned char index = *(image + ((width*y)+x));
    return palette[index];
}

void tImage8::SetColour(int x, int y, unsigned colour)
{
    *(image + ((width*y)+x)) = (unsigned char)colour;
}

unsigned* tImage8::GetPalette()
{
    return palette;
}

void tImage8::SetPalette(unsigned* pal, int nColours)
{
    if(nColours != 256)
        memset(palette, 0, sizeof(unsigned)*256);

    memcpy(palette, pal, sizeof(unsigned)*nColours);
}

void tImage8::SetPaletteEntry(int idx, unsigned colour)
{
    P3DASSERT(idx < 256);
    palette[idx] = colour;
}

unsigned tImage8::GetPaletteEntry(int idx)
{
    P3DASSERT(idx < 256);
    return palette[idx];
}

void tImage8::CopyRegion(tImage* srcImg, int sx, int sy, int sw, int sh, int dx, int dy)
{
    P3DASSERT(srcImg->GetDepth() == 8);
    P3DASSERT((sx >= 0) && (sx+sw <= srcImg->GetWidth()));
    P3DASSERT((sy >= 0) && (sy+sh <= srcImg->GetHeight()));
    // clip the blit
    if(dx < 0) 
    {
        sx += dx;
        sw -= dx;
        dx = 0;
    }
    if(dy < 0)
    {
        sy += dy;
        sh -= dy;
        dy = 0;
    }
    if(dx+sw > width)
    {
        sw -= ((dx+sw)-width);
    }
    if(dy+sh > height)
    {
        sh -= ((dy+sh)-height);
    }
    if((sw < 1) || (sh < 1)) return;   
    
    if(srcImg->GetColourChannel() && image)
    {
        // copy colour channel
        unsigned char* dest = image;
        dest += width*dy + dx;
        if(srcImg->GetColourKeyStatus())
        {
            ImageBlockCopy8Mask((tImage8*)srcImg, dest, sx, sy, sw, sh, width, (unsigned char)srcImg->GetColourKey());
        }
        else
        {
            ImageBlockCopy8((tImage8*)srcImg, dest, sx, sy, sw, sh, width);
        }
    }
}

void tImage8::Clear(unsigned colour)
{
    memset(image,colour,height*width);
}

//-------------------------------------------------------------------
tImage32::tImage32()
{
    depth = 32;
}

tImage32::tImage32(tImage* img) : tImage(img)
{
    depth = 32;
    SetSize(img->GetWidth(), img->GetHeight());
    bool saveCC = GetColourKeyStatus();
    EnableColourKey(false);
    tImage::CopyRegion(img, 0, 0);
    EnableColourKey(saveCC);
}

tImage32::~tImage32()
{
    /* */
}

void tImage32::SetImage(int w, int h, unsigned char *newImage)
{
    SetSize(w,h);   
    memcpy(image,newImage,sizeof(unsigned char)*w*h*4);
}

void tImage32::SetSize(int x, int y)
{
    width = x;
    height = y;
    delete [] image;
    image = new unsigned char[width*height*4];
}

void tImage32::ReSize(int x, int y)
{
    int size = sizeof(unsigned char) * 4;
    int minWidth = (width < x) ? width : x;
    int minHeight = (height < y) ? height : y;
    unsigned char *newImage, *newTemp, *oldTemp;

    newImage = new unsigned char[x*y*4];
    memset (newImage,0,size*x*y);
    if (image)
    {
        newTemp = newImage;
        oldTemp = image;
        minWidth *= size;

        for (int i = 0; i < minHeight; i++)
        {
            memcpy(newTemp,oldTemp,minWidth);
            newTemp+=size*x;
            oldTemp+=size*width;
        }   

        width = x;
        height = y;
        delete[] image;
    }
    image = newImage;
}

unsigned tImage32::GetColour(int x, int y)
{
    return *((unsigned*)image + ((width*y)+x));
}

void tImage32::SetColour(int x, int y, unsigned colour)
{
    *((unsigned*)image + ((width*y)+x)) = colour;
}

//-------------------------------------------------------------------
void tImage32::CopyRegion(tImage* srcImg, int sx, int sy, int sw, int sh, int dx, int dy)
{   
    hasAlpha = srcImg->HasAlpha();
    P3DASSERT((sx >= 0) && (sx+sw <= srcImg->GetWidth()));
    P3DASSERT((sy >= 0) && (sy+sh <= srcImg->GetHeight()));
    // clip the blit
    if(dx < 0) 
    {
        sx += dx;
        sw -= dx;
        dx = 0;
    }
    if(dy < 0)
    {
        sy += dy;
        sh -= dy;
        dy = 0;
    }
    if(dx+sw > width)
    {
        sw -= ((dx+sw)-width);
    }
    if(dy+sh > height)
    {
        sh -= ((dy+sh)-height);
    }
    if((sw < 1) || (sh < 1)) return;

    if(srcImg->GetColourChannel() && image)
    {
        if(srcImg->GetDepth() == 32)
        {
            // copy colour channel (fast 32 bit copy)
            unsigned* src = (unsigned*)srcImg->GetColourChannel();
            src += srcImg->GetWidth()*sy + sx;
            unsigned* dest = (unsigned*)image;
            dest += width*dy + dx;
            int srcPitch = srcImg->GetWidth();
            if(srcImg->GetColourKeyStatus())
            {
                BlockCopy32Mask(src, dest, sw, sh, srcPitch, width, srcImg->GetColourKey());
            }
            else
            {
                p3dBlockCopy((unsigned char*)src, (unsigned char*)dest, sw*4, sh, srcPitch*4, width*4);
            }
        }
        else
        {
            // copy colour channel (slower generic copy)
            unsigned* dest = (unsigned*)image;
            dest += width*dy + dx;
            if(srcImg->GetColourKeyStatus())
            {
                ImageBlockCopy32Mask(srcImg, dest, sx, sy, sw, sh, width, srcImg->GetColourKey());
            }
            else
            {
                ImageBlockCopy32(srcImg, dest, sx, sy, sw, sh, width);
            }
        }
    }
}

void tImage32::Clear(unsigned colour)
{
    unsigned* pix = (unsigned*)image;

    for(int i = 0; i < height*width; i++)
        *pix++ = colour;
}

//-------------------------------------------------------------------
// utility functions for tImage::CopyRegion()
void p3dBlockCopy(unsigned char* src, unsigned char* dest, int width, int height, int srcStride, int destStride)
{
    for(int i=0; i < height; i++)
    {
        memcpy(dest, src, width);
        src += srcStride;
        dest += destStride;
    }
}

void ImageBlockCopy8(tImage8* srcImg, unsigned char* destBase, int x0, int y0, int width, int height, int destStride)
{
    unsigned char* srcBase = srcImg->GetColourChannel();
    srcBase += srcImg->GetWidth()*y0 + x0;
    int srcStride = srcImg->GetWidth();
    unsigned int* palette = srcImg->GetPalette();
    int x1 = x0 + width;
    int y1 = y0 + height;
    for(int y=y0; y < y1; y++)
    {
        unsigned char* src = srcBase;
        unsigned char* dest = destBase;
        for(int x=x0; x < x1; x++)
        {
 //        if ((palette[*src] >> 24)>0)
            {
                *dest = *src;
            }
            src++;
            dest++;
        }
        srcBase += srcStride;
        destBase += destStride;
    }
}

void ImageBlockCopy8Mask(tImage8* srcImg, unsigned char* destBase, int x0, int y0, int width, int height, int destStride, unsigned char mask)
{
    unsigned char* srcBase = srcImg->GetColourChannel();
    srcBase += srcImg->GetWidth()*y0 + x0;
    int srcStride = srcImg->GetWidth();
    unsigned int* palette = srcImg->GetPalette();
    int x1 = x0 + width;
    int y1 = y0 + height;
    for(int y=y0; y < y1; y++)
    {
        unsigned char* src = srcBase;
        unsigned char* dest = destBase;
        for(int x=x0; x < x1; x++)
        {
            if(*src != mask) 
            {
//            if ((palette[*src] >> 24)>0)
                {
                    *dest = *src;
                }
            }
            src++;
            dest++;
        }
        srcBase += srcStride;
        destBase += destStride;
    }
}

void BlockCopy32Mask(unsigned* srcBase, unsigned* destBase, int width, int height, int srcStride, int destStride, unsigned mask)
{
    for(int i=0; i < height; i++)
    {
        unsigned* src = srcBase;
        unsigned* dest = destBase;
        for(int j=0; j < width; j++)
        {   
            if((*src & 0x00ffffff) != mask) *dest = *src;
            src++;
            dest++;
        }
        srcBase += srcStride;
        destBase += destStride;
    }
}

void ImageBlockCopy32(tImage* src, unsigned* destBase, int x0, int y0, int width, int height, int destStride)
{
    int x1 = x0 + width;
    int y1 = y0 + height;
    for(int y=y0; y < y1; y++)
    {
        unsigned* dest = destBase;
        for(int x=x0; x < x1; x++)
        {
            tColour srcColour(src->GetColour(x,y));
//         if (srcColour.Alpha()==255)
            {
                *dest = srcColour;
            }
            /*
            else if (srcColour.Alpha()>0)
            {
                tColour destColour(*dest);
                int red = (((srcColour.Red() - destColour.Red()) * srcColour.Alpha()) >> 8) + destColour.Red();
                int green = (((srcColour.Green() - destColour.Green()) * srcColour.Alpha()) >> 8) + destColour.Green();
                int blue = (((srcColour.Blue() - destColour.Blue()) * srcColour.Alpha()) >> 8) + destColour.Blue();
                int alpha = srcColour.Alpha() + destColour.Alpha();
                *dest = tColour(red, green, blue, alpha>255 ? 255 : alpha);
            }*/
            dest++;
        }
        destBase += destStride;
    }
}

void ImageBlockCopy32Mask(tImage* src, unsigned* destBase, int x0, int y0, int width, int height, int destStride, unsigned mask)
{
    int x1 = x0 + width;
    int y1 = y0 + height;
    for(int y=y0; y < y1; y++)
    {  
        unsigned* dest = destBase;
        for(int x=x0; x < x1; x++)
        {
            unsigned colour = src->GetColour(x, y);
            if((colour & 0x00ffffff) != mask)
            {
                tColour srcColour(colour);
                if (srcColour.Alpha()==255)
                {
                    *dest = srcColour;
                }
                else if (srcColour.Alpha()>0)
                {
                    tColour destColour(*dest);
                    int red = (((srcColour.Red() - destColour.Red()) * srcColour.Alpha()) >> 8) + destColour.Red();
                    int green = (((srcColour.Green() - destColour.Green()) * srcColour.Alpha()) >> 8) + destColour.Green();
                    int blue = (((srcColour.Blue() - destColour.Blue()) * srcColour.Alpha()) >> 8) + destColour.Blue();
                    int alpha = srcColour.Alpha() + destColour.Alpha();
                    *dest = tColour(red, green, blue, alpha>255 ? 255 : alpha);
                }
            }
            dest++;
        }
        destBase += destStride;
    }
}

//-------------------------------------------------------------------
static const int IMAGE_VERSION = 14000;

tImageLoader::tImageLoader() : tSimpleChunkHandler(Pure3D::Texture::IMAGE), depth(32)
{
    factory = new tImageFactory();
}

tImageLoader::~tImageLoader() 
{
    delete factory;
}


tEntity* tImageLoader::LoadObject(tChunkFile* file, tEntityStore*)
{
    return LoadImage(file, depth);
}

tImage* tImageLoader::LoadImage(tChunkFile* f, int depth /*=32*/)
{
    tImage* image = NULL;

    char name[128];
    f->GetPString(name);
    int version = f->GetLong();
    P3DASSERT(version == IMAGE_VERSION);
    int width = f->GetLong();
    int height = f->GetLong();
    int bpp = f->GetLong();
    bool palettized = f->GetLong() == 1;
    bool alpha = f->GetLong() == 1;
    unsigned format = f->GetLong();

    factory->SetDesiredDepth(bpp);

    while(f->ChunksRemaining())
    {
        switch(f->BeginChunk())
        {
            case Pure3D::Texture::IMAGE_DATA:
            {
                unsigned size = f->GetLong();
                tFile* file = f->BeginInset();
                image = factory->ParseAsImage(file, name, (tImageHandler::Format)format);
                f->EndInset(file);
                break;
            }

            case Pure3D::Texture::IMAGE_FILENAME:
            {
                char fileName[255];
                f->GetPString(fileName);
                image = factory->LoadAsImage(fileName, name);
                break;
            }

            default:
                break;
        }
        f->EndChunk();
    }

    if(image)
    {
        image->SetAlpha(alpha);
    }
    return image;
}

void tImageLoader::LoadIntoSurface(tChunkFile*, pddiLockInfo* lock)
{
    //
}

