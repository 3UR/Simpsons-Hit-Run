//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include <p3d/imagefactory.hpp>
#include <p3d/context.hpp>
#include <p3d/inventory.hpp>
#include <p3d/texture.hpp>
#include <p3d/image.hpp>
#include <p3d/imageconverter.hpp>
#include <p3d/file.hpp>
#include <p3d/error.hpp>
#include <p3d/utility.hpp>
#include <p3d/sprite.hpp>
#include <p3d/targa.hpp>
#include <p3d/png.hpp>
#include <p3d/bmp.hpp>
#include <p3d/dxtn.hpp>
#include <p3d/rawimage.hpp>

#include <SDL2/SDL.h>

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

//-------------------------------------------------------------------
class ImageBuilder : public tImageHandler::Builder
{
public:
    ImageBuilder();
    virtual bool BeginImage(int width, int height, int bpp, tImageHandler::Builder::Origin origin, pddiColour* palette);
    virtual void ProcessScanline32(unsigned* src);
    virtual void ProcessScanline8(unsigned char* src);
    virtual void EndImage();


    void SetDesiredDepth(int bpp) { desiredDepth = bpp; }
    tImage* GetImage()      { return image; }

private:
    tImage* image;
    unsigned* palette;
    unsigned char* scanline;
    int totalScanline;
    int stride;
    int desiredDepth;
};

ImageBuilder::ImageBuilder() :
    image(NULL), palette(NULL), scanline(NULL), totalScanline(0), stride(0), desiredDepth(0)
{
  //
}

bool ImageBuilder::BeginImage(int width, int height, int bpp, tImageHandler::Builder::Origin origin, pddiColour* p)
{
    int pixelWidth = 1;

    if((desiredDepth > 8) || (bpp > 8))
    {
        image = new tImage32;
        pixelWidth = 4;
    }
    else
    {
        image = new tImage8;
        ((tImage8*)image)->SetPalette((unsigned int*)p, 1 << bpp);
        ((tImage8*)image)->SetUsedColours(1 << bpp);
    }

    image->SetSize(width, height);
    image->SetAlpha(true); // HACK!
    image->SetAlphaDepth((bpp == 32) ? 8 : 1); // another HACK!;
 
    scanline = image->GetColourChannel();
    totalScanline = 0;
    stride = width * pixelWidth;

    if(origin == tImageHandler::Builder::BOTTOM)
    {
        // set up for a bottom-up bitmap
        scanline += (pixelWidth * width * (height-1));
        stride = -stride;
    }
    palette = (unsigned*)p;

    return true;
}

void ImageBuilder::ProcessScanline32(unsigned* src)
{
    P3DASSERT(totalScanline < image->GetHeight());
    memcpy(scanline, src, image->GetWidth()*4);
    scanline += stride;
    totalScanline++;
}

void ImageBuilder::ProcessScanline8(unsigned char* src)
{
    P3DASSERT(totalScanline < image->GetHeight());
    
    if(image->GetDepth() == 8)
    {
        memcpy(scanline, src, image->GetWidth());
    }
    else
    {
        int width = image->GetWidth();
        for(int x=0; x < width; x++)
        {
            *((unsigned*)&scanline[x*4]) = palette[src[x]];
        }
    }
    scanline += stride;
    totalScanline++;
}

void ImageBuilder::EndImage()
{
    //
}

//-------------------------------------------------------------------
class TextureBuilder : public tImageHandler::Builder
{
public:
    TextureBuilder();
    virtual bool BeginImage(int width, int height, int bpp, tImageHandler::Builder::Origin origin, pddiColour* palette);
    virtual void ProcessScanline32(unsigned* src);
    virtual void ProcessScanline8(unsigned char* src);
    virtual void EndImage();
    virtual void DirectCopy( unsigned char* data, int len );
    virtual void* GetMemoryImagePtr();
    virtual void* GetPaletteMemoryImagePtr();
    void SetCompressedData( int mipmap, char* data, int len );
    virtual void SetTextureType( pddiTextureType texType ){ type = texType; };

