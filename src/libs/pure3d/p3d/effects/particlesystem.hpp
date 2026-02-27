/*===========================================================================
    particlesystem.hpp
    28-Feb-2001 Created by Kevin Voon

    Copyright (c)2000 Radical Entertainment, Inc.
    All rights reserved.
===========================================================================*/
#ifndef _PARTICLE_SYSTEM_HPP
#define _PARTICLE_SYSTEM_HPP

#include <radmath/radmath.hpp>
#include <p3d/entity.hpp>
#include <p3d/refcounted.hpp>
#include <p3d/effects/effect.hpp>
#include <p3d/effects/particleemitter.hpp>

class tParticleSystem;

//*****************************************************************************
//
// Class tParticleSystemFactory
//
//*****************************************************************************
class tParticleSystemFactory : public tEffectFactory
{
public:     
    virtual tEffect* CreateEffect(tEffectController* controller = NULL);

    int GetNumEmitters() const                              { return numEmitters; }
    int FindEmitterFactory(const char* name) const;
    tBaseEmitterFactory* GetEmitterFactory(int index) const;

    void SetRenderModule(tParticleRenderModule* module)     { tRefCounted::Assign(renderModule,module); }
    tParticleRenderModule* GetRenderModule() const          { return renderModule; }
       
    float GetFrameRate()                                    { return 1000.0f/frameTime; }
    float GetFrameTime()                                    { return frameTime; }
    float GetNumFrames()                                    { return numFrames; }
    bool  GetCycleAnim()                                    { return cycleAnim; }
    bool  GetEnableSorting()                                { return enableSorting; }
    int   GetMaxInstances()                                 { return maxInstances; }
       
    void SetAutomaticParticleAllocation(bool enable);
    bool ShouldUpdateAfterDisplay()const                    { return alwaysUpdateAfterDisplay; }
    void SetAlwaysUpdateAfterDisplay( bool enable )         { alwaysUpdateAfterDisplay = enable; }



    friend class tParticleSystemFactoryLoader;

protected:
    tParticleSystemFactory();
    virtual ~tParticleSystemFactory();

    int                     numEmitters;              
    tBaseEmitterFactory**   emitters;
    float                   frameTime;                 // time of frame in milliseconds = 1000 * (1 / frame rate)
    float                   numFrames;                 // frame on which to reset system
    bool                    cycleAnim;                 // flag to indicate whether or not the cycle the particle system animation
    bool                    enableSorting;             // flag to enable emitter sorting, to draw emitters furthest to closest
    bool                    autoParticleAlloc;
    int                     maxInstances;

    bool                    alwaysUpdateAfterDisplay;

    tParticleRenderModule*  renderModule;
};

//*****************************************************************************
//
// Class tParticleSystem
//
//*****************************************************************************
class tParticleSystem : public tEffect
{
public:     
    virtual tEffectFactory* GetFactory() const                  { return factory; }    

    void SetRenderModule(tParticleRenderModule* module);

    int GetNumEmitters() const                                  { return numEmitters; }
    int FindEmitter(const char* name) const;
    tBaseEmitter* GetEmitter(int index) const;

    int GetNumLiveParticles();
    int GetNumAllocatedParticles();

    void AllocateParticles();
    void ReleaseParticles();

    void ResetParticles();
    void ResetAnimation();
    void ResetBiases();

    void SetBias(unsigned bias, float b);
    void SetBlendRatio(float ratio);

    void Rotate(float x, float y, float z)                      { rmt::Matrix rotate; rotate.FillRotateXYZ(x,y,z); localMatrix.Mult(rotate); }
    void Translate(float x, float y, float z)                   { localMatrix.m[3][0] += x; localMatrix.m[3][1] += y; localMatrix.m[3][2] += z; }
    void ResetMatrix()                                          { localMatrix.Identity(); }
    void SetMatrix(rmt::Matrix m)                               { localMatrix = m; }
    rmt::Matrix GetMatrix()                                     { return localMatrix; }
    
    virtual float           GetNumFrames()                                { return factory->GetNumFrames(); }
    virtual void            SetFrame(float f);                        
    virtual float           GetFrame()                                    { return frame; }
    virtual void            SetFrameRange(float minframe, float maxframe) { minFrame = minframe; maxFrame = maxframe; }    
    virtual void            SetCycleMode(p3dCycleMode cyclemode)          { cycleMode = cyclemode; }
    virtual p3dCycleMode    GetCycleMode()                                { return cycleMode; }
    virtual void            Advance(float deltaTime);
    virtual void            Reset()                                       { SetCycleMode(DEFAULT_CYCLE_MODE); SetBlendRatio(0.0f); ResetMatrix(); ResetParticles(); ResetAnimation(); ResetBiases(); SetFrameRange(-1.0f,-1.0f); }
            void            Update(const rmt::Matrix* worldMatrix = NULL);
    virtual void            Display();
    virtual void            SetVisibility( bool v )                       { visible = v; }
    virtual bool            GetVisibility()                               { return visible; }
    virtual int             LastFrameReached();

    virtual void            SetVelocity( const rmt::Vector& velocity );

    virtual void ProcessShaders(ShaderCallback&);

    void GetPosition( rmt::Vector* position ) const;

    void ConvertEmittersToLocal();

    // The following two functions do expensive calculations to attempt to determine
    // the largest box/sphere than encapsulates the particle system. 
    // The box will *not* be optimal, and will probably be too large.
    // Definitely do not to use these functions every frame. 
    void ComputePreciseBoundingBox( rmt::Box3D* box );
    void ComputePreciseBoundingSphere( rmt::Sphere* sphere );


    friend class tParticleSystemLoader;
    friend class tParticleSystemFactory;

protected:
    tParticleSystem();
    virtual ~tParticleSystem();

    virtual void SortEmitters();
    virtual float MakeValidFrame(float frame);

    tParticleSystemFactory* factory;
    bool                    visible;
    bool                    update;
    p3dCycleMode            cycleMode;
    float                   minFrame;
    float                   maxFrame;
    float                   frame;
    float                   deltaTime;
    float                   deltaFrame;
    int                     numCycles;
    int                     numEmitters;
    int*                    emitterDrawOrder;
    tBaseEmitter**          emitters;
    rmt::Matrix             localMatrix;
};


#endif
