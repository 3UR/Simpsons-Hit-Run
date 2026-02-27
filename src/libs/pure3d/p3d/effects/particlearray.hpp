/*===========================================================================
    particlearray.hpp
    28-Feb-2001 Created by Kevin Voon

    Copyright (c)2000 Radical Entertainment, Inc.
    All rights reserved.
===========================================================================*/
#ifndef _PARTICLE_ARRAY_HPP
#define _PARTICLE_ARRAY_HPP

#include <p3d/refcounted.hpp>
#include <constants/psenum.hpp>

class tParticlePool;
class tBaseEmitter;
class tBaseParticle;

//*****************************************************************************
//
// Class tParticleArray
//
//*****************************************************************************
class tParticleArray : public tRefCounted
{
public:
    tParticleArray(tParticlePool* pool);

    void            AllocateParticles();
    void            ReleaseParticles();
    void            UpdateParticles(float deltaTime, float deltaFrame, tBaseEmitter* emitter);
    void            KillAllParticles();
    void            RefreshParticleArray();

    unsigned        GetParticleType()                           { return particleType; }
    int             GetNumParticles()                           { return numParticles; }
    int             GetNumLiveParticles()                       { return numLiveParticles; }
    
    // MKR - unused and unnecessary
    //int             GetNumDeadParticles()                       { return numDeadParticles; }
    tBaseParticle** GetLiveParticles()                          { return liveParticles; }
    tBaseParticle*  RejuenateDeadParticle();

    // does not create new live particles only kills particles so that there are a maximum number of live particles
    void            SetNumLiveParticles(int num);

protected:   
    tParticleArray();
    virtual ~tParticleArray();
   
    unsigned        particleType;
    tParticlePool*  particlePool;
    int             numParticles;
    int             numLiveParticles;
    //int             numDeadParticles;
    // MKR - particlepools now deal with indices to a master list of particles
    // rather than a actual list of contiguous particles
    // switch the void* to a short*

    //void*           particles;
    tBaseParticle** liveParticles;
    short*          particleIndices;
    //tBaseParticle** deadParticles;
};

#endif
