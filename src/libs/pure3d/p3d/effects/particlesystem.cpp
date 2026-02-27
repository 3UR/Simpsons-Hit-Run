/*===========================================================================
    particlesystem.cpp
    28-Feb-2001 Created by Kevin Voon

    Copyright (c)2000 Radical Entertainment, Inc.
    All rights reserved.
===========================================================================*/
#include <p3d/camera.hpp>
#include <p3d/view.hpp>
#include <p3d/matrixstack.hpp>
#include <p3d/utility.hpp>
#include <p3d/error.hpp>
#include <p3d/effects/particleloader.hpp>
#include <p3d/effects/particleemitter.hpp>
#include <p3d/effects/particlesystem.hpp>
#include <p3d/effects/particlearray.hpp>

#include <radtime.hpp>

//*****************************************************************************
//
// Class tParticleSystemFactory
//
//*****************************************************************************
tParticleSystemFactory::tParticleSystemFactory() :
    numEmitters(0),
    emitters(NULL),
    frameTime(33.3f),
    numFrames(0),
    cycleAnim(false),
    enableSorting(false),
    maxInstances(1),
    alwaysUpdateAfterDisplay( true ),
    renderModule(NULL)
{
}

tParticleSystemFactory::~tParticleSystemFactory()
{
    for (int i = 0; i < numEmitters; i++)
    {
        tRefCounted::Release(emitters[i]);
    }
    delete [] emitters;
    tRefCounted::Release(renderModule);
}

tEffect* tParticleSystemFactory::CreateEffect(tEffectController* controller)
{
    tParticleSystem* partSys = new tParticleSystem;
    partSys->CopyName(this);
    
    P3DASSERT(numEmitters>0);

    partSys->factory = this;
    partSys->factory->AddRef();
    partSys->numEmitters = numEmitters;
    partSys->emitterDrawOrder = new int[numEmitters];
    partSys->emitters = new tBaseEmitter*[numEmitters];

    for (int i = 0; i < numEmitters; i++)
    {
        partSys->emitterDrawOrder[i] = i;
        partSys->emitters[i] = emitters[i]->CreateEmitter();
        partSys->emitters[i]->AddRef();
    }

    partSys->SetRenderModule(renderModule);

    if (controller)
    {
        controller->SetEffect(partSys);
    }

    return (tEffect*)partSys;
}

int tParticleSystemFactory::FindEmitterFactory(const char* name) const
{
    tUID emitterUID = tEntity::MakeUID(name);
    for (int i = 0; i < numEmitters; i++)
    {
        if (emitterUID == emitters[i]->GetUID())
        {
            return i;
        }
    }
    return -1;
}

tBaseEmitterFactory* tParticleSystemFactory::GetEmitterFactory(int index) const
{
    if ((index<numEmitters)&&(index>=0))
    {
        return emitters[index];
    }
    return NULL;
}

void tParticleSystemFactory::SetAutomaticParticleAllocation(bool enable)
{
    for (int i = 0; i < numEmitters; i++)
    {
        emitters[i]->SetAutomaticParticleAllocation(enable);
    }
}





//*****************************************************************************
//
// Class tParticleSystem
//
//*****************************************************************************
tParticleSystem::tParticleSystem() :
    factory(NULL),
    visible(true),
    update(true),
    cycleMode(DEFAULT_CYCLE_MODE),
    minFrame(-1.0f),
    maxFrame(-1.0f),
    frame(0.0f),
    deltaTime(0.0f),
    deltaFrame(0.0f),
    numCycles(0),
    numEmitters(0),
    emitterDrawOrder(NULL),
    emitters(NULL)
{
    localMatrix.Identity();
}

tParticleSystem::~tParticleSystem()
{
    tRefCounted::Release(factory);
    delete [] emitterDrawOrder;
    for (int i = 0; i < numEmitters; i++)
    {
        tRefCounted::Release(emitters[i]);
    }
    delete [] emitters;
}

void tParticleSystem::SetRenderModule(tParticleRenderModule* module)
{
    for (int i = 0; i < numEmitters; i++)
    {
        emitters[i]->SetRenderModule(module);
    }
}

int tParticleSystem::FindEmitter(const char* name) const
{
    tUID emitterUID = tEntity::MakeUID(name);
    for (int i = 0; i < numEmitters; i++)
    {
        if (emitterUID == emitters[i]->GetUID())
        {
            return i;
        }
    }
    return -1;
}

tBaseEmitter* tParticleSystem::GetEmitter(int index) const
{
    if ((index<numEmitters)&&(index>=0))
    {
        return emitters[index];
    }
    return NULL;
}

int tParticleSystem::GetNumLiveParticles()
{
    int numLiveParticles = 0;
    for (int i = 0; i < numEmitters; i++)
    {
        numLiveParticles += emitters[i]->GetParticles()->GetNumLiveParticles();
    }
    return numLiveParticles;
}

