/*===========================================================================
    particleutility.cpp
    28-Feb-2001 Created by Kevin Voon

    Copyright (c)2000 Radical Entertainment, Inc.
    All rights reserved.
===========================================================================*/
#include <p3d/anim/channel.hpp>
#include <p3d/effects/particleconstants.hpp>
#include <p3d/effects/particleutility.hpp>


int ParticleSystemRandomData::msCurInt=0;
int ParticleSystemRandomData::msCurFloat=0;
unsigned ParticleSystemRandomData::msIntRands[ParticleSystemRandomData::msIntRandMod];
float    ParticleSystemRandomData::msFloatRands[ParticleSystemRandomData::msFloatRandMod];

/*
//*****************************************************************************
//
// ParticleSystemRandomFloat
//
//*****************************************************************************
float ParticleSystemRandomFloat(float mean, float variance)
{
    static rmt::Randomizer randomGen(0);
    
    // get random number
    return (randomGen.FloatSigned() * variance) + mean;
}

//*****************************************************************************
//
// ParticleSystemRandomInt
//
//*****************************************************************************
int ParticleSystemRandomInt(int mean, int variance)
{
    static rmt::Randomizer randomGen(0);
    
    // get random number
    if (variance>0)
    {
        unsigned int temp = randomGen.IntRanged((unsigned int)(variance*2));
        return (temp/2) + mean;
    }
    else
    {
        return 0;
    }
}
*/
//*****************************************************************************
//
// ParticleCalcFloatLookUpTable
//
//*****************************************************************************
void ParticleCalcFloatLookUpTable(const tFloat1Channel* channel, float* lookUpTable, int lookUpSize, float numFrames, float defaultValue)
{
    if (channel)
    {
        float currentFrame = 0.0f;
        float frameInc = numFrames/rmt::LtoF(lookUpSize);

        for (int i = 0; i < lookUpSize; i++)
        {
            channel->GetValue(currentFrame,&lookUpTable[i]);
            currentFrame += frameInc;
        }      
    }
    else
    {
        for (int i = 0; i < lookUpSize; i++)
        {
            lookUpTable[i] = defaultValue;
        }
    }
}

//*****************************************************************************
//
// ParticleCalcIntLookUpTable
//
//*****************************************************************************
void ParticleCalcIntLookUpTable(const tIntChannel* channel, int* lookUpTable, int lookUpSize, float numFrames, int defaultValue)
{
    if (channel)
    {
        float currentFrame = 0.0f;
        float frameInc = numFrames/rmt::LtoF(lookUpSize);

        for (int i = 0; i < lookUpSize; i++)
        {
            channel->GetValue(currentFrame,&lookUpTable[i]);
            currentFrame += frameInc;
        }      
    }
    else
    {
        for (int i = 0; i < lookUpSize; i++)
        {
            lookUpTable[i] = defaultValue;
        }
    }
}

//*****************************************************************************
//
// ParticleCalcColourLookUpTable
//
//*****************************************************************************
void ParticleCalcColourLookUpTable(const tColourChannel* channel, tColour* lookUpTable, int lookUpSize, float numFrames, tColour defaultColour)
{
    if (channel)
    {
        float frameInc = numFrames/rmt::LtoF(lookUpSize);
        float currentFrame = 0.0f;

        for (int i = 0; i < lookUpSize; i++)
        {
            channel->GetValue(currentFrame, &lookUpTable[i]);
            currentFrame += frameInc;
        }      
    }
    else
    {
        for (int i = 0; i < lookUpSize; i++)
        {
            lookUpTable[i] = defaultColour;
        }
    }
}

//*****************************************************************************
//
// p3dCalcPlaneEqn
//
//*****************************************************************************
rmt::Vector4 p3dCalcPlaneEqn(rmt::Matrix transform, rmt::Vector v1, rmt::Vector v2, rmt::Vector v3)
{
    v1.Transform(transform);
    v2.Transform(transform);
    v3.Transform(transform);

    rmt::Vector normal;
    v2.Sub(v1);
    v3.Sub(v1);
    normal.CrossProduct(v2,v3);
    normal.Normalize();

    return rmt::Vector4(normal.x,normal.y,normal.z,(normal.x*(-v1.x))+(normal.y*(-v1.y))+(normal.z*(-v1.z)));
}
