/*===========================================================================
    particleemitter.hpp
    28-Feb-2001 Created by Kevin Voon

    Copyright (c)2000 Radical Entertainment, Inc.
    All rights reserved.
===========================================================================*/
#ifndef _PARTICLE_EMITTER_HPP
#define _PARTICLE_EMITTER_HPP

#include <radmath/radmath.hpp>
#include <p3d/p3dtypes.hpp>
#include <p3d/entity.hpp>
#include <p3d/shader.hpp>
#include <p3d/refcounted.hpp>
#include <p3d/effects/particleconstants.hpp>
#include <p3d/effects/particlearray.hpp>
#include <p3d/effects/particlepool.hpp>

class tBaseParticle;
class tSpriteParticle;
class tBaseEmitter;
class tSpriteEmitter;
class tLocalSpriteEmitter;
class tBaseGenerator;
class tBaseGeneratorFactory;
class tEmitterFactoryLoader;
class tAnimation;
class tBoolChannel;
class tIntChannel;
class tFloat1Channel;
class tColourChannel;
class tVectorChannel;
class tRotationChannel;

struct BaseLookUp
{
    BaseLookUp() : speed(1.0f), weight(1.0f) {}
    float speed;
    float weight;
};

struct SpriteLookUp
{
    SpriteLookUp() : colour(255,255,255,255), size(1.0f), spin(1.0f), texFrame(0) {}
    tColour colour;
    float   size;
    float   spin;
    int     texFrame;
};

//*****************************************************************************
//
// Class tParticleRenderModule
//
//*****************************************************************************
class tParticleRenderModule : public tRefCounted
{
public:
    tParticleRenderModule() {}
    virtual void RenderInit() = 0;
    virtual void RenderExit() = 0;
protected:
    virtual ~tParticleRenderModule() {}
};

//*****************************************************************************
//
// Class tBaseEmitterFactory
//
//*****************************************************************************
class tBaseEmitterFactory : public tEntity
{
public:
    virtual tBaseEmitter* CreateEmitter() = 0;
    virtual tBaseEmitter* CreateLocalEmitter() = 0;

    void SetRenderModule(tParticleRenderModule* module) { tRefCounted::Assign(renderModule,module); }
    tParticleRenderModule* GetRenderModule() const      { return renderModule; }

    unsigned    GetParticleType() const                 { return particleType; }
    unsigned    GetGeneratorType() const                { return generatorType; }
    int         GetMaxNumParticles() const              { return maxNumParticles; }
    float       GetTranslationalCohesion() const        { return translationalCohesion; }
    float       GetRotationalCohesion() const           { return rotationalCohesion; }
    bool        GetZTest() const                        { return zTest; }
    bool        GetZWrite() const                       { return zWrite; }
    bool        GetFog() const                          { return fog; }    
    bool        GetInfiniteLife() const                 { return infiniteLife; }

    tAnimation* GetEmitterAnimation() const             { return emitterAnim; }

    void SetAutomaticParticleAllocation(bool enable)    { autoParticleAlloc = enable; }
    bool GetAutomaticParticleAllocation()               { return autoParticleAlloc; }

    friend class tEmitterFactoryLoader;
    friend class tParticleSystemFactoryLoader;

protected:
    tBaseEmitterFactory();
    virtual ~tBaseEmitterFactory();

    virtual void SetUpEmitter(tBaseEmitter* emitter);
    
    unsigned    particleType;
    unsigned    generatorType;
    int         maxNumParticles;
    float       translationalCohesion;
    float       rotationalCohesion;
    bool        zTest;
    bool        zWrite;
    bool        fog;
    bool        infiniteLife;
    bool        autoParticleAlloc;

    BaseLookUp  baseLookUp[DEFAULT_LOOK_UP_SIZE];
    
    tAnimation*             emitterAnim;
    tBaseGeneratorFactory*  generatorFactory;
    tParticlePool*          particlePool;

    tParticleRenderModule*  renderModule;
};

//*****************************************************************************
//
// Class tSpriteEmitterFactory
//
//*****************************************************************************
class tSpriteEmitterFactory : public tBaseEmitterFactory
{
public:
    virtual tBaseEmitter* CreateEmitter();
    virtual tBaseEmitter* CreateLocalEmitter();

    tShader*    GetShader() const                       { return shader; }
    unsigned    GetAngleMode() const                    { return angleMode; }
    float       GetAngle() const                        { return angle; }
    unsigned    GetColourAnimationMode() const          { return colourAnimMode; }
    unsigned    GetTextureAnimationMode() const         { return texAnimMode; }
    int         GetNumTextureFrames() const             { return numTexFrames; }
    int         GetTextureFrameRate() const             { return texFrameRate; }
    
    friend class tEmitterFactoryLoader;

protected:
    tSpriteEmitterFactory();
    virtual ~tSpriteEmitterFactory();

    virtual void SetUpEmitter(tBaseEmitter* emitter);
    
    tShader*        shader;
    unsigned        angleMode;
    float           angle;
    unsigned        colourAnimMode;
    unsigned        texAnimMode;
    int             numTexFrames;
    int             texFrameRate;

    SpriteLookUp    spriteLookUp[DEFAULT_LOOK_UP_SIZE];
    float*          texFrameCoords;
};


//*****************************************************************************
//
// Class tBaseEmitter
//
//*****************************************************************************
class tBaseEmitter : public tEntity
{
public:
    void SetRenderModule(tParticleRenderModule* module) { tRefCounted::Assign(renderModule,module); }
    tParticleRenderModule* GetRenderModule() const      { return renderModule; }
    