    void SetDesiredDepth(int bpp)                 { desiredDepth = bpp; }
    void SetAlphaDepth(int alphaDepthHint)        { alphaDepth = alphaDepthHint; }
    void SetMipCount(int nMipHint)                { nMip = nMipHint; }
    void SetUsage(pddiTextureUsageHint usageHint) { usage = usageHint; }

    tTexture* GetTexture()  { return texture; }

    void SetCurrentMipLevel(int level) { currentMipLevel = level; }

    void SetTexture(tTexture* t) { texture = t; }

private:
    tTexture* texture;
    unsigned* palette;
    pddiLockInfo* lockInfo;
    int stride;
    unsigned char* scanline;
    int totalScanline;
    int currentMipLevel;

    
    int alphaDepth;
    int nMip;
    pddiTextureType type;
    pddiTextureUsageHint usage;

    int desiredDepth;
    

    void CreateTexture(int width, int height, int bpp, tImageHandler::Builder::Origin origin, pddiColour* p);
};

TextureBuilder::TextureBuilder() :
    texture(NULL), palette(NULL), lockInfo(NULL), stride(0), scanline(NULL), totalScanline(0), currentMipLevel(0),
    alphaDepth(8), nMip(0), type(PDDI_TEXTYPE_RGB), usage(PDDI_USAGE_STATIC), desiredDepth(0)
{
  //
}

void TextureBuilder::CreateTexture(int width, int height, int bpp, tImageHandler::Builder::Origin origin, pddiColour* p)
{

    if(texture)
    {
        texture->Release();
    }

    texture = new tTexture;

    if(desiredDepth == 16)
    {
        texture->Create(width, height, 16, alphaDepth, nMip, PDDI_TEXTYPE_RGB, usage);
    }
    else
    if((desiredDepth > 8) || (bpp > 8))
    {
        texture->Create(width, height, 32, alphaDepth, nMip, PDDI_TEXTYPE_RGB, usage);
    }
    else
    if(bpp == 8)
    {
        texture->Create(width, height, 8, alphaDepth, nMip, PDDI_TEXTYPE_PALETTIZED, usage);
        texture->SetPalette(256, p);
    }
    else
    if(bpp == 4)
    {
        texture->Create(width, height, 4, alphaDepth, nMip, PDDI_TEXTYPE_PALETTIZED, usage);
        texture->SetPalette(16, p);
    }
    else
    {
        P3DASSERT(0);
    }

    if(texture->GetTexture())
    {
        int lastError = p3d::device->GetLastError();
        switch(lastError)
        {
            case PDDI_OK:
            break;
            case PDDI_TEX_NOT_POW_2:
                P3DVERIFY(false, "Texture error: texture width and height are not powers of two, ", texture->GetNameDangerous());
            break;
            case PDDI_TEX_TOO_BIG:
                P3DVERIFY(false, "Texture error: texture width or height is too large, ", texture->GetNameDangerous());
            break;
            default:
                P3DVERIFY(false, "PDDI can't create texture: unknown error, ", texture->GetNameDangerous());
            break;
        }
    }

}

