//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include <pddi/base/baseshader.hpp>
#include <string.h>
#include <pddi/base/debug.hpp>


unsigned pddiBaseShader::currentUID = 0xffffffff;
unsigned pddiBaseShader::newUID = 0;
pddiBaseShader* pddiBaseShader::lastShader = 0;

// the structure holding initialized shader types
struct pddiShadeAllocTable
{
    char name[64];
    pddiShadeAllocFunc func;
    const char* aux;
};

static int shaderCount = 0;
static pddiShadeAllocTable shaders[PDDI_SHADER_COUNT] = 
{
    { "", NULL }
};

// Ugly macro for handling actual dispatch of functions. Has to be a macro 
// because of the need to insert the type name into an identifier
// basically just loops through the list until PDDI_SP_NULL is hit, 
// checks the parameter for that table entry against the calling parameter, 
// and calls the funciton pointer if it matches
#define DISPATCH(type, param, val) \
    pddiShade##type##Table * table##type = Get##type##Table(); \
    if(table##type) \
    {\
        while(table##type->param != PDDI_SP_NULL) \
        { \
            if(table##type->param == param) \
         { \
                (this->*(table##type->func))(val);\
                newUID++;\
                if (newUID==0xffffffff) newUID=0;\
                uid = newUID++;\
                return true;\
            }\
            table##type++;\
        }\
    }\
    return false;\


bool pddiBaseShader::SetTexture(unsigned param, pddiTexture* tex)
{
    DISPATCH(Texture, param, tex);
}

bool pddiBaseShader::SetInt(unsigned param, int i)
{
    // ints and floats are dispatched to both lists, so that numerical values can be set as either
    DISPATCH(Int, param, i);
    DISPATCH(Float, param, (float)i);
}

bool pddiBaseShader::SetFloat(unsigned param, float f)
{
    // ints and floats are dispatched to both lists, so that numerical values can be set as either
    DISPATCH(Float, param, f);
    DISPATCH(Int, param, (int)f);
}

bool pddiBaseShader::SetColour(unsigned param, pddiColour c)
{
    DISPATCH(Colour, param, c);
}

bool pddiBaseShader::SetVector(unsigned param, const pddiVector& v)
{
    DISPATCH(Vector, param, v);
}

bool pddiBaseShader::SetMatrix(unsigned param, const pddiMatrix& m)
{
    DISPATCH(Matrix, param, m);
}

// installing and allocating shaders
void pddiBaseShader::InstallShader(const char* name, pddiShadeAllocFunc func, const char* aux)
{
    PDDIASSERT(shaderCount < PDDI_SHADER_COUNT);
    strcpy(shaders[shaderCount].name, name);
    shaders[shaderCount].func = func;
    shaders[shaderCount].aux = aux;
    shaderCount++;
}
    
pddiBaseShader* pddiBaseShader::AllocateShader(pddiRenderContext* c, const char* name, const char* aux)
{
    for(int i = 0; i < shaderCount; i++)
    {
        if(strcmp(name, shaders[i].name) == 0)
            return shaders[i].func(c, name, shaders[i].aux ? shaders[i].aux : aux);
    }
    PDDIASSERT(shaders[0].func);
    return shaders[0].func(c, name, aux);
}


