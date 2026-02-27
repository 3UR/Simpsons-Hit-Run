/*===========================================================================
    particlegenerator.hpp
    28-Feb-2001 Created by Kevin Voon

    Copyright (c)2000 Radical Entertainment, Inc.
    All rights reserved.
===========================================================================*/
#include <p3d/utility.hpp>
#include <p3d/error.hpp>
#include <p3d/anim/animate.hpp>
#include <p3d/anim/channel.hpp>
#include <p3d/effects/particlegenerator.hpp>
#include <p3d/effects/particletype.hpp>
#include <p3d/effects/particlearray.hpp>
#include <p3d/effects/particleemitter.hpp>
#include <p3d/effects/particleloader.hpp>

rmt::Randomizer tBaseGenerator::randomGenerator(0);

static const float defaultRadius = 1.0f;
static const float defaultTheta = 1.57080f;
static const float defaultPhi = 1.57080f;

//*****************************************************************************
//
// Class tBaseGeneratorFactory 
//
//*****************************************************************************
tBaseGeneratorFactory::tBaseGeneratorFactory() :
    generatorType(p3dParticleSystemConstants::UNDEFINED),
    generatorAnim(NULL)
{
}

tBaseGeneratorFactory::~tBaseGeneratorFactory() 
{
    tRefCounted::Release(generatorAnim);
}

//*****************************************************************************
//
// Class tPointGeneratorFactory 
//
//*****************************************************************************
tPointGeneratorFactory::tPointGeneratorFactory()
{
    generatorType = p3dParticleSystemConstants::GeneratorType::POINT;
}

tPointGeneratorFactory::~tPointGeneratorFactory() 
{
}

tBaseGenerator* tPointGeneratorFactory::CreateGenerator()
{
    tPointGenerator* generator = new tPointGenerator;
    generator->CopyName(this);
    generator->factory = this;
    generator->factory->AddRef();
    generator->ReconnectAnimations();
    return (tBaseGenerator*)generator;
}

//*****************************************************************************
//
// Class tPlaneGeneratorFactory 
//
//*****************************************************************************
tPlaneGeneratorFactory::tPlaneGeneratorFactory()
{
    generatorType = p3dParticleSystemConstants::GeneratorType::PLANE;
}

tPlaneGeneratorFactory::~tPlaneGeneratorFactory() 
{
}

tBaseGenerator* tPlaneGeneratorFactory::CreateGenerator()
{
    tPlaneGenerator* generator = new tPlaneGenerator;
    generator->CopyName(this);
    generator->factory = this;
    generator->factory->AddRef();
    generator->ReconnectAnimations();
    return (tBaseGenerator*)generator;
}

//*****************************************************************************
//
// Class tSphereGeneratorFactory 
//
//*****************************************************************************
tSphereGeneratorFactory::tSphereGeneratorFactory()
{
    generatorType = p3dParticleSystemConstants::GeneratorType::SPHERE;
}

tSphereGeneratorFactory::~tSphereGeneratorFactory() 
{
}

tBaseGenerator* tSphereGeneratorFactory::CreateGenerator()
{
    tSphereGenerator* generator = new tSphereGenerator;
    generator->CopyName(this);
    generator->factory = this;
    generator->factory->AddRef();
    generator->ReconnectAnimations();
    return (tBaseGenerator*)generator;
}

//*****************************************************************************
//
// Class tBaseGenerator 
//
//*****************************************************************************
tBaseGenerator::tBaseGenerator() :
    blendRatio(0.0f),
    blendState1(0),
    blendState2(0),
    factory(NULL)
{
}

tBaseGenerator::~tBaseGenerator()
{
    tRefCounted::Release(factory);
}

void tBaseGenerator::SetBlendRatio(float ratio)
{
    ratio = rmt::Clamp(ratio,0.0f,1.0f);
    if (ratio == blendRatio)
        return;

    int numBlendStates = factory->GetGeneratorAnimation()->GetNumGroups()-1;
    if (numBlendStates>0)
    {
        float temp = ratio * rmt::LtoF(numBlendStates);  //span the range of 1.0f to numBlendStates
        int newBlendState1 = rmt::FtoL(rmt::Floor(temp));
        int newBlendState2 = rmt::FtoL(rmt::Ceil(temp));
        blendRatio = temp - rmt::LtoF(newBlendState1);
        if ((blendState1!=newBlendState1)||(blendState2!=newBlendState2))
        {
            blendState1 = newBlendState1;
            blendState2 = newBlendState2;
            ReconnectAnimations();
        }
    }
}

