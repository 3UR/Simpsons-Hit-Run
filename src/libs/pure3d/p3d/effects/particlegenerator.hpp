/*===========================================================================
    particlegenerator.hpp
    28-Feb-2001 Created by Kevin Voon

    Copyright (c)2000 Radical Entertainment, Inc.
    All rights reserved.
===========================================================================*/
#ifndef _PARTICLE_GENERATOR_HPP
#define _PARTICLE_GENERATOR_HPP

#include <radmath/radmath.hpp>
#include <radmath/random.hpp>
#include <p3d/entity.hpp>

class tBaseEmitter;
class tBaseGenerator;
class tGeneratorFactoryLoader;
class tAnimation;
class tFloat1Channel;

//*****************************************************************************
//
// Class tBaseGeneratorFactory 
//
//*****************************************************************************
class tBaseGeneratorFactory : public tEntity
{
public:
    virtual tBaseGenerator* CreateGenerator() = 0;

    unsigned GetGeneratorType() const                   { return generatorType; }

    tAnimation* GetGeneratorAnimation() const           { return generatorAnim; }

    friend class tGeneratorFactoryLoader;

protected:
    tBaseGeneratorFactory();
    virtual ~tBaseGeneratorFactory();

    unsigned                generatorType;
    tAnimation*             generatorAnim;
};

//*****************************************************************************
//
// Class tPointGeneratorFactory 
//
//*****************************************************************************
class tPointGeneratorFactory : public tBaseGeneratorFactory
{
public:
    virtual tBaseGenerator* CreateGenerator();

    friend class tGeneratorFactoryLoader;

protected:
    tPointGeneratorFactory();
    virtual ~tPointGeneratorFactory();
};

//*****************************************************************************
//
// Class tPlaneGeneratorFactory 
//
//*****************************************************************************
class tPlaneGeneratorFactory : public tBaseGeneratorFactory
{
public:
    virtual tBaseGenerator* CreateGenerator();

    friend class tGeneratorFactoryLoader;

protected:
    tPlaneGeneratorFactory();
    virtual ~tPlaneGeneratorFactory();
};

//*****************************************************************************
//
// Class tSphereGeneratorFactory 
//
//*****************************************************************************
class tSphereGeneratorFactory : public tBaseGeneratorFactory
{
public:
    virtual tBaseGenerator* CreateGenerator();

    friend class tGeneratorFactoryLoader;

protected:
    tSphereGeneratorFactory();
    virtual ~tSphereGeneratorFactory();
};

//*****************************************************************************
//
// Class tBaseGenerator 
//
//*****************************************************************************
class tBaseGenerator : public tEntity
{
public:
    tBaseGeneratorFactory* GetFactory() const { return factory; }
    
    virtual int   Generate(float frame, int numParticlesToGenerate, tBaseEmitter* emitter) = 0;

    virtual void  SetBias(unsigned bias, float b) = 0;
    virtual float GetBias(unsigned bias) const = 0;

    void SetBlendRatio(float ratio);
    float GetBlendRatio() const;

protected:
    tBaseGenerator();
    virtual ~tBaseGenerator();

    virtual void ReconnectAnimations() = 0;

    static rmt::Randomizer  randomGenerator;

    float frame;
    float blendRatio;
    int   blendState1;
    int   blendState2;

    tBaseGeneratorFactory* factory;    
};

//*****************************************************************************
//
// Class tPointGenerator 
//
//*****************************************************************************
class tPointGenerator : public tBaseGenerator
{
public:
    virtual int   Generate(float frame, int numParticlesToGenerate, tBaseEmitter* emitter);

    virtual void  SetBias(unsigned bias, float b);
    virtual float GetBias(unsigned bias) const;

    friend class tPointGeneratorFactory;

protected:
    tPointGenerator();
    virtual ~tPointGenerator();

    virtual void ReconnectAnimations();

    float horizBias;
    float vertBias;
    float radialBias;

    const tFloat1Channel*  horizSpreadChannel[2];
    const tFloat1Channel*  vertSpreadChannel[2];
    const tFloat1Channel*  radialVarChannel[2];
};

//*****************************************************************************
//
// Class tPlaneGenerator 
//
//*****************************************************************************
class tPlaneGenerator : public tBaseGenerator
{
public:
    virtual int   Generate(float frame, int numParticlesToGenerate, tBaseEmitter* emitter);

    virtual void  SetBias(unsigned bias, float b);
    virtual float GetBias(unsigned bias) const;

    friend class tPlaneGeneratorFactory;

protected:
    tPlaneGenerator();
    virtual ~tPlaneGenerator();

    virtual void ReconnectAnimations();

    float horizBias;
    float vertBias;
    float radialBias;
    float widthBias;
    float heightBias;

    rmt::Vector planeU;
    rmt::Vector planeV;
    rmt::Vector offset;

    const tFloat1Channel*  horizSpreadChannel[2];
    const tFloat1Channel*  vertSpreadChannel[2];
    const tFloat1Channel*  widthChannel[2];
    const tFloat1Channel*  heightChannel[2];
    const tFloat1Channel*  radialVarChannel[2];
};

//*****************************************************************************
//
// Class tSphereGenerator 
//
//*****************************************************************************
class tSphereGenerator : public tBaseGenerator
{
public:
    virtual int   Generate(float frame, int numParticlesToGenerate, tBaseEmitter* emitter);

    virtual void  SetBias(unsigned bias, float b);
    virtual float GetBias(unsigned bias) const;

    friend class tSphereGeneratorFactory;

protected:
    tSphereGenerator();
    virtual ~tSphereGenerator();

    virtual void ReconnectAnimations();

    float radialBias;
    float radiusBias;

    float radialVar;
    float radius;

    const tFloat1Channel*  radialVarChannel[2];
    const tFloat1Channel*  radiusChannel[2];
};

#endif
