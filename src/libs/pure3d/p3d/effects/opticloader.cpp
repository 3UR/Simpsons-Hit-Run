/*===========================================================================
    opticloader.cpp

    Copyright (c)2000 Radical Entertainment, Inc.
    All rights reserved.
===========================================================================*/
#include <p3d/utility.hpp>
#include <p3d/inventory.hpp>
#include <constants/chunks.h>
#include <constants/chunkids.hpp>
#include <p3d/chunkfile.hpp>
#include <p3d/error.hpp>
#include <p3d/shader.hpp>
#include <p3d/anim/channel.hpp>
#include <string.h>

#include <p3d/effects/opticconstants.hpp>
#include <p3d/effects/optic.hpp>
#include <p3d/effects/opticlensflare.hpp>
#include <p3d/effects/opticloader.hpp>

using namespace RadicalMathLibrary;

//*****************************************************************************
//
// Class tLensFlareGroupLoader
//
//*****************************************************************************
tLensFlareGroupLoader::tLensFlareGroupLoader() : tSimpleChunkHandler(Pure3D::OpticEffect::LENS_FLARE_GROUP)
{
}

tEntity* tLensFlareGroupLoader::LoadObject(tChunkFile* f, tEntityStore* store)
{
    int version = f->GetLong();
    P3DASSERT(version == LENS_FLARE_GROUP_VERSION);

    char Name[256];
    f->GetPString(Name);

    char shaderName[256];
    f->GetPString(shaderName);

    tShader* shader = p3d::find<tShader>(store, shaderName);
    if (shader)
    {
        shader->AddRef();
    }
   
    tLensFlareGroup* group = new tLensFlareGroup();
    group->SetName(Name);
    group->shader = shader;
    group->SetZTest(f->GetLong()==1);
    group->SetZWrite(f->GetLong()==1);
    group->SetFog(f->GetLong()==1);
    group->SetSourceRadius(f->GetFloat());
    group->SetEdgeRadius(f->GetFloat());

    group->numFlares = f->GetLong();
    group->flares.SetSize( group->numFlares );

    // load lensflare-specific attributes
    int flareIndex = 0;
    while (f->ChunksRemaining())
    {
        switch (f->BeginChunk())
        {
            case Pure3D::OpticEffect::LENS_FLARE:
            {
                group->flares[flareIndex] = LoadLensFlare(f,store);
                group->flares[flareIndex]->AddRef();
                ++flareIndex;
                break;
            }
            default:
            {
                break;
            }
        }
        f->EndChunk();
    }

    P3DASSERT(flareIndex == group->numFlares);

    return group;
}

tLensFlare* tLensFlareGroupLoader::LoadLensFlare(tChunkFile* f, tEntityStore* store)
{
    int version = f->GetLong();
    P3DASSERT(version == LENS_FLARE_VERSION);

    char Name[256];
    f->GetPString(Name);

    tLensFlare* flare = new tLensFlare();
    flare->SetName(Name);
    flare->SetColour(tColour(f->GetLong()));
    flare->SetUV(0,Vector2(f->GetFloat(),f->GetFloat()));
    flare->SetUV(1,Vector2(f->GetFloat(),f->GetFloat()));
    flare->SetUV(2,Vector2(f->GetFloat(),f->GetFloat()));
    flare->SetUV(3,Vector2(f->GetFloat(),f->GetFloat()));
    flare->SetUVOffset(Vector2(f->GetFloat(),f->GetFloat()));
    flare->SetDistance(f->GetFloat());
    flare->SetWidth(f->GetFloat());
    flare->SetHeight(f->GetFloat());

    return flare;
}