float tBaseGenerator::GetBlendRatio() const
{
    int numBlendStates = factory->GetGeneratorAnimation()->GetNumGroups()-1;
    if (numBlendStates>0)
    {
        return ((blendRatio + rmt::LtoF(blendState1))/(rmt::LtoF(numBlendStates)));
    }
    return 0.0f;
}

//*****************************************************************************
//
// Class tPointGenerator 
//
//*****************************************************************************
tPointGenerator::tPointGenerator() :
    horizBias(1.0f),
    vertBias(1.0f),
    radialBias(1.0f)
{
    horizSpreadChannel[0] = NULL;
    horizSpreadChannel[1] = NULL;
    vertSpreadChannel[0] = NULL;
    vertSpreadChannel[1] = NULL;
    radialVarChannel[0] = NULL;
    radialVarChannel[1] = NULL;
}

tPointGenerator::~tPointGenerator()
{
}

int tPointGenerator::Generate(float frame, int numParticlesToGenerate, tBaseEmitter* emitter)
{
    if (numParticlesToGenerate<=0)
    {
        return 0;
    }

    float horizSpread = 0.0f;
    float vertSpread = 0.0f;
    float radialVar = 0.0f;

    if (horizSpreadChannel[0])
    {
        horizSpreadChannel[0]->GetValue(frame,&horizSpread);
        horizSpread *= horizBias;
    }
    if (vertSpreadChannel[0])
    {
        vertSpreadChannel[0]->GetValue(frame,&vertSpread);
        vertSpread *= vertBias;
    }
    if (radialVarChannel[0])
    {
        radialVarChannel[0]->GetValue(frame,&radialVar);
        radialVar *= radialBias;
    }
    
    rmt::Matrix transform = emitter->GetTransformMatrix();
    int numGenerated = 0;
    for (int i = 0; i < numParticlesToGenerate; i++)
    {
        float newTheta = ParticleSystemRandomFloat(defaultTheta, horizSpread);
        float newPhi = ParticleSystemRandomFloat(defaultPhi, vertSpread);
        float scale = rmt::Max(ParticleSystemRandomFloat( 0.0f, radialVar ),0.0f);
        float x,y,z;
        rmt::SphericalToCartesian( defaultRadius, newTheta, newPhi, &x, &y, &z);
        tBaseParticle* particle = emitter->GetParticles()->RejuenateDeadParticle();        
        if (particle)
        {
            particle->velocityInit.Set(x,y,z);
            particle->velocityInit.Rotate(transform);
            particle->velocityInit.Normalize();
            particle->Initialize(emitter);
            particle->currPos.x = (transform.m[3][0] + (particle->velocity.x * scale));
            particle->currPos.y = (transform.m[3][1] + (particle->velocity.y * scale));
            particle->currPos.z = (transform.m[3][2] + (particle->velocity.z * scale));
        }
        numGenerated++;
    }
    return numGenerated;
}

void tPointGenerator::SetBias(unsigned bias, float b)
{
    switch (bias)
    {
        case p3dParticleSystemConstants::GeneratorBias::HORIZ_SPREAD:
        {
            horizBias = b;
            break;
        }
        case p3dParticleSystemConstants::GeneratorBias::VERT_SPREAD:
        {
            vertBias = b;
            break;
        }
        case p3dParticleSystemConstants::GeneratorBias::RADIAL_VAR:
        {
            radialBias = b;
            break;
        }
    }
}

float tPointGenerator::GetBias(unsigned bias) const
{
    switch (bias)
    {
        case p3dParticleSystemConstants::GeneratorBias::HORIZ_SPREAD:
        {
            return horizBias;
            break;
        }
        case p3dParticleSystemConstants::GeneratorBias::VERT_SPREAD:
        {
            return vertBias;
            break;
        }
        case p3dParticleSystemConstants::GeneratorBias::RADIAL_VAR:
        {
            return radialBias;
            break;
        }
        default:
        {
            return 0.0f;
            break;
        }
    }
}