bool TextureBuilder::BeginImage(int width, int height, int bpp, tImageHandler::Builder::Origin origin, pddiColour* p)
{
    if (texture == NULL)
    {
        texture = new tTexture;

        //RGB and palettized textures
        if( type == PDDI_TEXTYPE_RGB || type == PDDI_TEXTYPE_PALETTIZED )
        {
             if(desiredDepth == 16)
             {
                 texture->Create(width, height, 16, alphaDepth, nMip, PDDI_TEXTYPE_RGB, usage);
             }
             else
             if((desiredDepth > 8) || (bpp > 8))
             {
                 texture->Create(width, height, 32, alphaDepth, nMip, PDDI_TEXTYPE_RGB, usage);
             }
             else
             if(bpp == 8)
             {
                 texture->Create(width, height, 8, alphaDepth, nMip, PDDI_TEXTYPE_PALETTIZED, usage);
                 texture->SetPalette(256, p);
             }
             else
             if(bpp == 4)
             {
                 texture->Create(width, height, 4, alphaDepth, nMip, PDDI_TEXTYPE_PALETTIZED, usage);
                 texture->SetPalette(16, p);
             }
             else
             {
                 P3DASSERT(0);
             }
        }
        else
        if( type == PDDI_TEXTYPE_DXT1 || type == PDDI_TEXTYPE_DXT2 ||
             type == PDDI_TEXTYPE_DXT3 || type == PDDI_TEXTYPE_DXT4 ||
             type == PDDI_TEXTYPE_DXT5)
        {
             texture->Create(width, height, 2, 0, nMip, type, usage);
        }
        else
        // ps2 memory image formats
        if( (type == PDDI_TEXTYPE_PS2_4BIT) ||  (type == PDDI_TEXTYPE_PS2_8BIT) ||
             (type == PDDI_TEXTYPE_PS2_16BIT) || (type == PDDI_TEXTYPE_PS2_32BIT) )
        {
            texture->Create(width, height, bpp, 8, nMip, type, usage);
        }
    }

    if(!texture->GetTexture())
    {
        int lastError = p3d::device->GetLastError();
        switch(lastError)
        {
            case PDDI_OK:
            break;
            case PDDI_TEX_NOT_POW_2:
                P3DVERIFY(false, "Texture error: texture width and height are not powers of two, ", texture->GetNameDangerous());
                texture->Release();
                texture = NULL;
            break;
            case PDDI_TEX_TOO_BIG:
                P3DVERIFY(false, "Texture error: texture width or height is too large, ", texture->GetNameDangerous());
                texture->Release();
                texture = NULL;
            break;
            case PDDI_TEX_BADFORMAT:
                P3DVERIFY(false, "Texture error: no suitible texture formats availible, ", texture->GetNameDangerous());
                texture->Release();
                texture = NULL;
            break;
            default:
                P3DVERIFY(false, "PDDI can't create texture: unknown error, ", texture->GetNameDangerous());
                texture->Release();
                texture = NULL;
            break;
        }
    }

    // Bail on bad texture creation
    if (texture == NULL) return false;

    lockInfo = texture->Lock(currentMipLevel);

    scanline = (unsigned char*)lockInfo->bits;
    totalScanline = 0;
 
    stride = lockInfo->pitch;

    if(origin == tImageHandler::Builder::BOTTOM)
    {
        // set up for a bottom-up bitmap
        scanline += (stride * (height-1));
        stride = -stride;
    }
    palette = (unsigned*)p;
    return true;
}

void TextureBuilder::ProcessScanline32(unsigned* src)
{
    // Bail on bad texture creation
    if (texture == NULL) return;

    P3DASSERT(totalScanline < lockInfo->height);
    if(lockInfo->depth == 16)
    {
        P3D_U16* buf = (P3D_U16*)scanline;
        for(int i=0; i < lockInfo->width; i++)
        {
            *buf++ = lockInfo->MakeColour(*src++);
        }
    }
    else
    if(lockInfo->depth == 32)
    {
        if(lockInfo->native)
        {
            memcpy(scanline, src, lockInfo->width*4);
        }
        else
        {
            P3D_U32* buf = (PDDI_U32*)scanline;
            for(int i=0; i < lockInfo->width; i++)
            {
                *buf++ = lockInfo->MakeColour(*src++);
            }
        }
    }
    else
    {
        P3DASSERT(0);
    }

    scanline += stride;
    totalScanline++;
}

