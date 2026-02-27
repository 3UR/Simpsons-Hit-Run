/*===========================================================================
    particletype.hpp
    28-Feb-2001 Created by Kevin Voon

    Copyright (c)2000 Radical Entertainment, Inc.
    All rights reserved.
===========================================================================*/
#ifndef _PARTICLE_TYPE_HPP
#define _PARTICLE_TYPE_HPP

#include <radmath/radmath.hpp>
#include <p3d/p3dtypes.hpp>
#include <p3d/effects/particleconstants.hpp>
#include <p3d/effects/particleemitter.hpp>
#include <p3d/effects/particleutility.hpp>

//*****************************************************************************
//
// Class tBaseParticle
//
// Different types of particles can be created by deriving a class
// from tParticle.  The type of a particle refers to the art that is
// rendered to represent the particle.
//
//*****************************************************************************
class tBaseParticle
{
public:
    tBaseParticle() :
        lifeIndex(0),
        life(0.0f),
        lifeInit(DEFAULT_LIFE),
        weight(DEFAULT_WEIGHT),
        weightInit(DEFAULT_WEIGHT),
        velocity(DEFAULT_VELOCITY),
        velocityInit(DEFAULT_VELOCITY),
        accGravity(0.0f),
        currPos(DEFAULT_POSITION),
        prevPos(DEFAULT_POSITION)
    {
    }

    virtual ~tBaseParticle()
    {
    }

    virtual void Initialize(tBaseEmitter* emitter)
    {
        lifeIndex = 0;
        life = 0.0f;
        if (!emitter->GetFactory()->GetInfiniteLife())
        {
            lifeInit = rmt::Max(ParticleSystemRandomFloat(emitter->currLife,emitter->currLifeVar),0.0f);
        }
        else
        {
            lifeInit = -1.0f;
        }
        weight = weightInit = rmt::Max(ParticleSystemRandomFloat(emitter->currWeight,emitter->currWeightVar),0.0f);
        velocityInit.Scale(ParticleSystemRandomFloat(emitter->currSpeed,emitter->currSpeedVar));

        velocity = velocityInit;
        accGravity = 0.0f;
        currPos.Set(0.0f,0.0f,0.0f);
        prevPos.Set(0.0f,0.0f,0.0f);
    }
    
    virtual void Update(float deltaTime, float deltaFrame, tBaseEmitter* emitter)
    {
        float speed = 1.0f;
        life+=deltaFrame;
        if (lifeInit!=-1.0f)
        {
            if (life>lifeInit)
            {
                return;
            }
            else
            {
                lifeIndex = rmt::FtoL((life/lifeInit)*(float)(DEFAULT_LOOK_UP_SIZE-1));
                speed = emitter->baseLookUp[lifeIndex].speed;
                weight = weightInit * emitter->baseLookUp[lifeIndex].weight;
            }          
        }

        // note : this isn't realistic, weight affects how fast a particle falls!!!
        accGravity += (weight * emitter->currGravity * deltaFrame);
        velocity.x = (velocityInit.x * speed) * emitter->currDrag;
        velocity.y = ((velocityInit.y * speed) + accGravity) * emitter->currDrag;
        velocity.z = (velocityInit.z * speed) * emitter->currDrag;

        prevPos = currPos;
        currPos.x += (velocity.x * deltaFrame);
        currPos.y += (velocity.y * deltaFrame);
        currPos.z += (velocity.z * deltaFrame);

    }

    float GetFrame()               { return life; }
    void  SetFrame(float f)        { life = f; }    

    int          lifeIndex;
    float        life;
    float        lifeInit;
    float        weight;
    float        weightInit;
    rmt::Vector  velocity;
    rmt::Vector  velocityInit;
    float        accGravity;
    rmt::Vector  currPos;
    rmt::Vector  prevPos;
};

//*****************************************************************************
//
// Class tSpriteParticle
//
//*****************************************************************************
class tSpriteParticle : public tBaseParticle
{
public:
    tSpriteParticle() :
        texFrame(0),
        texFrameInit(0),
        colourVar(DEFAULT_COLOUR_VARIATION),
        alphaVar(DEFAULT_ALPHA_VARIATION),
        colour(DEFAULT_COLOUR),
        size(DEFAULT_SIZE),
        sizeInit(DEFAULT_SIZE),
        spin(DEFAULT_SPIN),
        spinInit(DEFAULT_SPIN)
    {
    }