int tParticleSystem::GetNumAllocatedParticles()
{
    int numAllocParticles = 0;
    for (int i = 0; i < numEmitters; i++)
    {
        numAllocParticles += emitters[i]->GetParticles()->GetNumParticles();
    }
    return numAllocParticles;
}

void tParticleSystem::AllocateParticles()
{
    for (int i = 0; i < numEmitters; i++)
    {
        emitters[i]->AllocateParticles();
    }
}

void tParticleSystem::ReleaseParticles()
{
    for (int i = 0; i < numEmitters; i++)
    {
        emitters[i]->ReleaseParticles();
    }
}

void tParticleSystem::ResetParticles()
{
    for (int i = 0; i < numEmitters; i++)
    {
        emitters[i]->ResetParticles();
    }
}

void tParticleSystem::ResetAnimation()
{
    for (int i = 0; i < numEmitters; i++)
    {
        emitters[i]->ResetAnimation();
    }
    SetFrame(0.0f);
}

void tParticleSystem::ResetBiases()
{
    for (int i = 0; i < numEmitters; i++)
    {
        emitters[i]->ResetBiases();
    }
}

void tParticleSystem::SetBias(unsigned bias, float b)
{
    for (int i = 0; i < numEmitters; i++)
    {
        emitters[i]->SetBias(bias,b);
    }
}

void tParticleSystem::SetBlendRatio(float ratio)
{
    for (int i = 0; i < numEmitters; i++)
    {
        emitters[i]->SetBlendRatio(ratio);
    }
}

void tParticleSystem::SetFrame(float f)
{
    frame = MakeValidFrame(f);
    for ( int i = 0; i < numEmitters; i++ )
    {
        emitters[i]->SetFrame(frame);
    }
    deltaTime = 0.0f;
    deltaFrame = 0.0f;    
    numCycles = 0;
}

float tParticleSystem::MakeValidFrame(float f)
{
    f = f>0.0f ? f : 0.0f;

    float tempMin = minFrame>=0.0f ? minFrame : 0.0f;
    float tempMax = maxFrame>=0.0f ? maxFrame : factory->GetNumFrames();

    if ((tempMin <= f)&&(f <= tempMax))
    {
        return f;
    }
    else if (f > tempMax)
    {
        switch (cycleMode)
        {
            case FORCE_CYCLIC:
            {
                return fmodf(f, tempMax-tempMin)+tempMin;
                break;
            }
            case FORCE_NON_CYCLIC:
            {
                return tempMax;
                break;
            }
            case DEFAULT_CYCLE_MODE:
            default:
            {
                return (factory->GetCycleAnim() ? fmodf(f, tempMax-tempMin)+tempMin : tempMax);
                break;
            }
        }        
    }
    else
    {
        return tempMin;
    }
}

void tParticleSystem::Advance(float dTime)
{
    if (visible)
    {
        P3DASSERT(factory->GetFrameTime() > 0.0f);
        float frameAdvance = dTime / factory->GetFrameTime();
        deltaTime += dTime;
        deltaFrame += frameAdvance;
    }
    else
    {
        deltaTime = 0.0f;
        deltaFrame = 0.0f;
    }
}

void tParticleSystem::Update(const rmt::Matrix* worldMatrix)
{       
    p3d::stack->Push();
    if (worldMatrix)
    {
        p3d::stack->LoadIdentity();
        rmt::Matrix temp = *worldMatrix;
        temp.Mult(localMatrix);
        for ( int i = 0; i < numEmitters; i++ )
        {
            emitters[emitterDrawOrder[i]]->Update(deltaTime,deltaFrame,&temp);
        }
    }
    else
    {
        p3d::stack->Multiply(localMatrix);
        for ( int i = 0; i < numEmitters; i++ )
        {
            emitters[emitterDrawOrder[i]]->Update(deltaTime,deltaFrame);
        }
    }    
    p3d::stack->Pop();

    if (factory->GetEnableSorting())
    {
        SortEmitters();
    }

    float newFrame = frame+deltaFrame;
    float endFrame = maxFrame<0.0f ? factory->GetNumFrames() : maxFrame;
    frame = MakeValidFrame(newFrame);
    for ( int i = 0; i < numEmitters; i++ )
    {
        emitters[i]->SetFrame(frame);
    }
    if (((cycleMode==FORCE_CYCLIC)||((cycleMode==DEFAULT_CYCLE_MODE)&&(factory->GetCycleAnim())))&&(newFrame>=endFrame))
    {
        numCycles++;
    }
    if ((maxFrame>0.0f)&&(frame>=maxFrame))
    {
        if ((minFrame!=maxFrame)&&(cycleMode==FORCE_NON_CYCLIC))
        {
            SetBias(p3dParticleSystemConstants::EmitterBias::EMISSION, 0.0f);
        }
    }
    deltaTime = 0.0f;
    deltaFrame = 0.0f;
    update = false;
}

