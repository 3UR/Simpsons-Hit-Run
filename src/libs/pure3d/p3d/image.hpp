//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _IMAGE_HPP
#define _IMAGE_HPP

#include <p3d/entity.hpp>
#include <p3d/loadmanager.hpp>

struct pddiLockInfo;
class tImageFactory;

class tImage : public tEntity
{
public:
    tImage();
    tImage(tImage* img);

    virtual void SetSize(int width, int height) = 0;
    virtual void SetImage(int width, int height, unsigned char* newImage) = 0;
    virtual void ReSize(int width, int height) = 0;

    virtual pddiLockInfo* Lock() = 0;

    virtual unsigned GetColour(int x, int y) = 0;
    
    virtual void SetColour(int x, int y, unsigned colour) = 0;
    void PutColour(int x, int y, unsigned colour) { SetColour(x, y, colour); }

    unsigned char* GetColourChannel() { return image; }

    virtual void Clear(unsigned colour) = 0;

    void CopyRegion(tImage* src, int dx, int dy);
    virtual void CopyRegion(tImage* src, int sx, int sy, int width, int height, int dx, int dy) = 0;

    /*
    // TODO
    void PasteRegion(tImage* src, int dx, int dy);
    virtual void PasteRegion(tImage* src, int sx, int sy, int width, int height, int dx, int dy) = 0;
    */

    void Invert(void);
    
    void EnableColourKey(bool enable)  { keyEnabled = enable; }
    bool GetColourKeyStatus()          { return keyEnabled; }
    void SetColourKey(unsigned colourKey) { key = colourKey; keyEnabled = true; }
    unsigned GetColourKey()               { return key; }

    void SetAlpha(bool a)              { hasAlpha = a; }
    bool HasAlpha()                    { return hasAlpha; }

    void SetAlphaDepth(int depth)      { alphaDepth = depth; }
    int GetAlphaDepth()                { return alphaDepth; }

    int GetWidth()  { return width; }
    int GetHeight() { return height; }
    int GetDepth()  { return depth; }
    
protected:
    virtual ~tImage();
    
    int width;
    int height;
    int depth;
    unsigned key;
    int alphaDepth;
    unsigned char* image;
    bool hasAlpha;
    bool keyEnabled;
};

//-------------------------------------------------------------------
class tImage8 : public tImage
{
public:
    tImage8();
    tImage8(tImage* img);

    virtual void SetSize(int width, int height);
    virtual void SetImage(int width, int height, unsigned char *newImage);
    virtual void ReSize(int width, int height);

    virtual void SetUsedColours(int i) {usedColours = i;}
    virtual int  GetUsedColours(void)  {return usedColours;}

    virtual pddiLockInfo* Lock() { return 0; }

    virtual unsigned GetColour(int x, int y);
    virtual void SetColour(int x, int y, unsigned colour);

    virtual void Clear(unsigned colour);

    virtual void SetPaletteEntry(int idx, unsigned colour);
    virtual void SetPalette(unsigned* colour, int n = 256);
    virtual unsigned GetPaletteEntry(int idx);   
    virtual unsigned* GetPalette();
    virtual void SetPixelIndex(int x, int y, unsigned index);
    virtual unsigned GetPixelIndex(int x, int y);
    
    virtual void CopyRegion(tImage* src, int sx0, int sy0, int sx1, int sy1, int dx, int dy);

protected:
    virtual ~tImage8();
    unsigned palette[256];
    int usedColours;
};

//-------------------------------------------------------------------
class tMipmapFilter;

class tImage32 : public tImage
{
public:
    tImage32();
    tImage32(tImage* img);

    virtual void SetSize(int width, int height);
    virtual void SetImage(int width, int height, unsigned char *newImage);
    virtual void ReSize(int width, int height);

    virtual pddiLockInfo* Lock() { return 0; }

    virtual unsigned GetColour(int x, int y);
    virtual void SetColour(int x, int y, unsigned colour);

    virtual void Clear(unsigned colour);

    virtual void CopyRegion(tImage* src, int sx0, int sy0, int sx1, int sy1, int dx, int dy);

protected:
    virtual ~tImage32();

};

//-----------------------------------------------------------------
class tImageLoader : public tSimpleChunkHandler
{
public:
    tImageLoader();

    tImage* LoadImage(tChunkFile* file, int depth = 32);
    void LoadIntoSurface(tChunkFile* file, pddiLockInfo* lock);

    void SetDepth(int d)  { depth = d; }
    int GetDepth()        { return depth; }

protected:
    ~tImageLoader();

    tImageFactory* factory;

    tEntity* LoadObject(tChunkFile* file, tEntityStore*);
    int depth;
};

#endif //IMAGE_HPP

