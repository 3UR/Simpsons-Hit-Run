/*****************************************************************************
    particleloader.cpp
    28-Feb-2001 Created by Kevin Voon

    Copyright (c)2000 Radical Entertainment, Inc.
    All rights reserved.
*****************************************************************************/
#include <p3d/utility.hpp>
#include <p3d/inventory.hpp>
#include <constants/chunks.h>
#include <constants/chunkids.hpp>
#include <p3d/chunkfile.hpp>
#include <p3d/error.hpp>
#include <p3d/shader.hpp>
#include <p3d/anim/animate.hpp>
#include <p3d/anim/channel.hpp>
#include <string.h>

#include <p3d/effects/particleconstants.hpp>
#include <p3d/effects/particlepool.hpp>
#include <p3d/effects/particlegenerator.hpp>
#include <p3d/effects/particleemitter.hpp>
#include <p3d/effects/particlesystem.hpp>
#include <p3d/effects/particleutility.hpp>
#include <p3d/effects/particleloader.hpp>

//*****************************************************************************
//
// Class tGeneratorFactoryLoader
//
//*****************************************************************************
tBaseGeneratorFactory* tGeneratorFactoryLoader::LoadGeneratorFactory(unsigned generatorType, tChunkFile* file, tEntityStore* store)
{
    P3DASSERT(file->GetCurrentID() == Pure3D::ParticleSystem::GENERATOR_ANIMATION);

    unsigned version = file->GetLong();
    P3DASSERT(version == GENERATOR_ANIMATION_VERSION);

    tBaseGeneratorFactory* factory = NULL;
    switch (generatorType)
    {
        case p3dParticleSystemConstants::GeneratorType::POINT:
        {
            factory = new tPointGeneratorFactory;
            break;
        }
        case p3dParticleSystemConstants::GeneratorType::PLANE:
        {
            factory = new tPlaneGeneratorFactory;
            break;
        }
        case p3dParticleSystemConstants::GeneratorType::SPHERE:
        {
            factory = new tSphereGeneratorFactory;
            break;
        }
        default:
        {
            return NULL;
            break;
        }
    }
    
    tAnimationLoader* loader = dynamic_cast<tAnimationLoader*>(p3d::loadManager->GetP3DHandler()->GetHandler(Pure3D::Animation::AnimationData::ANIMATION));
    loader->AddRef();

    int sortOrder = loader->GetSortOrder();
    loader->SetSortOrder(0);

    while(file->ChunksRemaining())
    {
        switch(file->BeginChunk())
        {
            case Pure3D::Animation::AnimationData::ANIMATION:
            {   
                tAnimation* anim = (tAnimation*)loader->LoadObject(file,store);
                tRefCounted::Assign(factory->generatorAnim,anim);
                factory->CopyName(anim);
                break;
            }

            default:
                break;

        } // switch chunkID
        file->EndChunk();
    }

    loader->SetSortOrder(sortOrder);

    loader->Release();

    return factory;    
}

//*****************************************************************************
//
// Class tEmitterFactoryLoader
//
//*****************************************************************************
tBaseEmitterFactory* tEmitterFactoryLoader::LoadEmitterFactory(tChunkFile* file, tEntityStore* store)
{
    switch(file->GetCurrentID())
    {
        case Pure3D::ParticleSystem::SPRITE_EMITTER_FACTORY:
        {
            tSpriteEmitterFactory* factory = new tSpriteEmitterFactory;
            LoadSpriteEmitterFactoryData(file,store,factory);
            return factory;
            break;
        }
        default:
        {
            return NULL;
            break;
        }
    }
}