void tPointGenerator::ReconnectAnimations()
{
    const tAnimationGroup* state = factory->GetGeneratorAnimation()->GetGroupByIndex(blendState1);
    if (state)
    {
        horizSpreadChannel[0] = state->GetFloat1Channel(Pure3DAnimationChannels::ParticleSystem::Generator::HORIZ_SPREAD_HOSP);
        vertSpreadChannel[0] = state->GetFloat1Channel(Pure3DAnimationChannels::ParticleSystem::Generator::VERT_SPREAD_VESP);
        radialVarChannel[0] = state->GetFloat1Channel(Pure3DAnimationChannels::ParticleSystem::Generator::RADIAL_VAR_RDVA);
        if (blendState2!=blendState1)
        {
            state = factory->GetGeneratorAnimation()->GetGroupByIndex(blendState2);
            horizSpreadChannel[1] = state->GetFloat1Channel(Pure3DAnimationChannels::ParticleSystem::Generator::HORIZ_SPREAD_HOSP);
            vertSpreadChannel[1] = state->GetFloat1Channel(Pure3DAnimationChannels::ParticleSystem::Generator::VERT_SPREAD_VESP);
            radialVarChannel[1] = state->GetFloat1Channel(Pure3DAnimationChannels::ParticleSystem::Generator::RADIAL_VAR_RDVA);
        }
        else
        {
            horizSpreadChannel[1] = NULL;
            vertSpreadChannel[1] = NULL;
            radialVarChannel[1] = NULL;
        }
    }
}

//*****************************************************************************
//
// Class tPlaneGenerator 
//
//*****************************************************************************
tPlaneGenerator::tPlaneGenerator() :
    horizBias(1.0f),
    vertBias(1.0f),
    radialBias(1.0f),
    widthBias(1.0f),
    heightBias(1.0f),
    planeU(1.0f,0.0f,0.0f),
    planeV(0.0f,0.0f,1.0f),
    offset(-0.5f,0.0f,-0.5f)
{
    horizSpreadChannel[0] = NULL;
    horizSpreadChannel[1] = NULL;
    vertSpreadChannel[0] = NULL;
    vertSpreadChannel[1] = NULL;
    radialVarChannel[0] = NULL;
    radialVarChannel[1] = NULL;
    widthChannel[0] = NULL;
    widthChannel[1] = NULL;
    heightChannel[0] = NULL;
    heightChannel[1] = NULL;
}

tPlaneGenerator::~tPlaneGenerator()
{
}

int tPlaneGenerator::Generate(float frame, int numParticlesToGenerate, tBaseEmitter* emitter)
{
    if (numParticlesToGenerate<=0)
    {
        return 0;
    }

    float horizSpread = 0.0f;
    float vertSpread = 0.0f;
    float radialVar = 0.0f;
    float width = 1.0f;
    float height = 1.0f;

    if (horizSpreadChannel[0])
    {
        horizSpreadChannel[0]->GetValue(frame,&horizSpread);
        horizSpread *= horizBias;
    }
    if (vertSpreadChannel[0])
    {
        vertSpreadChannel[0]->GetValue(frame,&vertSpread);
        vertSpread *= vertBias;
    }
    if (radialVarChannel[0])
    {
        radialVarChannel[0]->GetValue(frame,&radialVar);
        radialVar *= radialBias;
    }
    if (widthChannel[0])
    {
        widthChannel[0]->GetValue(frame,&width);
        width *= widthBias;
    }
    if (heightChannel[0])
    {
        heightChannel[0]->GetValue(frame,&height);
        height *= heightBias;
    }

    planeU.x = width;
    planeV.z = height;
    offset.Add(planeU, planeV);
    offset.Scale(-0.5f);

    rmt::Matrix transform = emitter->GetTransformMatrix();
    int numGenerated = 0;
    for (int i = 0; i < numParticlesToGenerate; i++)
    {
        float newTheta = ParticleSystemRandomFloat(defaultTheta, horizSpread);
        float newPhi = ParticleSystemRandomFloat(defaultPhi, vertSpread);
        float scale = rmt::Max(ParticleSystemRandomFloat( 0.0f, radialVar ),0.0f);
        float x,y,z;
        rmt::SphericalToCartesian( defaultRadius, newTheta, newPhi, &x, &y, &z);

        tBaseParticle* particle = emitter->GetParticles()->RejuenateDeadParticle();        
        if (particle)
        {
        
            particle->velocityInit.Set(x,y,z);
            particle->velocityInit.Rotate(transform);
            particle->velocityInit.Normalize();
            particle->Initialize(emitter);

            // get U vector
            rmt::Vector vU;
            vU.Scale(randomGenerator.Float(), planeU);
        
            // get V vector
            rmt::Vector vV;
            vV.Scale(randomGenerator.Float(), planeV);
        
            rmt::Vector startPos(vU.x + vV.x + offset.x, vU.y + vV.y + offset.y, vU.z + vV.z + offset.z);    

            particle->currPos.x = (transform.m[0][0]*startPos.x + transform.m[1][0]*startPos.y + transform.m[2][0]*startPos.z + transform.m[3][0]) + (particle->velocity.x * scale);
            particle->currPos.y = (transform.m[0][1]*startPos.x + transform.m[1][1]*startPos.y + transform.m[2][1]*startPos.z + transform.m[3][1]) + (particle->velocity.y * scale);
            particle->currPos.z = (transform.m[0][2]*startPos.x + transform.m[1][2]*startPos.y + transform.m[2][2]*startPos.z + transform.m[3][2]) + (particle->velocity.z * scale);        
        }
        numGenerated++;
    }
    return numGenerated;
}

