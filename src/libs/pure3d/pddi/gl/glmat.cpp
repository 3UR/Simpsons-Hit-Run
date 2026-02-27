//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================

#include <pddi/gl/gl.hpp>
#include <pddi/gl/glmat.hpp>
#include <pddi/gl/gltex.hpp>
#include <pddi/gl/glcon.hpp>
#include <pddi/gl/gldisplay.hpp>

#include <microprofile.h>

pddiShadeColourTable pglMat::colourTable[] = 
{
    {PDDI_SP_AMBIENT  , SHADE_COLOUR(&pglMat::SetAmbient)},
    {PDDI_SP_DIFFUSE  , SHADE_COLOUR(&pglMat::SetDiffuse)},
    {PDDI_SP_EMISSIVE , SHADE_COLOUR(&pglMat::SetEmissive)},
    {PDDI_SP_SPECULAR , SHADE_COLOUR(&pglMat::SetSpecular)},
    {PDDI_SP_NULL , NULL}
};

pddiShadeTextureTable pglMat::textureTable[] = 
{
    {PDDI_SP_BASETEX , SHADE_TEXTURE(&pglMat::SetTexture)},
    {PDDI_SP_NULL , NULL}
};

pddiShadeIntTable pglMat::intTable[] = 
{
    {PDDI_SP_UVMODE , SHADE_INT(&pglMat::SetUVMode)},
    {PDDI_SP_FILTER , SHADE_INT(&pglMat::SetFilterMode)},
    {PDDI_SP_SHADEMODE , SHADE_INT(&pglMat::SetShadeMode)},
    {PDDI_SP_ISLIT , SHADE_INT(&pglMat::EnableLighting)},
    {PDDI_SP_BLENDMODE , SHADE_INT(&pglMat::SetBlendMode)},
    {PDDI_SP_ALPHATEST , SHADE_INT(&pglMat::EnableAlphaTest)},
    {PDDI_SP_ALPHACOMPARE , SHADE_INT(&pglMat::SetAlphaCompare)},
    {PDDI_SP_TWOSIDED , SHADE_INT(&pglMat::SetTwoSided)},
    {PDDI_SP_EMISSIVEALPHA , SHADE_INT(&pglMat::SetEmissiveAlpha)},
    {PDDI_SP_NULL , NULL}
};

pddiShadeFloatTable pglMat::floatTable[] = 
{
    {PDDI_SP_SHININESS , SHADE_FLOAT(&pglMat::SetShininess)},
    {PDDI_SP_ALPHACOMPARE_THRESHOLD , SHADE_FLOAT(&pglMat::SetAlphaRef)},
    {PDDI_SP_NULL , NULL}
};

GLenum filterMagTable[5] =
{
    GL_NEAREST,
    GL_LINEAR,
    GL_NEAREST,
    GL_LINEAR,
    GL_LINEAR
};

// GL_NEAREST_MIPMAP_LINEAR not used
GLenum filterMinTable[5] =
{
    GL_NEAREST,
    GL_LINEAR,
    GL_NEAREST,//GL_NEAREST_MIPMAP_NEAREST,
    GL_LINEAR,//GL_LINEAR_MIPMAP_NEAREST,
    GL_LINEAR,//GL_LINEAR_MIPMAP_LINEAR
};

GLenum texBlendTable[6] =
{
    GL_REPLACE,
    GL_DECAL,
    GL_MODULATE,
    GL_MODULATE, // MODULATEALPHA not supported
    GL_REPLACE,  // ADD - Not supported
    GL_MODULATE, // MODULATEINTENSITY
};

GLenum uvTable[3] =
{
    GL_REPEAT,
    GL_CLAMP_TO_EDGE,
    GL_CLAMP_TO_EDGE
};

GLenum alphaCompareTable[8] =
{
    GL_NEVER,
    GL_ALWAYS,
    GL_LESS,
    GL_LEQUAL,
    GL_GREATER,
    GL_GEQUAL,
    GL_EQUAL,
    GL_NOTEQUAL
};

