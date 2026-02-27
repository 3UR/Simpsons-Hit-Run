//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include "p3d/shader.hpp"
#include "constants/chunks.h"
#include "constants/chunkids.hpp"
#include "p3d/utility.hpp"
#include "p3d/chunkfile.hpp"
#include "p3d/texture.hpp"
#include <string.h>

tShader::tShader()
{
    shader = p3d::device->NewShader("simple");
    shader->AddRef();
}

tShader::tShader(const char* shaderName, char* definition)
{
    shader = p3d::device->NewShader(shaderName, definition);
    shader->AddRef();
}

tShader::~tShader()
{
    if(shader)
        shader->Release();

    shader = NULL;
}

tShaderLoader::tShaderLoader() : tSimpleChunkHandler(Pure3D::Shader::SHADER)
{
}

static void DrawInventory(int index = -1)
{
    tInventory::Iterator<tEntity> it;
    tEntity* obj = it.First();
    p3d::print("inventory dump:\n");
    while(obj)
    {
        char cls[128];
        p3d::ClassName(obj, cls, 128);
#ifdef P3D_ALLOW_ENTITY_GETNAME
        p3d::printf("\t'%s' (%s)\n", obj->GetName(), cls);
#endif
        obj = it.Next();            
    }
}

tEntity* tShaderLoader::LoadObject(tChunkFile* f, tEntityStore* store)
{
    char name[128];
    char shaderName[128];
    
    
    char pure [ ]= "pure3";
    char tempname[8];
    char lambert[ ] ="lambe";
    char buffer [255];

    
    f->GetString(name);

    strncpy(tempname,name,5);    
    tempname[5]='\0';

#ifndef RAD_RELEASE 
     //Chuck: adding this to SO no shaders called pure3dsimpleshader and lambert ever get loaded into our game
    if (strcmp(tempname,pure)==0)
    {
        sprintf(buffer,"Error: A Shader with %s in the name found in File %s \n",name,f->GetFilename()); 
        rReleasePrintf(buffer);

        //rTuneAssertMsg(0,buffer);
        rAssertMsg(0,buffer);
        return NULL;
    }

    if (strcmp(tempname,lambert) ==0)
    {
        sprintf(buffer,"Error: A Shader with %s in the name found in File %s \n",name,f->GetFilename());
        rReleasePrintf(buffer);

        //rTuneAssertMsg(0,buffer);
        rAssertMsg(0,buffer);
        return NULL;
    }
    
#endif

    int version = f->GetInt();
    P3DASSERT(version == 0);
    f->GetString(shaderName);

    bool     HasTranslucency = (f->GetLong() != 0);
    unsigned VertexNeeds = f->GetLong();
    unsigned VertexMask = f->GetLong();

    // nv: hack to fix badly mipmapped chainlink fence
    bool foundEvilChainlink = false;

    int count = f->GetInt();


    tShader* shader = NULL;

    while(f->ChunksRemaining())
    {
        f->BeginChunk();

        switch(f->GetCurrentID())
        {
            case Pure3D::Shader::SHADER_DEFINITION:
                {
                    P3DASSERT(!shader);
                    char defName[256];
                    f->GetString(defName); // not used when embeded
                    int size = f->GetInt();
                    char* buffer = (char*)p3d::MallocTemp(size+1);
                    f->GetData(buffer, size, tFile::BYTE);
                    buffer[size] = 0;
                    shader = new tShader(shaderName, buffer);
                    p3d::FreeTemp(buffer);
                }
                break;

            case Pure3D::Shader::TEXTURE_PARAM:
                {
                    char texName[256];

                    if(!shader)
                        shader = new tShader(shaderName);

                    unsigned param = f->GetInt();
                    f->GetPString(texName);

                    // oct15/2002 amb - todo fix this
                    tTexture* tex = p3d::find<tTexture>(store, texName);
                    if(!tex)
                    {                        // try a global find.
                        tex = p3d::find<tTexture>(texName);
                    }
                    
                    // nv:  If this shader uses the chainlink texture, I'm going to turn off mipmapping
                    if( strstr(texName, "chainlink") )
                    {
                        foundEvilChainlink = true;
                    }

                    shader->SetTexture(param, tex);
                }
                break;

            case Pure3D::Shader::INT_PARAM:
                {
                    if(!shader)
                        shader = new tShader(shaderName);

                    unsigned param = f->GetInt();
                    unsigned val = f->GetInt();

                    shader->SetInt(param, val);
                }
                break;

            case Pure3D::Shader::FLOAT_PARAM:
                {
                    if(!shader)
                        shader = new tShader(shaderName);

                    unsigned param = f->GetInt();
                    float val = f->GetFloat();

                    shader->SetFloat(param, val);
                }
                break;

            case Pure3D::Shader::COLOUR_PARAM:
                {
                    if(!shader)
                        shader = new tShader(shaderName);

                    unsigned param = f->GetInt();
                    unsigned val = f->GetInt();

                    if(param != PDDI_SP_AMBIENT)
                    {
                        shader->SetColour(param, pddiColour(val));
                    }

                    if(param == PDDI_SP_DIFFUSE)
                    {
                        shader->SetColour(PDDI_SP_AMBIENT, pddiColour(val));
                    }

                }
                break;

            case Pure3D::Shader::VECTOR_PARAM:
                {
                    if(!shader)
                        shader = new tShader(shaderName);

                    unsigned param = f->GetInt();

                    pddiVector v;
                    v.x = f->GetFloat();
                    v.y = f->GetFloat();
                    v.z = f->GetFloat();

                    shader->SetVector(param, v);
                }
                break;

            case Pure3D::Shader::MATRIX_PARAM:
                {
                    if(!shader)
                        shader = new tShader(shaderName);

                    unsigned param = f->GetInt();

                    pddiMatrix m;
                    f->GetData(&m, 16, tFile::DWORD);
                    shader->SetMatrix(param, m);
                }
                break;

        }

        f->EndChunk();
    }
    if(!shader)
        shader = new tShader(shaderName);

    // nv:  fix the solid chainlink fences
    if( foundEvilChainlink )
    {
        shader->SetFloat( PDDI_SP_MIPMAP_SHARPNESS, 0.0f );
    }
    shader->mTranslucent = HasTranslucency;
    shader->SetName(name);
    return shader;
}


bool tShaderDefinitionLoader::CheckChunkID(unsigned i)
{
    return i == Pure3D::Shader::SHADER_DEFINITION;
}

tLoadStatus tShaderDefinitionLoader::Load(tChunkFile* f, tEntityStore* store)
{
    P3DASSERT(f->GetCurrentID() == Pure3D::Shader::SHADER_DEFINITION);

    char defName[256];
    f->GetString(defName);

    int size = f->GetInt();
    char* buffer = (char*)p3d::MallocTemp(size+1);
    f->GetData(buffer, size, tFile::BYTE);
    buffer[size] = 0;

    p3d::device->AddCustomShader(defName, buffer);

    p3d::FreeTemp(buffer);

    return LOAD_OK;
}