void tEmitterFactoryLoader::LoadBaseEmitterFactoryData(tChunkFile* file, tEntityStore* store, tBaseEmitterFactory* factory)
{
    P3DASSERT(file->GetCurrentID()==Pure3D::ParticleSystem::BASE_EMITTER_FACTORY);

    unsigned version = file->GetLong();
    P3DASSERT(version == BASE_PARTICLE_EMITTER_FACTORY_VERSION);

    char name[256];
    file->GetPString(name);
    factory->SetName(name);
    factory->particleType = file->GetLong();
    factory->generatorType = file->GetLong();
    factory->zTest = file->GetLong()==1;
    factory->zWrite = file->GetLong()==1;
    factory->fog = file->GetLong()==1;
    factory->maxNumParticles = file->GetLong();
    factory->infiniteLife = file->GetLong()==1;
    factory->rotationalCohesion = file->GetFloat();
    factory->translationalCohesion = file->GetFloat();

    tAnimationLoader* animLoader = dynamic_cast<tAnimationLoader*>(p3d::loadManager->GetP3DHandler()->GetHandler(Pure3D::Animation::AnimationData::ANIMATION));
    animLoader->AddRef();

    int sortOrder = animLoader->GetSortOrder();
    animLoader->SetSortOrder(0);

    while(file->ChunksRemaining())
    {
        switch(file->BeginChunk())
        {
            case Pure3D::ParticleSystem::PARTICLE_ANIMATION:
            {
                version = file->GetLong();
                P3DASSERT(version == PARTICLE_ANIMATION_VERSION);

                file->BeginChunk();
                tAnimation* anim = (tAnimation*)animLoader->LoadObject(file,store);
                if (anim)
                {
                    anim->AddRef();
                    switch (factory->particleType)
                    {
                        case p3dParticleSystemConstants::ParticleType::SPRITE:
                        {
                            LoadSpriteParticleData(anim,(tSpriteEmitterFactory*)factory);
                            break;
                        }
                    }                    
                    anim->Release();
                }
                file->EndChunk();
                break;
            }
            case Pure3D::ParticleSystem::EMITTER_ANIMATION:
            {
                version = file->GetLong();
                P3DASSERT(version == EMITTER_ANIMATION_VERSION);

                file->BeginChunk();
                tAnimation* anim = (tAnimation*)animLoader->LoadObject(file,store);
                tRefCounted::Assign(factory->emitterAnim,anim);
                file->EndChunk();
                break;
            }
            case Pure3D::ParticleSystem::GENERATOR_ANIMATION:
            {
                tGeneratorFactoryLoader genLoader;
                tBaseGeneratorFactory* genFactory = genLoader.LoadGeneratorFactory(factory->generatorType,file,store);
                tRefCounted::Assign(factory->generatorFactory,genFactory);
                break;
            }
        }
        file->EndChunk();
    }

    animLoader->SetSortOrder(sortOrder);

    animLoader->Release();
}
    
void tEmitterFactoryLoader::LoadSpriteEmitterFactoryData(tChunkFile* file, tEntityStore* store, tSpriteEmitterFactory* factory)
{
    P3DASSERT(file->GetCurrentID()==Pure3D::ParticleSystem::SPRITE_EMITTER_FACTORY);

    unsigned version = file->GetLong();
    P3DASSERT(version == SPRITE_PARTICLE_EMITTER_FACTORY_VERSION);

    char buf[256];
    file->GetPString(buf);
    factory->SetName(buf);

    file->GetPString(buf);
    tShader* shader = p3d::find<tShader>(store, buf);

    tRefCounted::Assign(factory->shader, shader);
    factory->angleMode = file->GetLong();
    factory->angle = file->GetFloat();
    factory->texAnimMode = file->GetLong();
    factory->numTexFrames = file->GetLong();
    factory->texFrameRate = file->GetLong();

    P3DASSERT(factory->numTexFrames>0);
    float inc = 1.0f/(float)factory->numTexFrames;

    factory->texFrameCoords = new float[factory->numTexFrames+1];
    for (int i = 0; i <= factory->numTexFrames; i++)
    {
        factory->texFrameCoords[i] = i*inc;
    }

    while(file->ChunksRemaining())
    {
        switch(file->BeginChunk())
        {
            case Pure3D::ParticleSystem::BASE_EMITTER_FACTORY:
            {
                LoadBaseEmitterFactoryData(file, store, factory);
                break;
            }
        }
        file->EndChunk();
    }
}

