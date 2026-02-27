/*===========================================================================
    particleutility.hpp
    28-Feb-2001 Created by Kevin Voon

    Copyright (c)2000 Radical Entertainment, Inc.
    All rights reserved.
===========================================================================*/
#ifndef _PARTICLE_UTILITY_HPP
#define _PARTICLE_UTILITY_HPP

#include <radmath/radmath.hpp>
#include <p3d/p3dtypes.hpp>

class tFloat1Channel;
class tIntChannel;
class tColourChannel;
//////////////////////////////////////////////////////////////////////////
//Contemplated calling this HackClassForStaticData
// But, restrained myself. This is to speed up particle generation, which consumes 
// 9% of our cycletime.
//////////////////////////////////////////////////////////////////////////
class ParticleSystemRandomData
{
public:
    enum
    {
        msIntRandMod    = 1000,
        msFloatRandMod  = 1000
    };

    inline static void SetUp()
    {
        rmt::Randomizer randomGen(0);
        msCurInt    = 0;
        msCurFloat  = 0;
        int i;
        for(i=msIntRandMod-1; i>-1; i--)
        {
            msIntRands[i] = randomGen.Int();
        }
        for(i=msFloatRandMod-1; i>-1; i--)
        {
            msFloatRands[i] = randomGen.FloatSigned();
        }
    }

    inline static unsigned GetRandInt(unsigned int iSpread)
    {
        int retVal = msIntRands[msCurInt]%iSpread;
        msCurInt = (msCurInt+1)%msIntRandMod;
        return retVal;
    }

    inline static float GetRandFloat()
    {
        float retVal = msFloatRands[msCurFloat];
        msCurFloat = (msCurFloat+1)%msFloatRandMod;
        return retVal;
    }

    static int      msCurInt;
    static int      msCurFloat;
    static unsigned msIntRands[msIntRandMod];
    static float    msFloatRands[msFloatRandMod];
private:
    ParticleSystemRandomData(){}
};
//*****************************************************************************
//
// ParticleSystemRandomFloat
//
//*****************************************************************************
inline float ParticleSystemRandomFloat(float mean, float variance)
{
    return (ParticleSystemRandomData::GetRandFloat() * variance) + mean;
}

//*****************************************************************************
//
// ParticleSystemRandomFloat
//
//*****************************************************************************
inline int ParticleSystemRandomInt(int mean, int variance)
{
    if (variance>0)
    {
        unsigned int temp = ParticleSystemRandomData::GetRandInt((unsigned int)(variance*2));
        return (temp/2) + mean;
    }
    else
    {
        return 0;
    }
}

//*****************************************************************************
//
// ParticleCalcFloatLookUpTable
//
//*****************************************************************************
void ParticleCalcFloatLookUpTable(const tFloat1Channel* channel, float* lookUpTable, int lookUpSize, float numFrames, float defaultValue);

//*****************************************************************************
//
// ParticleCalcIntLookUpTable
//
//*****************************************************************************
void ParticleCalcIntLookUpTable(const tIntChannel* channel, int* lookUpTable, int lookUpSize, float numFrames, int defaultValue);

//*****************************************************************************
//
// ParticleCalcColourLookUpTable
//
//*****************************************************************************
void ParticleCalcColourLookUpTable(const tColourChannel* channel, tColour* lookUpTable, int lookUpSize, float numFrames, tColour defaultColour);

#endif

