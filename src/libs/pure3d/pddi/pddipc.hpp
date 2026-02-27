//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _PDDIPC_HPP
#define _PDDIPC_HPP

//-------------------------------------------------------------------
// pddiPrimStream - immediate more rendering
//-------------------------------------------------------------------
class pddiPrimStream
{
public:
    virtual void Coord(float x, float y, float z)=0;
    virtual void Normal(float x, float y, float z)=0;
    virtual void Binormal(float x, float y, float z) { };
    virtual void Tangent(float x, float y, float z) { };
    virtual void Colour(pddiColour colour, int channel = 0)=0;
    virtual void UV(float u, float v, int channel = 0)=0;
    virtual void Specular(pddiColour colour)=0;
    virtual void SkinIndices(unsigned, unsigned = 0, unsigned = 0, unsigned = 0) {};
    virtual void SkinWeights(float, float = 0.0f, float = 0.0f) {};

    virtual void Vertex(pddiVector* v, pddiColour c)=0;
    virtual void Vertex(pddiVector* v, pddiVector* n)=0;
    virtual void Vertex(pddiVector* v, pddiVector2* uv)=0;
    virtual void Vertex(pddiVector* v, pddiColour c, pddiVector2* uv)=0;
    virtual void Vertex(pddiVector* v, pddiVector* n, pddiVector2* uv)=0;
};

//-------------------------------------------------------------------
// types and defines for non-console PDDI libraries
// assumes OpenGL-style rendering

// alpha blending modes
enum pddiBlendFactor
{
    PDDI_BF_ZERO,
    PDDI_BF_ONE,
    PDDI_BF_SRC,
    PDDI_BF_ONE_MINUS_SRC,
    PDDI_BF_DEST,
    PDDI_BF_ONE_MINUS_DEST,
    PDDI_BF_SRCALPHA,
    PDDI_BF_ONE_MINUS_SRCALPHA,
    PDDI_BF_DESTALPHA,
    PDDI_BF_ONE_MINUS_DESTALPHA,
    PDDI_BF_SRCALPHASATURATE
};

class pddiBlendController : public pddiObject
{
public:
    virtual void SetBlend(pddiBlendFactor src, pddiBlendFactor dest) = 0;
    virtual void GetBlend(pddiBlendFactor* src, pddiBlendFactor* dest) = 0;
};

#endif //_PDDIPC_HPP