void tPlaneGenerator::SetBias(unsigned bias, float b)
{
    switch (bias)
    {
        case p3dParticleSystemConstants::GeneratorBias::HORIZ_SPREAD:
        {
            horizBias = b;
            break;
        }
        case p3dParticleSystemConstants::GeneratorBias::VERT_SPREAD:
        {
            vertBias = b;
            break;
        }
        case p3dParticleSystemConstants::GeneratorBias::RADIAL_VAR:
        {
            radialBias = b;
            break;
        }
        case p3dParticleSystemConstants::GeneratorBias::WIDTH:
        {
            widthBias = b;
            break;
        }
        case p3dParticleSystemConstants::GeneratorBias::HEIGHT:
        {
            heightBias = b;
            break;
        }
    }
}

float tPlaneGenerator::GetBias(unsigned bias) const
{
    switch (bias)
    {
        case p3dParticleSystemConstants::GeneratorBias::HORIZ_SPREAD:
        {
            return horizBias;
            break;
        }
        case p3dParticleSystemConstants::GeneratorBias::VERT_SPREAD:
        {
            return vertBias;
            break;
        }
        case p3dParticleSystemConstants::GeneratorBias::RADIAL_VAR:
        {
            return radialBias;
            break;
        }
        case p3dParticleSystemConstants::GeneratorBias::WIDTH:
        {
            return widthBias;
            break;
        }
        case p3dParticleSystemConstants::GeneratorBias::HEIGHT:
        {
            return heightBias;
            break;
        }
        default:
        {
            return 0.0f;
            break;
        }
    }
}

void tPlaneGenerator::ReconnectAnimations()
{
    const tAnimationGroup* state = factory->GetGeneratorAnimation()->GetGroupByIndex(blendState1);
    if (state)
    {
        horizSpreadChannel[0] = state->GetFloat1Channel(Pure3DAnimationChannels::ParticleSystem::Generator::HORIZ_SPREAD_HOSP);
        vertSpreadChannel[0] = state->GetFloat1Channel(Pure3DAnimationChannels::ParticleSystem::Generator::VERT_SPREAD_VESP);
        radialVarChannel[0] = state->GetFloat1Channel(Pure3DAnimationChannels::ParticleSystem::Generator::RADIAL_VAR_RDVA);
        widthChannel[0] = state->GetFloat1Channel(Pure3DAnimationChannels::ParticleSystem::Generator::WIDTH_WDT);
        heightChannel[0] = state->GetFloat1Channel(Pure3DAnimationChannels::ParticleSystem::Generator::HEIGHT_HGT);
        if (blendState2!=blendState1)
        {
            state = factory->GetGeneratorAnimation()->GetGroupByIndex(blendState2);
            horizSpreadChannel[1] = state->GetFloat1Channel(Pure3DAnimationChannels::ParticleSystem::Generator::HORIZ_SPREAD_HOSP);
            vertSpreadChannel[1] = state->GetFloat1Channel(Pure3DAnimationChannels::ParticleSystem::Generator::VERT_SPREAD_VESP);
            radialVarChannel[1] = state->GetFloat1Channel(Pure3DAnimationChannels::ParticleSystem::Generator::RADIAL_VAR_RDVA);
            widthChannel[1] = state->GetFloat1Channel(Pure3DAnimationChannels::ParticleSystem::Generator::WIDTH_WDT);
            heightChannel[1] = state->GetFloat1Channel(Pure3DAnimationChannels::ParticleSystem::Generator::HEIGHT_HGT);
        }
        else
        {
            horizSpreadChannel[1] = NULL;
            vertSpreadChannel[1] = NULL;
            radialVarChannel[1] = NULL;
            widthChannel[1] = NULL;
            heightChannel[1] = NULL;
        }
    }
}

//*****************************************************************************
//
// Class tSphereGenerator 
//
//*****************************************************************************
tSphereGenerator::tSphereGenerator() :
    radialBias(1.0f),
    radiusBias(1.0f)
{
    radialVarChannel[0] = NULL;
    radialVarChannel[1] = NULL;
    radiusChannel[0] = NULL;
    radiusChannel[1] = NULL;
}

tSphereGenerator::~tSphereGenerator()
{
}

