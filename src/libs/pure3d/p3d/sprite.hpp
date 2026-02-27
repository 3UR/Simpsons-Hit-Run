//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _SPRITE_HPP_
#define _SPRITE_HPP_

#include <p3d/drawable.hpp>
#include <p3d/loadmanager.hpp>
#include <pddi/pdditype.hpp>

struct sprVertex
{
    pddiVector v;
    pddiColour c;
    pddiVector2 uv0;
};

class tTexture;
class pddiMat;
class tImageConverter;
class tImageLoader;
class tImageFactory;
class tImage;
class tShader;

struct tRect;

class tSprite : public tDrawable
{
public:
    tSprite(tImage*, tShader* Shader = NULL, int blitBorder = 1, int nativeX = 640, int nativeY = 480,tImageConverter* conv = 0);
    tSprite(tTexture**, int w, int h, int count, tShader* Shader = NULL, int blitBorder = 1, int nativeX = 640, int nativeY = 480,tImageConverter* conv = 0);

    void Display();

    int GetNativeX(void); 
    int GetNativeY(void);

    int GetHeight(void) {return height; } 
    int GetWidth(void)  {return width; }

    virtual void GetBoundingBox(rmt::Box3D* b)          { *b = box; }
    virtual void GetBoundingSphere(rmt::Sphere* s)      { *s = sphere; }

    virtual void ProcessShaders(ShaderCallback&);

    void SetColour(tColour colour); 
    tColour GetColour(void)                         { return colour; }

    void SetBlitBorder(int border);
    int  GetBlitBorder()                            { return blitBorder; }

    tShader* GetShader(void);

protected:
    ~tSprite();

    int  SelectSegment(int val);
    int  CalcSections(tRect** sections, int &numXSect, int &numYSect);
    void BuildSections(tImage* image, tImageConverter* conv);
    void BuildTexture(int texNum, tRect& src, int newX, int newY, tImage* image, tImage* buffer, tImageConverter* conv);
    void BuildPoly(int polyNum, tRect& src, int newX, int newY);

    int nPolys;
    sprVertex* polys;
    tTexture** textures;

    tShader* Shader;

    tColour colour;
    int height, width;
    int blitBorder;
    int nativeX, nativeY;
    int maxSize, minSize;
    float scaleX, scaleY;

    rmt::Box3D box;
    rmt::Sphere sphere;
};


class tSpriteLoader : public tSimpleChunkHandler
{
public:
    tSpriteLoader();

    tImageConverter* GetImageConverter(void) { return converter; }

    void SetAlphaTest(bool b) { forceAlphaTest = b; }
    bool GetAlphaTest(void) { return forceAlphaTest; }

    virtual void SetNormalization(bool n) { normalize = n;}
    virtual bool GetNormalization(void) { return normalize;}

    virtual tImage* LoadImage(tChunkFile*,int);
    virtual tTexture* LoadTexture(tChunkFile*,int);

protected:
    ~tSpriteLoader();
    tEntity* LoadObject(tChunkFile*, tEntityStore*);
    tImageConverter* converter;
    tImageLoader* imageLoader;
    tImageFactory* imageFactory;
    bool forceAlphaTest;
    bool normalize;
};

#endif

