/*=============================================================================
  expression.cpp
  21-Mar-2000
  Created by: Katrina

  Description: Pure3D expression classes for parameterized
                    facial expressions
  Copyright (c) 1997-2000 Radical Entertainment, Inc.
  All Rights Reserved
=============================================================================*/

#include <stdlib.h>

#include <constants/chunks.h>
#include <constants/chunkids.hpp>

#include <p3d/chunkfile.hpp>
#include <p3d/loadmanager.hpp>
#include <p3d/utility.hpp>
#include <p3d/anim/expression.hpp>
#include <p3d/anim/animate.hpp>
#include <p3d/anim/vertexoffsetexpression.hpp>

const unsigned  EXPRESSION_VERSION     = 0;
const unsigned  EXPRESSION_GROUP_VERSION = 0;
const unsigned  EXPRESSION_MIXER_VERSION = 0;

//-------------------------------------------------------------------
const float tExpression::INVALID_EXPRESSION_KEYVAL = -2.0f;

tExpression::tExpression() :
    keyIndices(NULL), 
    keys(NULL),
    posKeys(NULL),
    negKeys(NULL),
    nKeys(0),
    nPosKeys(0),
    nNegKeys(0),
    posKeysExist(false),
    negKeysExist(false)
{
    //
}

tExpression::~tExpression()
{
    delete[] keyIndices;
    delete[] keys;
    delete[] posKeys;
    delete[] negKeys;

    keyIndices = NULL;
    keys = NULL;
    posKeys = NULL;
    negKeys = NULL;
}

int compareFloat(const void* f1, const void* f2)
{
    if (( *(float*)f1 - *(float*)f2  ) < 0)
        return -1;
    else if ( ( *(float*)f1 - *(float*)f2  ) > 0)
        return 1;
    else return 0;
}

int tExpression::GetKeyIndex(float keyVal)
{
    void* ptr = bsearch((const void*)&keyVal, (const void*)keys, nKeys, sizeof(float), compareFloat);
    P3DASSERT(ptr);  // bsearch returns null if val not found
    return ((float*)ptr - (float*)keys);
}

//-------------------------------------------------------------------
tExpressionLoader::tExpressionLoader() :
    tSimpleChunkHandler(Pure3D::Expression::VERTEX_EXPRESSION)
{
    //
}

tEntity* tExpressionLoader::LoadObject(tChunkFile* f, tEntityStore*)
{
    unsigned version = f->GetLong();
    P3DASSERT(version == EXPRESSION_VERSION);    
    
    char buf[256];
    f->GetPString(buf);

    tExpression* expr = new tExpression;
    expr->SetName(buf);

    unsigned i;
    expr->nKeys = f->GetLong();
    P3DASSERT(expr->nKeys >= 1);
    
    expr->keys = new float[expr->nKeys]; // allocate the keys array, 
    expr->keyIndices = new unsigned int[expr->nKeys];  // allocate the indices array
    
    
    int numNeg = 0;
    int numPos = 0;
    for ( i = 0; i < expr->nKeys; i++ ) 
    {
        expr->keys[i] = f->GetFloat();
        if( expr->keys[i] < 0 ) numNeg++; 
        if( expr->keys[i] > 0 ) numPos++; expr->posKeysExist = true;
    }

    numNeg? expr->negKeysExist = true: expr->negKeysExist = false;
    numPos? expr->posKeysExist = true: expr->posKeysExist = false;

    expr->nNegKeys = numNeg + 1; // additional element for 0.0
    expr->nPosKeys = numPos + 1;

    for ( i = 0; i < expr->nKeys; i++ ) //read in the offset list indices
    {
        expr->keyIndices[i] = f->GetUInt();
    }

    //read in all the keys and fill the positive and negative only key arrays
    expr->posKeys = new float[expr->nPosKeys];
    expr->negKeys = new float[expr->nNegKeys]; 

    int n = 0, p = 1;
    expr->posKeys[0] = 0.0f;
    //populate the posKeys and negKeys arrays
    for( i = 0; i < expr->nKeys; i++ )
    {
        if(expr->keys[i] < 0)
        {
            expr->negKeys[n++] = expr->keys[i];
        }
        else
        {
            expr->posKeys[p++] = expr->keys[i];
        }
    }
    expr->negKeys[n++] = 0.0f;

    return expr;
}