void TextureBuilder::ProcessScanline8(unsigned char* src)
{
    // Bail on bad texture creation
    if (texture == NULL) return;

    P3DASSERT(totalScanline < lockInfo->height);
  
    if(lockInfo->depth == 16)
    {
        // depalettize image
        P3D_U16* buf = (P3D_U16*)scanline;
        for(int i=0; i < lockInfo->width; i++)
        {
            *buf++ = lockInfo->MakeColour(palette[*src++]);
        }
    }
    else
    if(lockInfo->depth == 32)
    {
        P3D_U32* buf = (PDDI_U32*)scanline;
        for(int i=0; i < lockInfo->width; i++)
        {
            *buf++ = lockInfo->MakeColour(palette[*src++]);
        }
    }
    else
    if(lockInfo->depth == 8)
    {
        memcpy(scanline, src, lockInfo->width);
    }
    else
    if(lockInfo->depth == 4)
    {
        unsigned char* buf = scanline;
        for(int i=0; i < lockInfo->width; i += 2)
        {
            *buf = (*src & 0xf) | (*(src+1)<<4);
            buf++;
            src += 2;
        }
    }
    else
    {
        P3DASSERT(0);
    }

    scanline += stride;
    totalScanline++;
}

void TextureBuilder::EndImage()
{
    // Bail on bad texture creation
    if (texture == NULL) return;

    texture->Unlock(currentMipLevel);
}

//Copy the texture data to the buffer directly
//this one is called by DXTn format, since it is compressed
//and will supported by DirextX8 directly, we don't need
//set any info such as pallete, for the texture at all
void TextureBuilder::DirectCopy( unsigned char *data, int len )
{
    // Bail on bad texture creation
    if (texture == NULL)
        return;

    memcpy( scanline, data, len );
}

//===========================================================================
// TextureBuilder::SetCompressedData
//===========================================================================
// Description: assigns the compressed data to the texture
//
// Constraints: None
//
// Parameters:  mipmap - mipmap level that this data corresponds to
//              data - pointer to the compressed data
//              len - length of the compressed data
//
// Return:      Nothing
//
//===========================================================================
void TextureBuilder::SetCompressedData( int mipmap, char* data, int len )
{
     pddiTexture* texture = this->texture->GetTexture();
}


// get pointer to raw lock info for memory image loading
void* TextureBuilder::GetMemoryImagePtr()
{
    P3DASSERT(scanline != NULL);
    P3DASSERT(texture != NULL);
    return scanline;
}

void* TextureBuilder::GetPaletteMemoryImagePtr()
{
    P3DASSERT(scanline != NULL);
    P3DASSERT(texture != NULL);
    return lockInfo->palette;
}

//-------------------------------------------------------------------
bool tImageHandler::CheckExtension(char* ext)
{
    const char* handler = GetExtension();

    int len = strlen(ext);
    int handlerLen = strlen(handler);

    if(len < handlerLen)
        return false;

    if(SDL_strcasecmp(handler, &ext[len-handlerLen]) == 0)
        return true;
  
    return false;
}

tLoadStatus tImageHandler::Load(tFile* file, tEntityStore* store)
{
    char name[256];
    if(fullName)
    {
        strcpy(name, file->GetFullFilename());
    }
    else
    {
        strcpy(name, file->GetFilename());
        int len = strlen(name);
        for(int i=len-1; i > 0; i--)
        {
            if(name[i] == '.')
            {
                name[i] = 0;
                break;
            }
        }
    }

    tLoadStatus status = LOAD_ERROR;

    if(loadType == TEXTURE)
    {
        TextureBuilder builder;
        CreateImage(file, &builder);
        tTexture* texture = builder.GetTexture();
        if(texture)
        {
            texture->SetName(name);
            store->Store(texture);
            status = LOAD_OK;
        }
    }
    else
    if(loadType == IMAGE)
    {
        ImageBuilder builder;
        CreateImage(file, &builder);
        tImage* image = builder.GetImage();
        if(image)
        {
            image->SetName(name);
            store->Store(image);
            status = LOAD_OK;
        }
    }
    else
    if(loadType == SPRITE)
    {
        ImageBuilder builder;
        CreateImage(file, &builder);
        tImage* image = builder.GetImage();
        if(image)
        {
            tSprite* sprite = new tSprite( image, NULL, 1, this->m_NativeX, this->m_NativeY );
            sprite->SetName(name);
            store->Store(sprite);
            image->Release();
            status = LOAD_OK;
        }
    }

    return status;
}