GLenum alphaBlendTable[8][3] =
{
    { GL_FUNC_ADD, GL_ONE, GL_ZERO },                       //PDDI_BLEND_NONE,
    { GL_FUNC_ADD, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA },  //PDDI_BLEND_ALPHA
    { GL_FUNC_ADD, GL_ONE, GL_ONE },                        //PDDI_BLEND_ADD
    { GL_FUNC_REVERSE_SUBTRACT, GL_ONE, GL_ONE },           //PDDI_BLEND_SUBTRACT
    { GL_FUNC_ADD, GL_DST_COLOR, GL_ZERO },                 //PDDI_BLEND_MODULATE,
    { GL_FUNC_ADD, GL_DST_COLOR, GL_SRC_COLOR},             //PDDI_BLEND_MODULATE2,
    { GL_FUNC_ADD, GL_ONE, GL_SRC_ALPHA},                   //PDDI_BLEND_ADDMODULATEALPHA,
    { GL_FUNC_REVERSE_SUBTRACT, GL_SRC_ALPHA, GL_SRC_ALPHA} //PDDI_BLEND_SUBMODULATEALPHA
};

static inline void FillGLColour(pddiColour c, float* f)
{
    f[0] = float(c.Red()) / 255;
    f[1] = float(c.Green()) / 255;
    f[2] = float(c.Blue()) / 255;
    f[3] = float(c.Alpha()) / 255;
}

pglMat::pglMat(pglContext* c) 
{
    context = c;

    for(int i = 0; i < pglMaxPasses; i++)
    {
        texEnv[i].enabled = false;
        texEnv[i].texture = NULL;
        texEnv[i].uvSet = i;
        texEnv[i].texGen = PDDI_TEXGEN_NONE;
        texEnv[i].uvMode = PDDI_UV_CLAMP;
        texEnv[i].filterMode = PDDI_FILTER_BILINEAR;

        texEnv[i].lit = false;
        texEnv[i].twoSided = false;
        texEnv[i].shadeMode = PDDI_SHADE_GOURAUD;
        texEnv[i].ambient.Set(255,255,255);
        texEnv[i].diffuse.Set(255,255,255);
        texEnv[i].specular.Set(0,0,0);
        texEnv[i].emissive.Set(0,0,0);
        texEnv[i].shininess = 0.0f;

    //   srcBlend = PDDI_BF_ONE;
    //   destBlend = PDDI_BF_ZERO;

        texEnv[i].alphaTest = false;
        texEnv[i].alphaCompareMode = PDDI_COMPARE_GREATEREQUAL;
        texEnv[i].alphaBlendMode = PDDI_BLEND_NONE;
        texEnv[i].alphaRef = 0.5f;
    }
    texEnv[0].enabled = true;
    pass = 0;
}

pglMat::~pglMat() 
{
    for(int i = 0; i < pglMaxPasses; i++)
        if(texEnv[i].texture)
            texEnv[i].texture->Release();
}


const char* pglMat::GetType(void)
{
    static char simple[] = "simple";
    return simple;
}

int pglMat::GetPasses(void)
{
    return 1;
}

void pglMat::SetPass(int pass)
{
    SetDevPass(pass);
}

void pglMat::SetTexture(pddiTexture* t) 
{
    if(t == texEnv[pass].texture)
        return;

    if(texEnv[pass].texture)
        texEnv[pass].texture->Release();

    texEnv[pass].texture = (pglTexture*)t;

    if(texEnv[pass].texture)
        texEnv[pass].texture->AddRef();
}

void pglMat::SetUVMode(int mode) 
{
    texEnv[pass].uvMode = (pddiUVMode)mode;
}

void pglMat::SetFilterMode(int mode) 
{
    texEnv[pass].filterMode = (pddiFilterMode)mode;
}

void pglMat::SetShadeMode(int shade) 
{
    texEnv[pass].shadeMode = (pddiShadeMode)shade;
}

void pglMat::SetTwoSided(int b)
{
    texEnv[pass].twoSided = b != 0;
}

void pglMat::EnableLighting(int b)
{
    texEnv[pass].lit = b != 0;
}