//-------------------------------------------------------------------
tExpressionGroup::tExpressionGroup() :
    tEntity(),
    nExpression(0),
    expression(0),
    target(0)
{
    //
}

tExpressionGroup::tExpressionGroup(int nExp) :
    tEntity(),
    nExpression(nExp),    
    stage(0),
    target(0)
{
    expression.SetSize( nExpression );
    stage.SetSize( nExpression );

    int i;
    for (i = 0; i < nExpression; i++)
    {      
        stage[i] = P3D_EXPRESSION_STAGE_1;
    }  
}

tExpressionGroup::~tExpressionGroup()
{
    int i;

    for (i = 0; i < nExpression; i++)
    {
        if (expression[i] != NULL)
        {
            expression[i]->Release();
            expression[i] = NULL;
        }
    }                
}


tExpression* tExpressionGroup::FindExpression(const tName& name, int* expIndex) const
{
    P3DASSERT(expIndex != NULL);

    for (int i = 0; i < nExpression; i++)
    {
        if (expression[i]->GetNameObject() == name)
        {
            *expIndex = i;
            return expression[i];
        }
    }

    *expIndex = -1;
    return NULL;
}


p3dExpressionStage tExpressionGroup::FindExpressionStage(const tName& name) const
{
    for (int i = 0; i < nExpression; i++)
    {
        if (expression[i]->GetNameObject() == name)
        {
            return stage[i];
        }
    }

    return P3D_EXPRESSION_STAGE_1;
}


tExpressionMixer::tExpressionMixer() :
    tEntity(),
    expressionGroup(NULL),
    state(NULL), 
    mixerReady(false)
{
    //
}

tExpressionMixer::~tExpressionMixer()
{
    if (expressionGroup != NULL)
    {
        expressionGroup->Release();
        expressionGroup = NULL;
    }
    
    if (state != NULL)
    {
        delete [] state;
        state = NULL;
    }
}

void tExpressionMixer::SetExpressionGroup(tExpressionGroup* eg)
{
    P3DASSERT(ValidateExpressionGroup(eg));
    
    if (state != NULL)
    {
        delete [] state;
        state = NULL;
    }

    tRefCounted::Assign(expressionGroup, eg);

    int nExp = GetNumExpression();

    state = new float[nExp]; 

    for (int i = 0; i < nExp; i++)
    {
        state[i] = 0.0f;

    }

    mixerReady = false;
    return;
}


void tExpressionMixer::SetExpressionState(const tName& name, const float s)
{
    P3DASSERT(GetNumExpression() != 0);

    int expIndex = -1;
    tExpression* tmp = NULL;

    tmp = expressionGroup->FindExpression(name, &expIndex);

    P3DASSERT((tmp != NULL) &&
                 (expIndex != -1));

    if (state[expIndex] != s)
    {
        state[expIndex] = s;
        mixerReady = false;
    }
}


float tExpressionMixer::GetExpressionState(const tName& name) const
{
    P3DASSERT(GetNumExpression() != 0);

    int expIndex = -1;
    tExpression* tmp = NULL;
    tmp = expressionGroup->FindExpression(name, &expIndex);
    P3DASSERT((tmp != NULL) && (expIndex != -1));

    return state[expIndex];
}


void tExpressionMixer::ResetToNeutral()
{
    for (int i = 0; i < GetNumExpression(); i++)
    {
        state[i] = 0.0f; 
    }

    mixerReady = false;
}