    virtual ~tSpriteParticle()
    {
    }
 
    tColour ColourClamp(tColour colour, char colourVar, char alphaVar)
    {
#if 0
        register unsigned int result;
        register unsigned int variance = ((unsigned)colourVar) | ((unsigned)colourVar<<8) | ((unsigned)colourVar<<16) | ((unsigned)alphaVar<<24);
        register unsigned int offset = 0x80808080;

        asm __volatile__ ("
            paddsb %0, %1, %2
            paddub %0, %0, %3
            "
            : "+d" (result)
            : "d" (colour), "d" (variance), "d" (offset) );

        return result;
#else
        int colourClampTableMin[4] = 
        {
            0xff, // 00
            0x00, // 01
            0xff, // 10
            0x00, // 11
        };

        // overflow neg
        int colourClampTableMax[4] = 
        {
            0x00, // 00
            0x00, // 01
            0xff, // 10
            0x00, // 11
        };

        int idx;
        int r = colour.Red() + colourVar;
        idx = ((r>>31)&1) | ((r>>7)&2);
        r &= colourClampTableMin[idx];
        r |= colourClampTableMax[idx];
        int g = colour.Green() + colourVar;
        idx = ((g>>31)&1) | ((g>>7)&2);
        g &= colourClampTableMin[idx];
        g |= colourClampTableMax[idx];
        int b = colour.Blue() + colourVar;
        idx = ((b>>31)&1) | ((b>>7)&2);
        b &= colourClampTableMin[idx];
        b |= colourClampTableMax[idx];
        int a = colour.Alpha() + alphaVar;
        idx = ((a>>31)&1) | ((a>>7)&2);
        a &= colourClampTableMin[idx];
        a |= colourClampTableMax[idx];

        return tColour(r,g,b,a);
#endif
    }

    virtual void Initialize(tBaseEmitter* emitter)
    {
        tBaseParticle::Initialize(emitter);

        tSpriteEmitter* spriteEmitter = (tSpriteEmitter*)emitter;
        tSpriteEmitterFactory* spriteEmitterFactory = (tSpriteEmitterFactory*)spriteEmitter->GetFactory();

        switch (spriteEmitterFactory->GetTextureAnimationMode())
        {
            case p3dParticleSystemConstants::TextureAnimMode::NONE:
            case p3dParticleSystemConstants::TextureAnimMode::RANDOM:
            {
                int numTexFrames = spriteEmitterFactory->GetNumTextureFrames();

                if (numTexFrames > 1)
                {
                    texFrameInit = texFrame = rmt::FtoL(((float)numTexFrames)*ParticleSystemRandomFloat(0.5f,0.5f));
                }
                else
                {
                    texFrame = texFrameInit = 0;
                }
                break;
            }
            case p3dParticleSystemConstants::TextureAnimMode::SPECIFIED:
            {
                texFrame = texFrameInit = spriteEmitter->spriteLookUp[0].texFrame;
                break;
            }
            default:
            {
                texFrame = texFrameInit = 0;
                break;
            }
        }

        switch (spriteEmitterFactory->GetColourAnimationMode())
        {
            case p3dParticleSystemConstants::ColourAnimMode::BOTH:
            {
                colour = spriteEmitter->spriteLookUp[0].colour;
                break;
            }
            case p3dParticleSystemConstants::ColourAnimMode::COLOUR:
            {
                colour = spriteEmitter->spriteLookUp[0].colour;
                colour.SetAlpha(spriteEmitter->currAlpha);
                break;
            }
            case p3dParticleSystemConstants::ColourAnimMode::ALPHA:
            {
                colour = spriteEmitter->currColour;
                colour.SetAlpha(spriteEmitter->spriteLookUp[0].colour.Alpha());
                break;
            }
            case p3dParticleSystemConstants::ColourAnimMode::NONE:
            default:
            {
                colour = spriteEmitter->currColour;
                colour.SetAlpha(spriteEmitter->currAlpha);
                break;
            }
        }

        colourVar = ParticleSystemRandomInt(0,spriteEmitter->currColourVar);
        alphaVar = ParticleSystemRandomInt(0,spriteEmitter->currAlphaVar);
        colour = ColourClamp(colour,colourVar,alphaVar);

        size = sizeInit = rmt::Max(ParticleSystemRandomFloat(spriteEmitter->currSize,spriteEmitter->currSizeVar),0.0f);
        switch (spriteEmitterFactory->GetAngleMode())
        {
            case p3dParticleSystemConstants::ParticleAngleMode::RANDOM:
            {
                spinInit = ParticleSystemRandomFloat(spriteEmitter->currSpin,spriteEmitter->currSpinVar);
                spin = ParticleSystemRandomFloat(0.0f,rmt::PI);
                break;
            }
            case p3dParticleSystemConstants::ParticleAngleMode::SPECIFIED:
            {
                spinInit = ParticleSystemRandomFloat(spriteEmitter->currSpin,spriteEmitter->currSpinVar);
                spin = ((tSpriteEmitterFactory*)spriteEmitter->GetFactory())->GetAngle();
                break;
            }
        }
    }

