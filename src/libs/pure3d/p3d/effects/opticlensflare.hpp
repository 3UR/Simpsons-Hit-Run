/*===========================================================================
    opticlensflare.hpp

    Copyright (c)2000 Radical Entertainment, Inc.
    All rights reserved.
===========================================================================*/

#ifndef _OPTIC_LENS_FLARE_HPP
#define _OPTIC_LENS_FLARE_HPP

#include <radmath/radmath.hpp>
#include <p3d/p3dtypes.hpp>
#include <p3d/refcounted.hpp>
#include <p3d/effects/optic.hpp>
#include <p3d/array.hpp>

class tShader;
class tLensFlareGroup;
class tLensFlare;

//*****************************************************************************
//
// Class tLensFlareGroup
//
//*****************************************************************************
class tLensFlareGroup : public tOpticEffect
{
public:
    // shader used to draw all flares of the group
    void SetShader(tShader* shader);                                          
    tShader* GetShader() const                                      { return shader; }
   
    // source radius in world space
    void SetSourceRadius(float radius)                              { sourceRadius = radius; }
    float GetSourceRadius() const                                   { return sourceRadius; }

    // edge radius in world space
    void SetEdgeRadius(float radius)                                { edgeRadius = radius; }
    float GetEdgeRadius() const                                     { return edgeRadius; }
  
    // individual flare accessor functions
    int GetNumLensFlares() const                                    { return numFlares; }
    tLensFlare* GetLensFlareByIndex(int index) const;
    tLensFlare* GetLensFlareByName(const char* name) const;
    tLensFlare* GetLensFlareByUID(tUID uid) const;

    virtual void ProcessShaders(ShaderCallback&);

    friend class tLensFlareGroupLoader;

protected:
    tLensFlareGroup();
    virtual ~tLensFlareGroup();

    virtual void DisplayEffect();

    tShader*        shader;
    float           sourceRadius;
    float           edgeRadius;
    int             numFlares;
    tPtrArray<tLensFlare *> flares;
};

//*****************************************************************************
//
// Class tLensFlare
//
//*****************************************************************************
class tLensFlare : public tEntity
{
public:         
    // width of flare
    void SetWidth(float w)                                      { width = w * 0.5f; }
    float GetWidth() const                                      { return (width * 2.0f); }

    // height of flare
    void SetHeight(float h)                                     { height = h * 0.5f; }
    float GetHeight() const                                     { return (height * 2.0f); }
    
    // distance of flare from source center
    void SetDistance(float d)                                   { distance = d; }
    float GetDistance() const                                   { return distance; }

    // colour of flare
    void SetColour(tColour c)                                   { colour = c; }
    tColour GetColour() const                                   { return colour; }

    // uv of each vertex of the flare
    void SetUV(unsigned int index, rmt::Vector2 u)              { if (index < 4) uv[index] = u; }
    rmt::Vector2 GetUV(unsigned int index) const                { if (index < 4) return uv[index]; return rmt::Vector2(0.0f,0.0f); }

    // uv offset of each vertex of the flare
    void SetUVOffset(rmt::Vector2 offset)                       { uvOffset = offset; }
    rmt::Vector2 GetUVOffset() const                            { return uvOffset; }

    friend class tLensFlareGroupLoader;
    friend class tLensFlareGroup;

protected:
    tLensFlare();
    virtual ~tLensFlare();

    virtual void DisplayFlare(pddiPrimStream* stream, rmt::Vector position, float intensity);

    float               width;
    float               height;
    float               distance;
    tColour             colour;
    rmt::Vector2        uv[4];      // 0 = bottom left, 3 = top left
    rmt::Vector2        uvOffset;
};


#endif