int tSphereGenerator::Generate(float frame, int numParticlesToGenerate, tBaseEmitter* emitter)
{
    if (numParticlesToGenerate<=0)
    {
        return 0;
    }

    float radialVar = 0.0f;
    float radius = 1.0f;

    if (radialVarChannel[0])
    {
        radialVarChannel[0]->GetValue(frame,&radialVar);
        radialVar *= radialBias;
    }
    if (radiusChannel[0])
    {
        radiusChannel[0]->GetValue(frame,&radius);
        radius *= radialBias;
    }

    rmt::Matrix transform = emitter->GetTransformMatrix();  
    int numGenerated = 0;

    if (radius != 0.0f)
    {
        for (int i = 0; i < numParticlesToGenerate; i++)
        {
            float theta = ParticleSystemRandomFloat( 0.0f, rmt::PI);
            float phi   = ParticleSystemRandomFloat( 0.0f, rmt::PI);
            float scale = rmt::Max(ParticleSystemRandomFloat( 0.0f, radialVar ),0.0f);
            float x,y,z;
            rmt::SphericalToCartesian( radius, theta, phi, &x, &y, &z);                

            tBaseParticle* particle = emitter->GetParticles()->RejuenateDeadParticle();        
            if (particle)
            {
        
                particle->velocityInit.Set(x,y,z);
                particle->velocityInit.Rotate(transform);
                particle->velocityInit.Normalize();
                particle->Initialize(emitter);
                particle->currPos.x = (x + transform.m[3][0] + (particle->velocity.x * scale));
                particle->currPos.y = (y + transform.m[3][1] + (particle->velocity.y * scale));
                particle->currPos.z = (z + transform.m[3][2] + (particle->velocity.z * scale));
            }
            numGenerated++;
        }
    }
    else
    {
        for (int i = 0; i < numParticlesToGenerate; i++)
        {
            float theta = ParticleSystemRandomFloat( 0.0f, rmt::PI);
            float phi   = ParticleSystemRandomFloat( 0.0f, rmt::PI);
            float scale = rmt::Max(ParticleSystemRandomFloat( 0.0f, radialVar ),0.0f);
            float x,y,z;
            rmt::SphericalToCartesian( defaultRadius, theta, phi, &x, &y, &z);
            tBaseParticle* particle = emitter->GetParticles()->RejuenateDeadParticle();        
            if (particle)
            {
                particle->velocityInit.Set(x,y,z);
                particle->velocityInit.Rotate(transform);
                particle->velocityInit.Normalize();
                particle->Initialize(emitter);
                particle->currPos.x = transform.m[3][0] + (particle->velocity.x * scale);
                particle->currPos.y = transform.m[3][1] + (particle->velocity.y * scale);
                particle->currPos.z = transform.m[3][2] + (particle->velocity.z * scale);
            }
            numGenerated++;
        }
    }
    return numGenerated;
}

void tSphereGenerator::SetBias(unsigned bias, float b)
{
    switch (bias)
    {
        case p3dParticleSystemConstants::GeneratorBias::RADIAL_VAR:
        {
            radialBias = b;
            break;
        }
        case p3dParticleSystemConstants::GeneratorBias::RADIUS:
        {
            radiusBias = b;
            break;
        }
    }
}

float tSphereGenerator::GetBias(unsigned bias) const
{
    switch (bias)
    {
        case p3dParticleSystemConstants::GeneratorBias::RADIAL_VAR:
        {
            return radialBias;
            break;
        }
        case p3dParticleSystemConstants::GeneratorBias::RADIUS:
        {
            return radiusBias;
            break;
        }
        default:
        {
            return 0.0f;
            break;
        }
    }
}

void tSphereGenerator::ReconnectAnimations()
{
    const tAnimationGroup* state = factory->GetGeneratorAnimation()->GetGroupByIndex(blendState1);
    if (state)
    {
        radialVarChannel[0] = state->GetFloat1Channel(Pure3DAnimationChannels::ParticleSystem::Generator::RADIAL_VAR_RDVA);
        radiusChannel[0] = state->GetFloat1Channel(Pure3DAnimationChannels::ParticleSystem::Generator::RADIUS_RAD);
        if (blendState2!=blendState1)
        {
            state = factory->GetGeneratorAnimation()->GetGroupByIndex(blendState2);
            radialVarChannel[1] = state->GetFloat1Channel(Pure3DAnimationChannels::ParticleSystem::Generator::RADIAL_VAR_RDVA);
            radiusChannel[1] = state->GetFloat1Channel(Pure3DAnimationChannels::ParticleSystem::Generator::RADIUS_RAD);
        }
        else
        {
            radialVarChannel[1] = NULL;
            radiusChannel[1] = NULL;
        }
    }
}