    virtual void Update(float deltaTime, float deltaFrame, tBaseEmitter* emitter)
    {
        tBaseParticle::Update(deltaTime, deltaFrame,emitter);

        tSpriteEmitter* spriteEmitter = (tSpriteEmitter*)emitter;
        tSpriteEmitterFactory* spriteEmitterFactory = (tSpriteEmitterFactory*)spriteEmitter->GetFactory();

        if (life<=lifeInit)
        {
            if (spriteEmitterFactory->GetTextureAnimationMode() == p3dParticleSystemConstants::TextureAnimMode::SPECIFIED)
            {
                texFrame = spriteEmitter->spriteLookUp[lifeIndex].texFrame;
            }
            else if (spriteEmitterFactory->GetNumTextureFrames()>1)
            {
                texFrame = rmt::FtoL(life/rmt::LtoF(spriteEmitterFactory->GetTextureFrameRate())) + texFrameInit;
                texFrame %= spriteEmitterFactory->GetNumTextureFrames();
            }

            switch (spriteEmitterFactory->GetColourAnimationMode())
            {
                case p3dParticleSystemConstants::ColourAnimMode::BOTH:
                {
                    colour = spriteEmitter->spriteLookUp[lifeIndex].colour;
                    colour = ColourClamp(colour,colourVar,alphaVar);
                    break;
                }
                case p3dParticleSystemConstants::ColourAnimMode::COLOUR:
                {
                    tColour newColour = spriteEmitter->spriteLookUp[lifeIndex].colour;
                    newColour.SetAlpha(colour.Alpha());
                    colour = ColourClamp(newColour,colourVar,alphaVar);
                    break;
                }
                case p3dParticleSystemConstants::ColourAnimMode::ALPHA:
                {
                    tColour newColour = colour; 
                    newColour.SetAlpha(spriteEmitter->spriteLookUp[lifeIndex].colour.Alpha());
                    colour = ColourClamp(newColour,colourVar,alphaVar);
                    break;
                }
                case p3dParticleSystemConstants::ColourAnimMode::NONE:
                default:
                {
                    break;
                }
            }

            size = sizeInit * spriteEmitter->spriteLookUp[lifeIndex].size;
            spin += spinInit * spriteEmitter->spriteLookUp[lifeIndex].spin * deltaFrame;
        }
        else
        {
            if (spriteEmitterFactory->GetNumTextureFrames()>1)
            {
                texFrame = rmt::FtoL(life/rmt::LtoF(spriteEmitterFactory->GetTextureFrameRate())) + texFrameInit;
                texFrame %= spriteEmitterFactory->GetNumTextureFrames();
            }
            spin += spinInit * deltaFrame;
        }
    }

    char            texFrame;
    char            texFrameInit;
    char            colourVar;
    char            alphaVar;
    tColour         colour;
    float           size;
    float           sizeInit;
    float           spin;
    float           spinInit;
};

#endif
