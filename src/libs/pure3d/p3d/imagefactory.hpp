//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _IMAGEFACTORY_HPP
#define _IMAGEFACTORY_HPP

#include <p3d/p3dtypes.hpp>
#include <p3d/loadmanager.hpp>
#include <pddi/pddienum.hpp>

class tFile;
class tImage;
class tTexture;
class tImageConverter;

class tImageHandler : public tFileHandler
{
public:
    enum Format
    {
        IMG_RAW,
        IMG_PNG,
        IMG_TGA,
        IMG_BMP,
        IMG_IPU,
        IMG_DXT,
        IMG_DXT1,
        IMG_DXT2,            //not used  
        IMG_DXT3,            
        IMG_DXT4,            //not used
        IMG_DXT5,
        IMG_PS2_4BIT,        // PS2 Memory image formats
        IMG_PS2_8BIT,
        IMG_PS2_16BIT,
        IMG_PS2_32BIT
    };

    class Builder
    {
    public:
        enum Origin { TOP, BOTTOM };
        virtual bool BeginImage(int width, int height, int bpp, Origin origin, pddiColour* palette) = 0;
        virtual void ProcessScanline32(unsigned* src) = 0;
        virtual void ProcessScanline8(unsigned char* src) = 0;
        virtual void DirectCopy( unsigned char* data, int len )  {;}
        virtual void* GetMemoryImagePtr() { return NULL; }
        virtual void* GetPaletteMemoryImagePtr() { return NULL; }
        virtual void SetCompressedData( int mipmap, char* data, int len ) {;}
        virtual void SetExpectedFileSize( const int size )       {;}
        virtual int  GetExpectedFileSize()                       {return -1;}
        virtual void SetTextureType( pddiTextureType type )      {;}
        virtual void EndImage() = 0;
    };

    tImageHandler();

    enum LoadType { IMAGE, TEXTURE, SPRITE };

    virtual bool CanLoad() = 0;
    virtual bool CanSave() = 0;
    virtual const char* GetExtension() = 0;

    virtual bool CheckExtension(char* filename);
    virtual bool CheckFormat(Format) = 0;

    virtual void CreateImage(tFile* data, Builder* builder) = 0;

    virtual bool SaveImage(tImage* image, char* filename) { return false; }

    virtual void SetLoadType(LoadType l) { loadType = l; }
    virtual LoadType GetLoadType(void)   { return loadType; }

    virtual void SetFullName(bool b) { fullName = b;}
    virtual bool GetFullName(void  ) { return fullName;}

    virtual tLoadStatus Load(tFile* file, tEntityStore* store);
    void SetNativeResolution( const int nativeX, const int nativeY );


protected:
    LoadType loadType;
    bool fullName;
    int m_NativeX;
    int m_NativeY;
};

//-------------------------------------------------------------------
class tImageFactory
{
    public:
        tImageFactory();
        ~tImageFactory();

        tImage* LoadAsImage(char* filename, char* inventoryName=NULL);
        tImage* LoadAsImage(tFile*, char* inventoryName=NULL);
        tImage* ParseAsImage(tFile*, char* inventoryName, tImageHandler::Format);
        tTexture* LoadAsTexture(char* filename, char* inventoryName=NULL);
        tTexture* ParseAsTexture( tFile*, char* inventoryName, const int size, tImageHandler::Format);
        void LoadIntoTexture(char* filename, tTexture* texture, int mipLevel = 0);
        void ParseIntoTexture(tFile* file, tTexture* texture, tImageHandler::Format, int mipLevel = 0);

        bool SaveImage(tImage* image, char* filename);

        void SetIgnoreExtension(bool ignore) { ignoreExt = ignore; }
        bool GetIgnoreExtension() const      { return ignoreExt; }

        void SetAutoStore(bool store)        { autoStore = store; }
        bool GetAutoStore(void) const        { return autoStore; }

        void SetAlpha(bool alpha)            { hasAlpha = alpha; }
        bool GetAlpha(void) const            { return hasAlpha; }

        void SetDesiredDepth(int bpp)        { desiredDepth = bpp; }
        int GetDesiredDepth() const          { return desiredDepth; }

        void SetTextureHints(int alphaDepth, int nMip, pddiTextureType type, pddiTextureUsageHint hint);

        tImageConverter* GetConverter()      { return converter; }

        void AddHandler(tImageHandler* handler);
        void ClearHandlers();

    private:
        int nHandler;
        tImageHandler* handler[32];
        tImageConverter *converter;
        bool ignoreExt;
        bool autoStore;
        bool hasAlpha;
        int desiredDepth;
        bool fullName;

        int alphaDepthHint;
        int nMipHint;
        pddiTextureType typeHint;
        pddiTextureUsageHint usageHint;

        tImageHandler* FindHandler(char* filename);
        tImageHandler* FindHandler(tImageHandler::Format);

        tImageHandler* OpenImage(char* filename, tFile** file);
};

#endif /* _IMAGEFACTORY_HPP */