void pglMat::SetAmbient(pddiColour a) 
{
    texEnv[pass].ambient = a;
}

void pglMat::SetDiffuse(pddiColour colour) 
{
    texEnv[pass].diffuse = colour;
}

void pglMat::SetSpecular(pddiColour c) 
{
    texEnv[pass].specular = c;
}

void pglMat::SetEmissive(pddiColour c) 
{
    texEnv[pass].emissive = c;
    SetEmissiveAlpha(c.Alpha());
}

void pglMat::SetEmissiveAlpha(int alpha)
{
    texEnv[pass].diffuse.SetAlpha(alpha);
    if(alpha < 255)
    {
        texEnv[pass].specular.SetAlpha(0);
        texEnv[pass].ambient.SetAlpha(0);
        texEnv[pass].emissive.SetAlpha(0);
    }
    else
    {
        texEnv[pass].specular.SetAlpha(255);
        texEnv[pass].ambient.SetAlpha(255);
        texEnv[pass].emissive.SetAlpha(255);
    }
}

void pglMat::SetShininess(float power) 
{
    texEnv[pass].shininess = power;
}

void pglMat::SetBlendMode(int mode) 
{
    texEnv[pass].alphaBlendMode = (pddiBlendMode)mode;
}

void pglMat::EnableAlphaTest(int b) 
{
    texEnv[pass].alphaTest = b != 0;
}

void pglMat::SetAlphaCompare(int compare) 
{
    texEnv[pass].alphaCompareMode = pddiCompareMode(compare);
}

void pglMat::SetAlphaRef(float ref) 
{
    texEnv[pass].alphaRef = ref;
}

int pglMat::CountDevPasses(void) 
{
    return 1;
}

void pglMat::SetDevPass(unsigned pass)
{
    MICROPROFILE_SCOPEI( "PDDI", "pglMat::SetDevPass", MP_RED );

    int i = 0;

    if(texEnv[i].texture)
    {
        texEnv[i].texture->SetGLState();

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filterMagTable[texEnv[i].filterMode]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filterMinTable[texEnv[i].filterMode]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, uvTable[texEnv[i].uvMode]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, uvTable[texEnv[i].uvMode]);
        glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
    }
    else
    {
        glDisable(GL_TEXTURE_2D);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    }

    if(texEnv[i].alphaTest)
    {
        glEnable(GL_ALPHA_TEST);
        glAlphaFunc(alphaCompareTable[texEnv[i].alphaCompareMode],texEnv[i].alphaRef);
    }
    else
    {
        glDisable(GL_ALPHA_TEST);
    }

    if(texEnv[i].alphaBlendMode == PDDI_BLEND_NONE)
    {
        glDisable(GL_BLEND);
    }
    else
    {
        glEnable(GL_BLEND);
        glBlendEquation(alphaBlendTable[texEnv[i].alphaBlendMode][0]);
        glBlendFunc(alphaBlendTable[texEnv[i].alphaBlendMode][1],alphaBlendTable[texEnv[i].alphaBlendMode][2]);
    }
 
    if(texEnv[i].lit)
    {
        float c[4];

        glMaterialf(GL_FRONT_AND_BACK,GL_SHININESS, texEnv[i].shininess);

        FillGLColour(texEnv[i].ambient, c);
        glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT, c);

        FillGLColour(texEnv[i].emissive, c);
        glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION, c);

        FillGLColour(texEnv[i].diffuse, c);
        glMaterialfv(GL_FRONT_AND_BACK,GL_DIFFUSE, c);

        FillGLColour(texEnv[i].specular, c);
        glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR, c);

        glEnable(GL_LIGHTING);
        glDisableClientState(GL_COLOR_ARRAY);
    }
    else
    {
        glDisable(GL_LIGHTING);
        glDisableClientState(GL_NORMAL_ARRAY);
    }

    if( texEnv[i].twoSided || context->GetCullMode() == PDDI_CULL_NONE )
    {
        glDisable(GL_CULL_FACE);
    }
    else
    {
        glEnable(GL_CULL_FACE);
    }
}