    tBaseEmitterFactory* GetFactory() const             { return factory; }
    tBaseGenerator*      GetGenerator() const           { return generator; }
    tParticleArray*      GetParticles() const           { return particles; }

    void AllocateParticles()                            { particles->AllocateParticles(); if (particles->GetNumParticles()>0) forcedParticleAlloc = true; }
    void ReleaseParticles()                             { forcedParticleAlloc = false; particles->ReleaseParticles(); }
   
 //   void Rotate(float x, float y, float z)              { rmt::Matrix rotate; rotate.FillRotateXYZ(x,y,z); localMatrix.Mult(rotate); }
 //   void Translate(float x, float y, float z)           { localMatrix.m[3][0] += x; localMatrix.m[3][1] += y; localMatrix.m[3][2] += z; }
 //   void ResetMatrix()                                  { localMatrix.Identity(); }
    rmt::Matrix GetTransformMatrix()                    { return transformMatrix; }

    virtual void ResetParticles();                                               
    virtual void ResetAnimation();
    virtual void ResetBiases();

    void SetBlendRatio(float ratio);
    float GetBlendRatio() const;

    void Update(float deltaTime, float deltaFrame, const rmt::Matrix* worldMatrix = NULL);
    virtual void Display() = 0;
    
    virtual void  SetBias(unsigned bias, float b);
    virtual float GetBias(unsigned bias) const;

    virtual void SetFrame(float f);
    virtual float GetFrame() const                      { return frame; }
    void GetPosition( rmt::Vector* )const;

    // The following two functions do expensive calculations to attempt to determine
    // the largest box/sphere than encapsulates the particle system. 
    // The box will *not* be optimal, and will probably be too large.
    // Definitely do not to use these functions every frame. 
    void ComputePreciseBoundingBox( rmt::Box3D* box );
    void ComputePreciseBoundingSphere( rmt::Sphere* sphere );


    friend class tBaseParticle;
    friend class tBaseEmitterFactory;

protected:
    tBaseEmitter();
    virtual ~tBaseEmitter();
    
    virtual void ReconnectAnimations();

    virtual void UpdateEmitterAttributes();

    tBaseEmitterFactory* factory;
    tBaseGenerator*      generator;
    tParticleArray*      particles;
    bool                 forcedParticleAlloc;

    bool  visible;
    float frame;

    rmt::Matrix transformMatrix;

    float numPartBias;
    float emissionBias;
    float lifeBias;
    float speedBias;
    float weightBias;
    float gravityBias;
    float dragBias;    
    
    float blendRatio;
    int   blendState1;
    int   blendState2;
       
    float numParticlesQueued;
    int   currNumParticles;
    float currEmissionRate;
    float currLife;
    float currLifeVar;
    float currSpeed;
    float currSpeedVar;
    float currWeight;
    float currWeightVar;
    float currGravity;  
    float currDrag;

    BaseLookUp* baseLookUp;
  
    const tBoolChannel*     visibilityChannel;
    const tVectorChannel*   translationChannel;
    const tRotationChannel* rotationChannel;
    const tIntChannel*      numParticlesChannel[2];
    const tFloat1Channel*   emissionRateChannel[2];
    const tFloat1Channel*   lifeChannel[2];
    const tFloat1Channel*   lifeVarChannel[2];
    const tFloat1Channel*   speedChannel[2];
    const tFloat1Channel*   speedVarChannel[2];
    const tFloat1Channel*   weightChannel[2];
    const tFloat1Channel*   weightVarChannel[2];
    const tFloat1Channel*   gravityChannel[2];
    const tFloat1Channel*   dragChannel[2];
    
    tParticleRenderModule*  renderModule;
};

//*****************************************************************************
//
// Class tSpriteEmitter
//
//*****************************************************************************
class tSpriteEmitter : public tBaseEmitter
{
public:
    virtual void Display();

    virtual void ResetBiases();

    virtual void  SetBias(unsigned bias, float b);
    virtual float GetBias(unsigned bias) const;

    void SetShader(tShader* s)  { tRefCounted::Assign(shader,s); }
    tShader* GetShader() const  { return shader; }

    friend class tSpriteParticle;
    friend class tSpriteEmitterFactory;
    inline void SetLocal( const bool value )
    {
        local = value;
    }
     
protected:
    tSpriteEmitter();
    virtual ~tSpriteEmitter();

    virtual void ReconnectAnimations();
    
    virtual void UpdateEmitterAttributes();

    tShader* shader;

    float sizeBias;
    float spinBias;
          
    tColour currColour;
    int     currColourVar;
    int     currAlpha;
    int     currAlphaVar;
    float   currSize;
    float   currSizeVar;
    float   currSpin;
    float   currSpinVar;
    bool    local;  //nv

    SpriteLookUp*   spriteLookUp; 
    float*          texFrameCoords;

    const tColourChannel*   colourChannel[2];
    const tIntChannel*      colourVarChannel[2];
    const tIntChannel*      alphaChannel[2];
    const tIntChannel*      alphaVarChannel[2];
    const tFloat1Channel*   sizeChannel[2];
    const tFloat1Channel*   sizeVarChannel[2];
    const tFloat1Channel*   spinChannel[2];
    const tFloat1Channel*   spinVarChannel[2];
};

#endif