//===========================================================================
// tImageHandler::SetNativeResolution
//===========================================================================
// Description: allows you to set the resolution used for creation of 
//              sprites.  This should be set to the screen resolution if you 
//              want 1:1 pixel correspondance between art and screen
//              
//
// Constraints: this should be the same as the screen resolution
//
// Parameters:  nativeX - the x resolution of the screen
//              nativeY - the y resolution of the screen
//
// Return:      NONE
//
//===========================================================================
void tImageHandler::SetNativeResolution( const int nativeX, const int nativeY )
{
     this->m_NativeX = nativeX;
     this->m_NativeY = nativeY;
}

//-------------------------------------------------------------------
// tImageFactory
tImageFactory::tImageFactory() :
    nHandler(0), ignoreExt(false), autoStore(false), hasAlpha(false), desiredDepth(32),
    alphaDepthHint(8), nMipHint(0), typeHint(PDDI_TEXTYPE_RGB), usageHint(PDDI_USAGE_STATIC)
{
    converter = new tImageConverter;

    AddHandler(new tPNGHandler);
    AddHandler(new tBMPHandler);
    AddHandler(new tTargaHandler); 
    AddHandler(new tRawImageHandler);
#if defined(RAD_WIN32) || defined(RAD_UWP)
    AddHandler( new tDXTNHandler );
#endif
}

tImageFactory::~tImageFactory()
{
    delete converter;
    for(int i=0; i < nHandler; i++)
    {
        handler[i]->Release();
    }
}         

void tImageFactory::SetTextureHints(int alphaDepth, int nMip, pddiTextureType type, pddiTextureUsageHint usage)
{
    alphaDepthHint = alphaDepth;
    nMipHint = nMip;
    typeHint = type;
    usageHint = usage;
}
    
tImageHandler* tImageFactory::OpenImage(char* filename, tFile** file)
{
    tImageHandler* handler = NULL;
    tFile* local_file = p3d::openFile(filename);

    if(!local_file)
    {
        return NULL;
    }

//   if(!ignoreExt)
    {
        handler = FindHandler(filename);
    }
    /*
    else
    {
        handler = FindHandler(local_file);
    }
    */

    *file = local_file;
    return handler;
}

tImage* tImageFactory::LoadAsImage(tFile* file, char* inventoryName)
{
    file->AddRef();
    const char* filename = file->GetFilename();
    tImageHandler* handler = FindHandler(const_cast<char*>(filename));

    if(!handler)
    {
        file->Release();
        return NULL;
    }

    ImageBuilder builder;
    builder.SetDesiredDepth(desiredDepth);
    handler->CreateImage(file, &builder);
    tImage* result = builder.GetImage();

    if(result)
    {
        inventoryName ? result->SetName(inventoryName) : result->SetName(file->GetFullFilename());
        if(autoStore)
        {
            p3d::inventory->Store(result);
        }
     }

    file->Release();
    return result;
}

tImage* tImageFactory::LoadAsImage(char* filename, char* inventoryName)
{
    tFile* file = p3d::openFile(filename);
    tImage* image = NULL;
    if(file)
    {
        file->AddRef();
        image = LoadAsImage(file, inventoryName);
        file->Release();
    }
    return image;
}

tImage* tImageFactory::ParseAsImage(tFile* file, char* inventoryName, tImageHandler::Format format)
{
    tImageHandler* handler = FindHandler(format);
    if(!handler)
    {
        return NULL;
    }

    ImageBuilder builder;
    builder.SetDesiredDepth(desiredDepth);
    handler->CreateImage(file, &builder);
    tImage* result = builder.GetImage();

    if(result)
    {
        inventoryName ? result->SetName(inventoryName) : result->SetName(file->GetFilename());
        if(autoStore)
        {
            p3d::inventory->Store(result);
        }
     }

    return result;
}

