/*===========================================================================
    particlepool.hpp
    15-April-2002 Created by Kevin Voon

    Copyright (c)2000 Radical Entertainment, Inc.
    All rights reserved.
===========================================================================*/
#ifndef _PARTICLE_POOL_HPP
#define _PARTICLE_POOL_HPP

#include <p3d/refcounted.hpp>
#include <constants/psenum.hpp>

class tBaseParticle;

//*****************************************************************************
//
// Class tParticlePool
//
//      Each particle pool consists of x number of evenly divided sub pools
//*****************************************************************************
class tParticlePool : public tRefCounted
{
public:
    tParticlePool(unsigned particleType, unsigned int numParticles, unsigned int numSubPools);
 
    unsigned GetParticleType()          { return particleType; }
    int GetNumSubPools()                { return numSubPools; }
    int GetNumAvailableSubPools()       { return numAvailableSubPools; }
    int GetNumParticlesPerSubPool()     { return numParticlesPerSubPool; }

    bool RequestParticle( tBaseParticle** newParticle, short* newParticleIndex );
    void ReleaseParticle( short particleIndex );

protected:
    tParticlePool();
    virtual ~tParticlePool();

    unsigned        particleType;
    int             numSubPools;
    int             numAvailableSubPools;
    int             numParticlesPerSubPool;

};
  
#endif
