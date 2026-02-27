//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include <p3d/light.hpp>
#include <p3d/chunkfile.hpp>
#include <constants/chunks.h>
#include <constants/chunkids.hpp>
#include <p3d/directionallight.hpp>
#include <p3d/spotlight.hpp>
#include <p3d/pointlight.hpp>
#include <p3d/ambientlight.hpp>
#include <p3d/inventory.hpp>
#include <p3d/error.hpp>
#include <p3d/utility.hpp>

tLightLoader::tLightLoader() : tSimpleChunkHandler(Pure3D::Light::LIGHT)
{
}

tEntity* tLightLoader::LoadObject(tChunkFile* f, tEntityStore* store)
{
    char name[256];
    f->GetPString(name);
    unsigned long version = f->GetLong();
    unsigned long type = f->GetLong();
    
    tColour colour(f->GetLong());
    
    float constant = f->GetFloat();
    float linear   = f->GetFloat();
    float squared  = f->GetFloat();

    bool enabled = (f->GetLong() != 0);

    tLight *light = NULL;
    rmt::Vector dir;
    rmt::Vector pos;
    float theta = 0.0f, phi = 0.0f, range = 0.0f, falloff = 0.0f;
    tLight::DecayRange::Type decayType = tLight::DecayRange::NO_DECAY_RANGE;
    rmt::Vector innerRange(0.0f, 0.0f, 0.0f);
    rmt::Vector outerRange(0.0f, 0.0f, 0.0f);
    float rotationY = 0.0f;
    bool isShadowCaster = false;
    tLight::IlluminationType illuminationType = tLight::POSITIVE_ILLUMINANT;

    while(f->ChunksRemaining())
    {
        f->BeginChunk();
        
        switch(f->GetCurrentID())
        {
            case Pure3D::Light::DIRECTION:
                {
                    dir.x = f->GetFloat();
                    dir.y = f->GetFloat();
                    dir.z = f->GetFloat();
                }
                break;
            case Pure3D::Light::POSITION:
                {
                    pos.x = f->GetFloat();
                    pos.y = f->GetFloat();
                    pos.z = f->GetFloat();
                }
                break;
            case Pure3D::Light::CONE_PARAM:
                {
                    phi = f->GetFloat();
                    theta = f->GetFloat();
                    falloff = f->GetFloat();
                    range = f->GetFloat();

                }
                break;
            case Pure3D::Light::DECAY_RANGE:
                {
                    unsigned long type = f->GetLong();
                    switch(type) 
                    {
                    case 1:
                        {
                            decayType = tLight::DecayRange::SPHERE_DECAY_RANGE;
                        }
                        break;
                    case 2:
                        {
                            decayType = tLight::DecayRange::CUBOID_DECAY_RANGE;
                        }
                        break;
                    case 3:
                        {
                            decayType = tLight::DecayRange::ELLIPSOID_DECAY_RANGE;
                        }
                        break;
                    case 0:
                    default:
                        {
                            decayType = tLight::DecayRange::NO_DECAY_RANGE;
                        }
                        break;
                    }
                    innerRange.x = f->GetFloat();
                    innerRange.y = f->GetFloat();
                    innerRange.z = f->GetFloat();
                    outerRange.x = f->GetFloat();
                    outerRange.y = f->GetFloat();
                    outerRange.z = f->GetFloat();
                    while(f->ChunksRemaining())
                    {
                        f->BeginChunk();
                        
                        switch(f->GetCurrentID())
                        {
                        case Pure3D::Light::DECAY_RANGE_ROTATION_Y:
                            {
                                rotationY = f->GetFloat();
                            }
                            break;
                        }
                        f->EndChunk();
                    }
                }
                break;
            case Pure3D::Light::SHADOW:
            {
                isShadowCaster = f->GetLong() == 1;
            }
            break;
            
            case Pure3D::Light::ILLUMINATION_TYPE:
            {
                unsigned long type = f->GetLong();
                switch(type) 
                {
                    case 1:
                    {
                        illuminationType = tLight::ZERO_ILLUMINANT;
                    }
                    break;
                    case 2:
                    {
                        illuminationType = tLight::NEGATIVE_ILLUMINANT;
                    }
                    break;
                    default:
                    {
                        illuminationType = tLight::POSITIVE_ILLUMINANT;
                    }
                    break;
                }
            }
        };

        f->EndChunk();

    }

    switch (type)
    {
    case 0:    //ambient light
        {
            tAmbientLight* al = new tAmbientLight;
            al->SetName(name);
            al->SetColour(colour);
            al->SetPosition(pos);
            al->Enable(enabled);
            al->SetDecayType(decayType);
            al->SetDecayRange(innerRange, outerRange);
            al->SetDecayRotationY(rotationY);
            al->SetIlluminationType(illuminationType);

            light = al;
        }
        break;
    case 1:  //point light
        {
            tPointLight* pl = new tPointLight;
            pl->SetName(name);
            pl->SetColour(colour);
            pl->SetPosition(pos.x, pos.y, pos.z);
            pl->Enable(enabled);
            pl->SetAttenuation(constant, linear, squared);
            pl->SetDecayType(decayType);
            pl->SetDecayRange(innerRange, outerRange);
            pl->SetShadowCaster(isShadowCaster);
            pl->SetDecayRotationY(rotationY);
            pl->SetIlluminationType(illuminationType);

            light = pl;
        }
        break;
    case 2:  //directional light
        {
            tDirectionalLight *dl = new tDirectionalLight;
            dl->SetName(name);
            dl->SetColour(colour);
            dl->SetPosition(pos);
            dl->SetDirection(dir.x, dir.y, dir.z);
            dl->Enable(enabled);
            dl->SetDecayType(decayType);
            dl->SetDecayRange(innerRange, outerRange);
            dl->SetShadowCaster(isShadowCaster);
            dl->SetDecayRotationY(rotationY);
            dl->SetIlluminationType(illuminationType);
 
            light = dl;
          break;
        }
        break;
    case 3:  //spotlight
        {
            tSpotLight* sl = new tSpotLight;
            sl->SetName(name);
            sl->SetColour(colour);
            sl->SetDirection(dir.x, dir.y, dir.z);
            sl->SetPosition(pos.x, pos.y, pos.z);
            sl->SetCone(phi, theta, falloff);
            sl->SetRange(range);
            sl->Enable(enabled);
            sl->SetAttenuation(constant, linear, squared);
            sl->SetDecayType(decayType);
            sl->SetDecayRange(innerRange, outerRange);
            sl->SetShadowCaster(isShadowCaster);
            sl->SetDecayRotationY(rotationY);            
            sl->SetIlluminationType(illuminationType);

            light = sl; 
        }
        break;
        
    };
    
    return light;
}

//--------------------------------------------------
tLightGroupLoader::tLightGroupLoader() : tSimpleChunkHandler(P3D_LIGHT_GROUP)
{
}

//--------------------------------------------------
tEntity* tLightGroupLoader::LoadObject(tChunkFile* f, tEntityStore* store)
{
    char name[256];

    tLightGroup *lg = new tLightGroup;

    f->GetPString(name);
    lg->SetName(name);  
 
    lg->mNumLights = f->GetLong();
    lg->mCurNumLights = lg->mNumLights;

    lg->mLights.SetSize( lg->mNumLights );

    int a;
    for (a = 0; a < lg->mNumLights; a++)
    {
        f->GetPString(name);
        lg->mLights[a] = p3d::find<tLight>(store, name);
        if(lg->mLights[a])
            lg->mLights[a]->AddRef();
    }
        
    return lg;
}

