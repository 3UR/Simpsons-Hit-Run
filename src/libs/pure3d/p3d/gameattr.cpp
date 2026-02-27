//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include "p3d/gameattr.hpp"
#include "constants/chunks.h"
#include "constants/chunkids.hpp"
#include "p3d/utility.hpp"
#include "p3d/chunkfile.hpp"


static rmt::Vector dummyVector(0,0,0);
static rmt::Matrix dummyMatrix(1,0,0,0, 
                               0,1,0,0, 
                               0,0,1,0,
                               0,0,0,1);


tGameAttr::tGameAttr(const char* name, unsigned count) :
    numParams(count),
    params(new BaseMap[count])
{
    SetName(name);
}

tGameAttr::~tGameAttr()
{
    delete [] params;
}

bool tGameAttr::HasParam(tUID paramName, Type type)
{
    for(unsigned i = 0; i < numParams; i++)
    {
        if((params[i].name == paramName) && ((params[i].type = type) || (type == UNKNOWN)))
        {
            return true;
        }
    }
    return false;
}

int tGameAttr::GetInt(tUID paramName)
{
    BaseMap* map = Get(paramName, INT);
    return map ? *((int*)(map->value)) : 0;
}

float tGameAttr::GetFloat(tUID paramName)
{
    BaseMap* map = Get(paramName, FLOAT);
    return map ? *((float*)(map->value)) : 0.0f;
}

const rmt::Vector& tGameAttr::GetVector(tUID paramName)
{
    BaseMap* map = Get(paramName, VECTOR);
    return map ? *((rmt::Vector*)(map->value)) : dummyVector;
}

const rmt::Matrix& tGameAttr::GetMatrix(tUID paramName)
{
    BaseMap* map = Get(paramName, MATRIX);
    return map ? *((rmt::Matrix*)(map->value)) : dummyMatrix;
}

tColour tGameAttr::GetColour(tUID paramName)
{
    BaseMap* map = Get(paramName, COLOUR);

    if (map) {
        return *((tColour*)(map->value));
    }

    return tColour(255,255,255,255);
}

tGameAttr::BaseMap* tGameAttr::Get(tUID paramName, Type type)
{
    for(unsigned i = 0; i < numParams; i++)
    {
        if((params[i].name == paramName) && (params[i].type == type))
        {
            return &params[i];
        }
    }
    p3d::printf("WARNING: tGameAttr : Fetched unknown parameter\n");
    return NULL;
}


void  tGameAttr::Set(char* paramName, Type t, void* data, unsigned size)
{
    unsigned i = 0;
    
    while (params[i].value != NULL)
    {
        i++;
    }

    if(i >= numParams)
    {
        p3d::printf("WARNING : tGameAttr could not add parameter\n");
        return;
    }

    //we've reached the next free slot
    params[i].SetName(paramName);
    params[i].SetParam(data, size);
    params[i].SetType(t);

}

void tGameAttr::SetInt(char* paramName, int v)
{
    Set(paramName, INT, &v, sizeof(int));
}
void tGameAttr::SetFloat(char* paramName, float v)
{
    Set(paramName, FLOAT, &v, sizeof(float));
}
void tGameAttr::SetColour(char* paramName, tColour v)
{
    Set(paramName, COLOUR, &v, sizeof(tColour));

}
void tGameAttr::SetMatrix(char* paramName, const rmt::Matrix& v)
{
    Set(paramName, MATRIX, (void*)&v, sizeof(rmt::Matrix));
}

void tGameAttr::SetVector(char* paramName, const rmt::Vector& v)
{
    Set(paramName, VECTOR, (void*)&v, sizeof(rmt::Vector));
}

tGameAttrLoader::tGameAttrLoader() : tSimpleChunkHandler(Pure3D::GameAttr::GAME_ATTR)
{
}

tEntity* tGameAttrLoader::LoadObject(tChunkFile* f, tEntityStore* store)
{
    char name[128];
    char paramName[128];
    
    
    f->GetString(name);
    
    int version = f->GetInt();
    P3DASSERT(version == 0);
    

    int count = f->GetInt();


    tGameAttr* GameAttr = new tGameAttr(name, count);

    while(f->ChunksRemaining())
    {
        f->BeginChunk();

        switch(f->GetCurrentID())
        {
            case Pure3D::GameAttr::INT_PARAM:
                {
                    char* param = f->GetString(paramName);
                    unsigned val = f->GetInt();

                    GameAttr->SetInt(param, val);
                }
                break;

            case Pure3D::GameAttr::FLOAT_PARAM:
                {
                    char* param = f->GetString(paramName);
                    float val = f->GetFloat();

                    GameAttr->SetFloat(param, val);
                }
                break;

            case Pure3D::GameAttr::COLOUR_PARAM:
                {
                    char* param = f->GetString(paramName);
                    unsigned val = f->GetInt();

                    GameAttr->SetColour(param, pddiColour(val));
                }
                break;

            case Pure3D::GameAttr::VECTOR_PARAM:
                {
                    char* param = f->GetString(paramName);

                    rmt::Vector v;
                    v.x = f->GetFloat();
                    v.y = f->GetFloat();
                    v.z = f->GetFloat();

                    GameAttr->SetVector(param, v);
                }
                break;

            case Pure3D::GameAttr::MATRIX_PARAM:
                {
                    char* param = f->GetString(paramName);

                    rmt::Matrix m;
                    f->GetData(&m, 16, tFile::DWORD);
                    GameAttr->SetMatrix(param, m);
                }
                break;

        }

        f->EndChunk();
    }
    return GameAttr;
}

tGameAttr::BaseMap::~BaseMap()
{
    if (value)
        delete [] value;
}

void tGameAttr::BaseMap::SetName(char* nameText)
{
    name.SetText(nameText);
}

void tGameAttr::BaseMap::SetParam(void* param, int size)
{
    value = new char[size];
    memcpy(value, param, size);
}

