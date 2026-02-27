/*===========================================================================
    particleloader.hpp
    28-Feb-2001 Created by Kevin Voon

    Copyright (c)2000 Radical Entertainment, Inc.
    All rights reserved.
===========================================================================*/
#ifndef _PARTICLE_LOADER_HPP
#define _PARTICLE_LOADER_HPP

#include <p3d/loadmanager.hpp>

class tChunkFile;
class tAnimation;
class tBaseGeneratorFactory;
class tPointGeneratorFactory;
class tPlaneGeneratorFactory;
class tSphereGeneratorFactory;
class tBaseEmitterFactory;
class tSpriteEmitterFactory;

//*****************************************************************************
//
// Class tGeneratorFactoryLoader
//
//*****************************************************************************
class tGeneratorFactoryLoader
{
public:
    tBaseGeneratorFactory* LoadGeneratorFactory(unsigned generatorType, tChunkFile* file, tEntityStore* store);
};

//*****************************************************************************
//
// Class tEmitterFactoryLoader
//
//*****************************************************************************
class tEmitterFactoryLoader
{
public:
    tBaseEmitterFactory* LoadEmitterFactory(tChunkFile* file, tEntityStore* store);

protected:
    void LoadBaseEmitterFactoryData(tChunkFile* file, tEntityStore* store, tBaseEmitterFactory* factory);
    void LoadSpriteEmitterFactoryData(tChunkFile* file, tEntityStore* store, tSpriteEmitterFactory* factory);
    void LoadBaseParticleData(tAnimation* particleAnim, tBaseEmitterFactory* factory);
    void LoadSpriteParticleData(tAnimation* particleAnim, tSpriteEmitterFactory* factory);
};

//*****************************************************************************
//
// Class tParticleSystemFactoryLoader
//
//*****************************************************************************
class tParticleSystemFactoryLoader : public tSimpleChunkHandler
{
public:
    tParticleSystemFactoryLoader();
    tEntity* LoadObject(tChunkFile* chunk, tEntityStore* store);
protected:   
    ~tParticleSystemFactoryLoader() {};

};

//*****************************************************************************
//
// Class tParticleSystemLoader
//
//*****************************************************************************
class tParticleSystemLoader : public tSimpleChunkHandler
{
public:
    tParticleSystemLoader();
    tEntity* LoadObject(tChunkFile* chunk, tEntityStore* store);
protected:   
    ~tParticleSystemLoader() {};

};

#endif