tTexture* tImageFactory::LoadAsTexture(char* filename, char* inventoryName)
{
    tFile* file = p3d::openFile(filename);
    if(file)
    {
        file->AddRef();

        tImageHandler* handler = FindHandler(filename);

        if(!handler)
        {
            return NULL;
        }

        TextureBuilder builder;
        builder.SetDesiredDepth(desiredDepth);
        builder.SetAlphaDepth(alphaDepthHint);
        builder.SetMipCount(nMipHint);
        builder.SetTextureType(typeHint);
        builder.SetUsage(usageHint);
        builder.SetExpectedFileSize( file->GetSize() );
        handler->CreateImage(file, &builder);
        tTexture* texture = builder.GetTexture();

        if(texture)
        {
            inventoryName ? texture->SetName(inventoryName) : texture->SetName(file->GetFilename());
            if(autoStore)
            {
                p3d::inventory->Store(texture);
            }
        }

        file->Release();
        return texture;
    }
    return NULL;
}

tTexture* tImageFactory::ParseAsTexture( tFile* file, char* inventoryName, const int size, tImageHandler::Format format)
{
    tImageHandler* handler = FindHandler(format);

    if(!handler)
    {
        return NULL;
    }

    TextureBuilder builder;
    builder.SetDesiredDepth(desiredDepth);
    builder.SetAlphaDepth(alphaDepthHint);
    builder.SetMipCount(nMipHint);
    builder.SetTextureType(typeHint);
    builder.SetUsage(usageHint);
    builder.SetExpectedFileSize( size );
    handler->CreateImage(file, &builder);
    tTexture* texture = builder.GetTexture();

    if(texture)
    {
        inventoryName ? texture->SetName(inventoryName) : texture->SetName(file->GetFilename());
        if(autoStore)
        {
            p3d::inventory->Store(texture);
        }
    }

    return texture;
}

void tImageFactory::LoadIntoTexture(char* filename, tTexture* texture, int mipLevel)
{
    tFile* file = p3d::openFile(filename);
    if(file)
    {
        file->AddRef();

        tImageHandler* handler = FindHandler(filename);
        if(!handler)
        {
            return;
        }

        TextureBuilder builder;

        builder.SetTexture(texture);

        builder.SetCurrentMipLevel(mipLevel);

        handler->CreateImage(file, &builder);

        file->Release();
    }
}

void tImageFactory::ParseIntoTexture(tFile* file, tTexture* texture, tImageHandler::Format format, int mipLevel)
{
    tImageHandler* handler = FindHandler(format);
    if(!handler)
    {
        return;
    }

    TextureBuilder builder;

    builder.SetTexture(texture);

    builder.SetCurrentMipLevel(mipLevel);

    handler->CreateImage(file, &builder);
}

bool tImageFactory::SaveImage(tImage* image, char* filename)
{
    tImageHandler* handler = FindHandler(filename);
    if(!handler)
    {
        return false;
    }

    if(!handler->CanSave())
    {
        return false;
    }

    return handler->SaveImage(image, filename);
}

tImageHandler* tImageFactory::FindHandler(char* filename)
{
    tImageHandler* ret = NULL;
    for(int i=0; i < nHandler; i++)
    {
        if(handler[i]->CheckExtension(filename))
        {
            ret = handler[i];
            break;
        }
    }
    return ret;
}

tImageHandler* tImageFactory::FindHandler(tImageHandler::Format format)
{
    tImageHandler* ret = NULL;
    for(int i=0; i < nHandler; i++)
    {
        if(handler[i]->CheckFormat(format))
        {
            ret = handler[i];
            break;
        }
    }
    return ret;
}

void tImageFactory::AddHandler(tImageHandler* h)
{
    P3DASSERT(nHandler < 32);
    h->AddRef();
    handler[nHandler] = h;
    nHandler++;
}

void tImageFactory::ClearHandlers()
{
    nHandler = 0;
}

tImageHandler::tImageHandler()
:
     loadType( IMAGE ),
     fullName( false ),
     m_NativeX( p3d::display->GetWidth() ),
     m_NativeY( p3d::display->GetHeight() )
{
}