void tParticleSystem::Display()
{
    if (!visible)
    {
        return;
    }

    if (update)
    {
        Update();
    }
    update = factory->ShouldUpdateAfterDisplay();

    for ( int i = 0; i < numEmitters; i++ )
    {
        emitters[emitterDrawOrder[i]]->Display();
    }
}
    
void tParticleSystem::ProcessShaders(ShaderCallback& callback)
{
    for(int i = 0; i < numEmitters; i++)
    {
        tSpriteEmitter* emm = dynamic_cast<tSpriteEmitter*>(emitters[i]);
        if(emm)
        {
            tShader* shader = emm->GetShader();
            tShader* newShader = callback.Process(shader);
            if(newShader != shader)
            {
                emm->SetShader(newShader);
            }
        }
    }
}

void tParticleSystem::GetPosition( rmt::Vector* position ) const
{
    // Get the center point of the emitters
    // To be quick, lets just grab the first emitter's position
    if ( GetNumEmitters() > 0 )
    {
        GetEmitter( 0 )->GetPosition( position );
    }
}


int tParticleSystem::LastFrameReached()
{ 
    float endFrame = maxFrame<0.0f ? factory->GetNumFrames() : maxFrame;
    if ((cycleMode==FORCE_NON_CYCLIC)||((cycleMode==DEFAULT_CYCLE_MODE)&&(!factory->GetCycleAnim())))
    {
        return (frame>=endFrame) ? 1 : 0;
    }
    else if ((minFrame>=0.0f)&&(minFrame==maxFrame))
    {
        return 1;
    }
    else
    {
        return numCycles; 
    }
}

void tParticleSystem::SetVelocity( const rmt::Vector& velocity )
{
}


void tParticleSystem::SortEmitters()
{
    if (numEmitters<=1)
    {
        return;
    }

    rmt::Matrix worldToCamera = p3d::context->GetView()->GetCamera()->GetWorldToCameraMatrix();
    float zValues[256];
    
    int i;
    for (i = 0; i < numEmitters; i++)
    {
        rmt::Vector tempPos = emitters[emitterDrawOrder[i]]->GetTransformMatrix().Row(3);
        zValues[i] = worldToCamera.m[0][2]*tempPos.x + worldToCamera.m[1][2]*tempPos.y + worldToCamera.m[2][2]*tempPos.z + worldToCamera.m[3][2];
    }

    //use bubble sort as particle systems will usually have small numbers of emitters
    bool done = false;
    while(!done)
    {
        done = true;
        for (i = 0; i < numEmitters-1; i++)
        {
            if (zValues[i] < zValues[i+1])
            {
                float tempZ = zValues[i];
                zValues[i] = zValues[i+1];
                zValues[i+1] = tempZ;            
                
                int tempDrawOrder = emitterDrawOrder[i];
                emitterDrawOrder[i] = emitterDrawOrder[i+1];
                emitterDrawOrder[i+1] = tempDrawOrder;
                done = false;
            }
        }
    }
}

    
void 
tParticleSystem::ConvertEmittersToLocal()
{
    for ( int i = 0 ; i < numEmitters ; i++ )
    {
        rAssert( emitters[ i ] != NULL );

        //Chuck: and Ian hopefully fixing a very bad memory fragmentation caused by the emitters
        //Rather and releasing the odd emitter and creating a new one from the heap
        //we will recycle the old emitter.

        tBaseEmitter* be = emitters[ i ];
        rAssert( be != NULL );
        tSpriteEmitter* se = dynamic_cast< tSpriteEmitter* >( be );
        if( se != NULL )
        {
            se->SetLocal( true );
        }
        //tBaseEmitter* localEmitter = emitters[i]->GetFactory()->CreateLocalEmitter();
        //emitters[i]->ReleaseVerified();
        //emitters[i] = NULL;
        //tRefCounted::Assign( emitters[i], localEmitter );
    }
}

// The following two functions do expensive calculations to attempt to determine
// the largest box/sphere than encapsulates the particle system. 
// The box will *not* be optimal, and will probably be too large.
// Definitely do not to use these functions every frame. 
void tParticleSystem::ComputePreciseBoundingBox( rmt::Box3D* pbox )
{
    rmt::Box3D box;

    // Make sure the returned box accounts for all the emitters
    for( int i = 0 ; i < GetNumEmitters() ; i++ )
    {
        tBaseEmitter* emitter = GetEmitter( i );
        if ( emitter )
        {
            rmt::Box3D emitterBox;
            emitter->ComputePreciseBoundingBox( &emitterBox );
            box.Expand( emitterBox.low );
            box.Expand( emitterBox.high );
        }
    }
    *pbox = box;
}

void tParticleSystem::ComputePreciseBoundingSphere( rmt::Sphere* sphere )
{
    rmt::Box3D box;
    ComputePreciseBoundingBox( &box );
    *sphere = box.GetBoundingSphere();
}

