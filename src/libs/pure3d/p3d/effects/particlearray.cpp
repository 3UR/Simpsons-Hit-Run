/*===========================================================================
    particlearray.cpp
    28-Feb-2001 Created by Kevin Voon

    Copyright (c)2000 Radical Entertainment, Inc.
    All rights reserved.
===========================================================================*/
#include <radmath/radmath.hpp>
#include <p3d/error.hpp>
#include <p3d/effects/particleconstants.hpp>
#include <p3d/effects/particletype.hpp>
#include <p3d/effects/particlepool.hpp>
#include <p3d/effects/particlearray.hpp>
#include <string.h>

//*****************************************************************************
//
// Class tParticleArray
//
//*****************************************************************************
tParticleArray::tParticleArray() :
    particleType(p3dParticleSystemConstants::UNDEFINED),
    particlePool(NULL),
    numParticles(0),
    numLiveParticles(0),
//    numDeadParticles(0),
//    particles(NULL),
    liveParticles(NULL),
//    deadParticles(NULL)
    particleIndices(NULL)
{
    P3DASSERT(0);
}

tParticleArray::tParticleArray(tParticlePool* pool) :
    particleType(p3dParticleSystemConstants::UNDEFINED),
    particlePool(NULL),
    numParticles(0),
    numLiveParticles(0),
//   numDeadParticles(0),
//    particles(NULL),
    liveParticles(NULL),
//    deadParticles(NULL)
    particleIndices(NULL)
{
    P3DASSERT((pool)&&(pool->GetNumParticlesPerSubPool()>0));
    tRefCounted::Assign(particlePool,pool);
    particleType = pool->GetParticleType();
    liveParticles = new tBaseParticle*[pool->GetNumParticlesPerSubPool()];
    particleIndices = new short[ pool->GetNumParticlesPerSubPool() ];
    // MKR - deadParticles = new tBaseParticle*[pool->GetNumParticlesPerSubPool()];
}

tParticleArray::~tParticleArray()
{
    ReleaseParticles();
    tRefCounted::Release(particlePool);
    delete [] liveParticles;
    // MKR - delete [] deadParticles;
    delete [] particleIndices;
}

void tParticleArray::AllocateParticles()
{
    numParticles = particlePool->GetNumParticlesPerSubPool();
    P3DASSERT(particlePool);
    return;
//    if (!particles)
    {
        //particles = particlePool->RetrieveNextAvailableSubPool();
//        if (particles)
        {
            numParticles = particlePool->GetNumParticlesPerSubPool();
            for ( int  i = 0 ; i < numLiveParticles ; i++)
            {
                particlePool->ReleaseParticle( particleIndices[ i ] );
            }
            numLiveParticles = 0;
            //numDeadParticles = numParticles;
            switch(particleType)
            {
                case p3dParticleSystemConstants::ParticleType::SPRITE:
                {
                    for (int i = 0; i < numParticles; i++)
                    {
                        // MKR - deadParticles is an array of pointers to dead particles
                        // I want to keep this unchanged. So the interfaces above particlearray
                        // and particlepool don't require changing.
                        // So we need to get a list of 
                        //deadParticles[i] = (tBaseParticle*)(&((tSpriteParticle*)particles)[numParticles-i-1]);
                    }
                    break;
                }
                default:
                {
                    P3DASSERT(0);
                    break;
                }
            }
        }
    }
}

void tParticleArray::ReleaseParticles()
{
    for ( int  i = 0 ; i < numLiveParticles ; i++)
    {
        particlePool->ReleaseParticle( particleIndices[ i ] );
    }

    numLiveParticles = 0;
    //numDeadParticles = 0;
    memset(liveParticles,0,sizeof(int)*numParticles);
    //memset(deadParticles,0,sizeof(int)*numParticles);
    numParticles = 0;
    // MKR - the subpool is now a short**, not a tBaseParticle**
    //particlePool->ReleaseSubPool(&particles);
    //particlePool->ReleaseSubPool((tBaseParticle**)&particles);
}

void tParticleArray::UpdateParticles(float deltaTime, float deltaFrame, tBaseEmitter* emitter)
{
    if (deltaTime==0.0f)
    {
        return;
    }
    for (int i = 0; i < numLiveParticles; i++)
    {
        liveParticles[i]->Update(deltaTime,deltaFrame,emitter);
    }
}

void tParticleArray::KillAllParticles()
{
    for ( int  i = 0 ; i < numLiveParticles ; i++)
    {
        particlePool->ReleaseParticle( particleIndices[ i ] );
    }
    numLiveParticles = 0; 
    //numDeadParticles = numParticles;
    //for (int i = 0; i < numParticles; i++)
    {
    //    deadParticles[i] = (tBaseParticle*)(&((tSpriteParticle*)particles)[numParticles-i-1]);
    }
}

void tParticleArray::RefreshParticleArray()
{/*
    int curr = 0;
    int prev = 0;
    for (curr = 0; curr < numLiveParticles; curr++)
    {
        if ((liveParticles[curr]->lifeInit==-1.0f)||(liveParticles[curr]->life<liveParticles[curr]->lifeInit))
        {
            liveParticles[prev]=liveParticles[curr];
            prev++;
        }
        else
        {
            deadParticles[numDeadParticles]=liveParticles[curr];
            numDeadParticles++;
        }
    }   
    numLiveParticles=prev;*/

    int curr = 0;
    int prev = 0;
    for (curr = 0; curr < numLiveParticles; curr++)
    {
        if ((liveParticles[curr]->lifeInit==-1.0f)||(liveParticles[curr]->life<liveParticles[curr]->lifeInit))
        {
            liveParticles[prev]=liveParticles[curr];
            particleIndices[prev]=particleIndices[curr];
            prev++;
        }
        else
        {
            particlePool->ReleaseParticle( particleIndices[curr] );
        }
    }
    numLiveParticles=prev;
}

tBaseParticle* tParticleArray::RejuenateDeadParticle()
{

    /*
    if (numDeadParticles > 0)
    {
        numDeadParticles--;
        liveParticles[numLiveParticles] = deadParticles[numDeadParticles];
        numLiveParticles++;
        return liveParticles[numLiveParticles-1];
    }*/
    
    short index;
    tBaseParticle* newParticle;
    if ( particlePool->RequestParticle( &newParticle, &index ) )
    {
        liveParticles[numLiveParticles] = newParticle;
        particleIndices[numLiveParticles] = index;
        liveParticles[numLiveParticles]->life = liveParticles[numLiveParticles]->lifeInit+1.0f;

        numLiveParticles++;
    }
    else
    {
        newParticle = NULL;
    }

    return newParticle;
}

void tParticleArray::SetNumLiveParticles(int num)
{   
            
    
    if (num>=numLiveParticles)
        return;

    for (int curr = num; curr < numLiveParticles; curr++)
    {
        // MKR - is the next line necessary?
        //liveParticles[curr]->life = liveParticles[curr]->lifeInit+1.0f;
        particlePool->ReleaseParticle( particleIndices[curr] );

        //deadParticles[numDeadParticles]=liveParticles[curr];
        //numDeadParticles++;
    }   
    numLiveParticles=num;
}