tEntity* tExpressionGroupLoader::LoadObject(tChunkFile* f,tEntityStore* store)
{
    unsigned version = f->GetLong();
    P3DASSERT(version == EXPRESSION_GROUP_VERSION);

    char name[256];
    f->GetPString(name);

    char targetname[256];
    f->GetPString(targetname);

    int nExp = f->GetLong();

    tExpressionGroup* expressionGroup = new tExpressionGroup(nExp);
    expressionGroup->SetName(name);
    expressionGroup->target = tEntity::MakeUID(targetname);

    for (int i = 0; i < nExp; i++)
    {
        expressionGroup->stage[i] = (p3dExpressionStage) f->GetLong();
    }

    int np = 0;
    int ne = 0;
    while (f->ChunksRemaining())
    {
        switch(f->BeginChunk())
        {
            case Pure3D::Expression::VERTEX_EXPRESSION:           
            {
                tP3DFileHandler* p3d = p3d::context->GetLoadManager()->GetP3DHandler();
                tChunkHandler* handler = p3d->GetHandler(f->GetCurrentID());
                if (handler != NULL)
                {
                    P3DASSERT(ne < expressionGroup->nExpression);

                    // create a specialized store that will not put the
                    // expressions in the Pure3D inventory, but rather will
                    // put them in the expression group's array, when the
                    // handler calls store->Store();
                    tExpressionStore* expStore = new tExpressionStore;
                    expStore->AddRef();

                    // pass the store that was given to the expression loader
                    // into the new store so that if the handler calls p3d::find
                    // it will look for whatever it needs in the Pure3D inventory
                    expStore->SetPrimaryStore(store);

                    // pass the store the address we want the expression stored
                    // in
                    expStore->SetExpStore(&(expressionGroup->expression[ne]));
                    
                    tLoadStatus rc = handler->Load(f, expStore);
                    expStore->Release();
                    if (rc != LOAD_OK)
                    {
                        status = rc;
                        return NULL;
                    }
                    ne++;
                }
                break;
            }

            default:
                break;

        }// switch ID
        f->EndChunk();
    } // while !EndOfChunk

    return expressionGroup;
}

tExpressionGroupLoader::tExpressionGroupLoader() :
    tSimpleChunkHandler(Pure3D::Expression::VERTEX_EXPRESSION_GROUP)
{
    //
}

tExpressionGroupLoader::tExpressionStore::tExpressionStore() :
    storedExp(NULL),
    primaryStore(NULL)
{
    //
}

tExpressionGroupLoader::tExpressionStore::~tExpressionStore()
{
    // do not delete, as this is held elsewhere
    storedExp = NULL;

    if (primaryStore != NULL)
    {
        primaryStore->Release();
        primaryStore = NULL;
    }
}

IRefCount* tExpressionGroupLoader::tExpressionStore::Find(radLoadInventory::SafeCastBase& c, const tUID uid)
{
    P3DASSERT(primaryStore != NULL);
    return static_cast<tEntity*>(primaryStore->Find(c, uid));
}


void tExpressionGroupLoader::tExpressionStore::Store(tEntity* obj)
{
    P3DASSERT(storedExp != NULL);
    tExpression* exp = dynamic_cast<tExpression*>(obj);
    tRefCounted::Assign(*storedExp,exp);
}

void tExpressionGroupLoader::tExpressionStore::StoreHandlingCollisions(tEntity* obj)
{
    P3DASSERT(storedExp != NULL);
    tExpression* exp = dynamic_cast<tExpression*>(obj);
    tRefCounted::Assign(*storedExp,exp);
}

tExpressionMixerLoader::tExpressionMixerLoader() :
    tSimpleChunkHandler(Pure3D::Expression::VERTEX_EXPRESSION_MIXER)
{
    //
}

tEntity* tExpressionMixerLoader::LoadObject(tChunkFile* f, tEntityStore* store)
{
    char mname[256]; // mixer name
    char tname[256]; // target name
    char egname[256]; // expression group name
    MixerType type;

    unsigned version = f->GetLong(); // Version
    P3DASSERT(version == EXPRESSION_MIXER_VERSION);

    f->GetPString(mname);
    type = (MixerType) f->GetLong();
    f->GetPString(tname);
    f->GetPString(egname);

    if(type != VertexOffset)
    {
        status = LOAD_ERROR;
        return NULL;
    }

    tEntity* obj = NULL;

    switch (type)
    {

        case Undefined:
            P3DASSERTMSG(0, "Invalid Mixer chunk type", "tExpressionMixerLoader");
            break;

        case VertexOffset:
        {
            tVertexOffsetExpressionMixer* vm = new tVertexOffsetExpressionMixer;
            vm->SetName(mname);
            tExpressionGroup* eg = p3d::find<tExpressionGroup>(store, egname);
            tDrawable* drawable = p3d::find<tDrawable>(store, tname);
            if ( eg == NULL || drawable == NULL )
            {
                if(eg == NULL)
                {
                    p3d::printf("Warning : can't find expression group '%s' for mixer '%s'\n", egname, mname);
                }

                if(drawable == NULL)
                {
                    p3d::printf("Warning : can't find drawable '%s' for mixer '%s'\n", tname, mname);
                }

                vm->Release();
                vm = NULL;
            }
            else
            {
                vm->SetExpressionGroup( eg );
                vm->SetTarget( drawable );
                obj = vm;
            }
        }
        break;
        
        default:
            break;
    }

    return obj;
}
