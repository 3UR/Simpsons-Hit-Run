//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _TEXTURE_HPP
#define _TEXTURE_HPP

#include <p3d/entity.hpp>
#include <p3d/loadmanager.hpp>
#include <pddi/pddi.hpp>
#include <radmath/random.hpp> // Used by the set chunk loader to pick a chunk from the set.

class tTexture: public tEntity
{
public:  
    tTexture();

    bool Create(int xSize, int ySize, int bpp, int alphaDepth, int nMip,
                    pddiTextureType textureType = PDDI_TEXTYPE_RGB,
                    pddiTextureUsageHint usageHint = PDDI_USAGE_STATIC);

    bool CreateVolume(int xSize, int ySize, int zSize, int bpp, int alphaDepth, int nMip,
                            pddiTextureType textureType = PDDI_TEXTYPE_RGB, 
                            pddiTextureUsageHint usageHint = PDDI_USAGE_STATIC);

    pddiPixelFormat GetPixelFormat()                      { return texture->GetPixelFormat(); }
    int GetWidth()                                        { return texture->GetWidth(); }
    int GetHeight()                                       { return texture->GetHeight(); }
    int GetDepth()                                        { return texture->GetDepth(); }
    int GetAlphaDepth()                                   { return texture->GetAlphaDepth(); }
    int GetNumMipMaps()                                   { return texture->GetNumMipMaps(); }
    
    int GetNumPaletteEntries()                            { return texture->GetNumPaletteEntries(); }
    void SetPalette(int nEntries, tColour* palette)       { texture->SetPalette(nEntries, palette); }
    int GetPalette(tColour* palette)                      { return texture->GetPalette(palette); }
    
    pddiLockInfo* Lock(int mip, pddiRect* rect = 0)       { return texture->Lock(mip, rect); }
    void Unlock(int mip)                                  { texture->Unlock(mip); }
    
    void Prefetch()                                       { texture->Prefetch(); }
    void Discard()                                        { texture->Discard(); }
    void SetPriority(int priority)                        { texture->SetPriority(priority); }
    int GetPriority()                                     { return texture->GetPriority(); }

    pddiTexture* GetTexture()                             { return texture; }
    void SetTexture(pddiTexture *t);

protected:
    virtual ~tTexture();
    pddiTexture* texture;
};

class tImageFactory;
class tImageConverter;

class tTextureLoader : public tSimpleChunkHandler
{
public:
    tTextureLoader();

    tEntity* LoadObject(tChunkFile*, tEntityStore*);

    tTexture* LoadTexture(tChunkFile*);

protected:
    tTexture* LoadImage(tChunkFile*, tImageFactory* factory = NULL, tTexture* texture = NULL, int mipLevel = 0);
    tTexture* LoadVolumeImage(tChunkFile*, tImageFactory* factory = NULL, tTexture* texture = NULL, int mipLevel = 0, int numMipMaps = 0);

    tImageFactory* imageFactory;
    tImageConverter* imageConverter;

    ~tTextureLoader();
};

// Currently the SetChunk only has textures in it, but
//theortically, it could have tDrawables. However, until
//this happens, the setloader will be treated like a texture.
class tSetLoader : public tChunkHandler // Can't extend from tSimpleChunkHandler.
{
public:
    tSetLoader();
    tLoadStatus Load( tChunkFile* file, tEntityStore* store );
    bool CheckChunkID( unsigned ID );
	unsigned int GetChunkID();
    static void SetRandomSeed( unsigned NewSeed );

protected:
    ~tSetLoader();
    static rmt::Randomizer m_Random;
};

#endif //_TEXTURE_HPP

