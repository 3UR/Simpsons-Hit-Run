//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _GLMAT_HPP_
#define _GLMAT_HPP_

#include <pddi/pddi.hpp>
#include <pddi/base/baseshader.hpp>
#include <pddi/gl/glcon.hpp>
class pglTexture;

const int pglMaxPasses = 1;

struct pglTextureEnv
{
    bool enabled;
    pglTexture* texture;

    int uvSet;
    pddiTextureGen texGen;
    pddiUVMode uvMode;
    pddiFilterMode filterMode;

    bool alphaTest;
    pddiBlendMode alphaBlendMode;
    pddiCompareMode alphaCompareMode;
    float alphaRef;

    bool lit;
    bool twoSided;
    pddiShadeMode shadeMode;
    pddiColour diffuse;
    pddiColour specular;
    pddiColour ambient;
    pddiColour emissive;
    float shininess;
};

class pglMat : public pddiBaseShader
{
public:
    pglMat(pglContext*);
    ~pglMat();

    static pddiShadeColourTable colourTable[];
    static pddiShadeTextureTable textureTable[];
    static pddiShadeIntTable intTable[];
    static pddiShadeFloatTable floatTable[];

    const char* GetType(void);
    int         GetPasses(void);
    void        SetPass(int pass);

    pddiShadeTextureTable* GetTextureTable(void) { return textureTable;}
    pddiShadeIntTable*     GetIntTable(void)     { return intTable;}
    pddiShadeFloatTable*   GetFloatTable(void)   { return floatTable;}
    pddiShadeColourTable*  GetColourTable(void)  { return colourTable;}

    // texture
    void SetTexture(pddiTexture* texture);
    void SetUVMode(int mode);
    void SetFilterMode(int mode);

    // shading
    void SetShadeMode(int shade);
    void SetTwoSided(int);

    // lighting
    void EnableLighting(int);

    void SetDiffuse(pddiColour colour);
    void SetAmbient(pddiColour colour);
    void SetEmissive(pddiColour);
    void SetEmissiveAlpha(int);
    void SetSpecular(pddiColour);
    void SetShininess(float power);

    // alpha blending
    void SetBlendMode(int mode);
    void EnableAlphaTest(int);
    void SetAlphaCompare(int compare);
    void SetAlphaRef(float ref);

    int  CountDevPasses(void);
    void SetDevPass(unsigned);

private:
   
    pglContext* context;
    int pass;
    pglTextureEnv texEnv[pglMaxPasses];
};

#endif