void tEmitterFactoryLoader::LoadBaseParticleData(tAnimation* particleAnim, tBaseEmitterFactory* factory)
{
    if ((particleAnim)&&(factory))
    {
        const tFloat1Channel* floatchannel = NULL;
        floatchannel = particleAnim->GetGroupByIndex(0)->GetFloat1Channel(Pure3DAnimationChannels::ParticleSystem::Particle::SPEED_OVER_LIFE_SPOL);
        if (floatchannel)
        {
            float currentFrame = 0.0f;
            float frameInc = particleAnim->GetNumFrames()/rmt::LtoF(DEFAULT_LOOK_UP_SIZE);

            for (int i = 0; i < DEFAULT_LOOK_UP_SIZE; i++)
            {
                floatchannel->GetValue(currentFrame,&(factory->baseLookUp[i].speed));
                currentFrame += frameInc;
            }      
        }
        floatchannel = particleAnim->GetGroupByIndex(0)->GetFloat1Channel(Pure3DAnimationChannels::ParticleSystem::Particle::WEIGHT_OVER_LIFE_WEOL);
        if (floatchannel)
        {
            float currentFrame = 0.0f;
            float frameInc = particleAnim->GetNumFrames()/rmt::LtoF(DEFAULT_LOOK_UP_SIZE);

            for (int i = 0; i < DEFAULT_LOOK_UP_SIZE; i++)
            {
                floatchannel->GetValue(currentFrame,&(factory->baseLookUp[i].weight));
                currentFrame += frameInc;
            }      
        }
    }
}

void tEmitterFactoryLoader::LoadSpriteParticleData(tAnimation* particleAnim, tSpriteEmitterFactory* factory)
{
    if ((particleAnim)&&(factory))
    {
        const tIntChannel*    intchannel = NULL;
        const tFloat1Channel* floatchannel = NULL;
        const tColourChannel* colourchannel = NULL;

        colourchannel = particleAnim->GetGroupByIndex(0)->GetColourChannel(Pure3DAnimationChannels::ParticleSystem::Particle::COLOUR_OVER_LIFE_COOL);
        if (colourchannel)
        {
            float frameInc = particleAnim->GetNumFrames()/rmt::LtoF(DEFAULT_LOOK_UP_SIZE);
            float currentFrame = 0.0f;

            for (int i = 0; i < DEFAULT_LOOK_UP_SIZE; i++)
            {
                colourchannel->GetValue(currentFrame,&(factory->spriteLookUp[i].colour));
                currentFrame += frameInc;
            }
            
            factory->colourAnimMode = p3dParticleSystemConstants::ColourAnimMode::COLOUR;
        }
        intchannel = particleAnim->GetGroupByIndex(0)->GetIntChannel(Pure3DAnimationChannels::ParticleSystem::Particle::ALPHA_OVER_LIFE_ALOL);
        if (intchannel)
        {
            float currentFrame = 0.0f;
            float frameInc = particleAnim->GetNumFrames()/rmt::LtoF(DEFAULT_LOOK_UP_SIZE);

            for (int i = 0; i < DEFAULT_LOOK_UP_SIZE; i++)
            {
                int alpha;
                intchannel->GetValue(currentFrame,&alpha);
                factory->spriteLookUp[i].colour.SetAlpha(alpha);
                currentFrame += frameInc;
            }

            if (factory->colourAnimMode == p3dParticleSystemConstants::ColourAnimMode::COLOUR)
            {
               factory->colourAnimMode = p3dParticleSystemConstants::ColourAnimMode::BOTH;
            }
            else
            {
                factory->colourAnimMode = p3dParticleSystemConstants::ColourAnimMode::ALPHA;
            }
        }
        floatchannel = particleAnim->GetGroupByIndex(0)->GetFloat1Channel(Pure3DAnimationChannels::ParticleSystem::Particle::SIZE_OVER_LIFE_SZOL);
        if (floatchannel)
        {
            float currentFrame = 0.0f;
            float frameInc = particleAnim->GetNumFrames()/rmt::LtoF(DEFAULT_LOOK_UP_SIZE);

            for (int i = 0; i < DEFAULT_LOOK_UP_SIZE; i++)
            {
                floatchannel->GetValue(currentFrame,&(factory->spriteLookUp[i].size));
                currentFrame += frameInc;
            }      
        }
        if (factory->GetAngleMode()!=p3dParticleSystemConstants::ParticleAngleMode::MOTION_ALIGNED)
        {            
            floatchannel = particleAnim->GetGroupByIndex(0)->GetFloat1Channel(Pure3DAnimationChannels::ParticleSystem::Particle::SPIN_OVER_LIFE_SNOL);
            if (floatchannel)
            {
                float currentFrame = 0.0f;
                float frameInc = particleAnim->GetNumFrames()/rmt::LtoF(DEFAULT_LOOK_UP_SIZE);

                for (int i = 0; i < DEFAULT_LOOK_UP_SIZE; i++)
                {
                    floatchannel->GetValue(currentFrame,&(factory->spriteLookUp[i].spin));
                    currentFrame += frameInc;
                }
            }
        }
        if (factory->GetTextureAnimationMode()==p3dParticleSystemConstants::TextureAnimMode::SPECIFIED)
        {
            intchannel = particleAnim->GetGroupByIndex(0)->GetIntChannel(Pure3DAnimationChannels::ParticleSystem::Particle::TEXTURE_OVER_LIFE_TEOL);
            if (intchannel)
            {
                float currentFrame = 0.0f;
                float frameInc = particleAnim->GetNumFrames()/rmt::LtoF(DEFAULT_LOOK_UP_SIZE);

                for (int i = 0; i < DEFAULT_LOOK_UP_SIZE; i++)
                {
                    intchannel->GetValue(currentFrame,&(factory->spriteLookUp[i].texFrame));
                    currentFrame += frameInc;
                }
            }
        }
        LoadBaseParticleData(particleAnim,factory);
    }
}

