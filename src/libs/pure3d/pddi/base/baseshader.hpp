//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _BASESHADER_HPP_
#define _BASESHADER_HPP_

#include <pddi/pddi.hpp>

class pddiBaseShader;
const int PDDI_SHADER_COUNT = 16;

// Definitiions for the function tables used to dispatch set operations
// into derived classes, one eact for Texture, Int, Float, Colour, 
// Matrix and Vector:
//    pddiShade<type>Function = function pointer
//    pddiShade<type>Table    = table of param type/funciton pointer pairs
//    SHADE_<type>            = cast macro for converting funciton pointers to base class
//
//    tables should be terminated with a PDDI_SP_NULL entry.
//    see dx9/simpleshader.* for examples of user

typedef void (pddiBaseShader::*pddiShadeIntFunction)(int);
struct pddiShadeIntTable
{
    unsigned param;
    pddiShadeIntFunction func;
};
#define SHADE_INT(x) ((pddiShadeIntFunction)x)

typedef void (pddiBaseShader::*pddiShadeTextureFunction)(pddiTexture*);
struct pddiShadeTextureTable
{
    unsigned param;
    pddiShadeTextureFunction func;
};
#define SHADE_TEXTURE(x) ((pddiShadeTextureFunction)x)

typedef void (pddiBaseShader::*pddiShadeFloatFunction)(float);
struct pddiShadeFloatTable
{
    unsigned param;
    pddiShadeFloatFunction func;
};
#define SHADE_FLOAT(x) ((pddiShadeFloatFunction)x)

typedef void (pddiBaseShader::*pddiShadeColourFunction)(pddiColour);
struct pddiShadeColourTable
{
    unsigned param;
    pddiShadeColourFunction func;
};

#define SHADE_COLOUR(x) ((pddiShadeColourFunction)x)

typedef void (pddiBaseShader::*pddiShadeVectorFunction)(const pddiVector&);
struct pddiShadeVectorTable
{
    unsigned param;
    pddiShadeVectorFunction func;
};
#define SHADE_VECTOR(x) ((pddiShadeVectorFunction)x)


typedef void (pddiBaseShader::*pddiShadeMatrixFunction)(const pddiMatrix&);
struct pddiShadeMatrixTable
{
    unsigned param;
    pddiShadeMatrixFunction func;
};
#define SHADE_MATRIX(x) ((pddiShadeMatrixFunction)x)


// a shader allocation funciton
typedef pddiBaseShader* (*pddiShadeAllocFunc)(pddiRenderContext* context, const char* name, const char* aux);

// the base class of all shaders
class pddiBaseShader : public pddiShader
{
public:
    pddiBaseShader()  { uid = newUID++;}
    ~pddiBaseShader() { if(lastShader == this) lastShader = NULL;}

    // set functions for pddi interface, implimented to pass through dispatch tables
    // don't need to be overloaded most of the time (DX9 effects shader does though)
    virtual bool SetTexture(unsigned param, pddiTexture* tex);
    virtual bool SetInt(unsigned param, int);
    virtual bool SetFloat(unsigned param, float);
    virtual bool SetColour(unsigned param, pddiColour);
    virtual bool SetVector(unsigned param, const pddiVector&);
    virtual bool SetMatrix(unsigned param, const pddiMatrix&);


    // for managing last used material/avoiding redundant state setting, should not be overloaded
    bool IsCurrent(void) { return ((uid == currentUID) && (GetPasses() == 1)); }
    // number of passes this material requires, must be overloaded in derived class
    virtual int  GetPasses(void) = 0;

    void SetMaterial(int pass = 0)  
    { 
        if(uid != currentUID)
        { 
            if(lastShader) lastShader->PostRender();
            currentUID = uid;  
            lastShader = this;
            PreRender();
            SetPass(pass);
        }
        else if(GetPasses() != 1)
        {
            SetPass(pass);
        }
    }

    static void ClearCurrentShader(void)
    {
        if(lastShader) lastShader->PostRender();
        currentUID = 0xffffffff;  
        lastShader = NULL;
    }

    // installing and allocating shaders
    static void InstallShader(const char* name, pddiShadeAllocFunc, const char* aux);
    static pddiBaseShader* AllocateShader(pddiRenderContext* c, const char* name, const char* aux);

protected:
    // data for managing last used material
    static pddiBaseShader* lastShader;
    static unsigned currentUID;
    static unsigned newUID;
    unsigned uid;
    
    // internal setup function for each pass, derived class needs to define this
    virtual void SetPass(int pass) = 0;
    virtual void PreRender(void) {};
    virtual void PostRender(void) {};

    // funcitons to return each parameter table
    // stubbed so that derived classes only need to impliment the ones they use
    virtual pddiShadeTextureTable* GetTextureTable(void) { return NULL;}
    virtual pddiShadeIntTable*     GetIntTable(void) { return NULL;}
    virtual pddiShadeFloatTable*   GetFloatTable(void) { return NULL;}
    virtual pddiShadeColourTable*  GetColourTable(void) { return NULL;}
    virtual pddiShadeVectorTable*  GetVectorTable(void) { return NULL;}
    virtual pddiShadeMatrixTable*  GetMatrixTable(void) { return NULL;}

};

#endif