//*****************************************************************************
//
// Class tParticleSystemFactoryLoader
//
//*****************************************************************************
tParticleSystemFactoryLoader::tParticleSystemFactoryLoader() : tSimpleChunkHandler(Pure3D::ParticleSystem::SYSTEM_FACTORY)
{
}

tEntity* tParticleSystemFactoryLoader::LoadObject(tChunkFile* f, tEntityStore* store)
{
    tParticleSystemFactory* factory = new tParticleSystemFactory;

    // version
    unsigned version = f->GetLong();
    P3DASSERT(version == PARTICLE_SYSTEM_FACTORY_VERSION);

    // name
    char name[128];
    f->GetPString( name );
    factory->SetName( name );   

    // frame rate
    factory->frameTime = 1000.0f/f->GetFloat();
    
    // number of frames in animation
    factory->numFrames = (float)f->GetLong();

    // number of frames in over life animation
    f->GetLong();

    // cycle anim flag
    factory->cycleAnim = f->GetWord()==1;

    // enable emitter sorting flag
    factory->enableSorting = f->GetWord()==1;
   
    // number of emitters
    factory->numEmitters = f->GetLong();

    if(factory->numEmitters > 0)
    {
        factory->emitters = new tBaseEmitterFactory*[factory->numEmitters];
   
        int currEmitter = 0;    
        while( f->ChunksRemaining() )
        {
            switch ( f->BeginChunk() )
            {
                case Pure3D::ParticleSystem::INSTANCING_INFO:
                {
                    version = f->GetLong();
                    factory->maxInstances = f->GetLong();
                    break;
                }              
                case Pure3D::ParticleSystem::SPRITE_EMITTER_FACTORY:
                {
                    if (currEmitter<factory->numEmitters)
                    {
                        tEmitterFactoryLoader loader;
                        factory->emitters[currEmitter] = loader.LoadEmitterFactory(f, store);
                        P3DASSERT(factory->emitters[currEmitter] != NULL);
                        factory->emitters[currEmitter]->AddRef();
                        factory->emitters[currEmitter]->particlePool = new tParticlePool(factory->emitters[currEmitter]->GetParticleType(), factory->emitters[currEmitter]->GetMaxNumParticles(), factory->GetMaxInstances());                        
                        factory->emitters[currEmitter]->particlePool->AddRef();
                        currEmitter++;
                    }
                    break;
                }
                default:
                    break;    
            }
            f->EndChunk();
        }
    }
    
    return factory;
}

//*****************************************************************************
//
// Class tParticleSystemLoader
//
//*****************************************************************************
tParticleSystemLoader::tParticleSystemLoader() : tSimpleChunkHandler(Pure3D::ParticleSystem::SYSTEM)
{
}

tEntity* tParticleSystemLoader::LoadObject(tChunkFile* f, tEntityStore* store)
{   
    // version
    unsigned version = f->GetLong();
    P3DASSERT(version == PARTICLE_SYSTEM_VERSION);

    char name[256];
    f->GetPString( name );
    
    char factoryName[256];
    f->GetPString( factoryName );

    tParticleSystemFactory* factory = p3d::find<tParticleSystemFactory>(store, factoryName);

    if (factory)
    {
        tParticleSystem* system = (tParticleSystem*)factory->CreateEffect();
        system->SetName(name);
        return system;
    }
    return NULL;
    
}

