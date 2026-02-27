/*
 * choreo/engine.cpp
 */

#include <choreo/engine.hpp>
#include <choreo/rig.hpp>
#include <choreo/driver.hpp>

#include <choreo/utility.hpp>
#include <choreo/jointblender.hpp>
#include <choreo/replayblender.hpp>
#include <choreo/footblender.hpp>
#include <choreo/footplanter.hpp>
#include <choreo/partition.hpp>

#include <simcommon/simtarget.hpp>
//#include <simik/ikdriver.hpp>

#include <poser/joint.hpp>

#include <p3d/anim/skeleton.hpp>

#include<poser/joint.hpp>


namespace choreo
{


//---------------------------------------------------------------------------
// class FootBlenderEffectorSimTarget - interface
//---------------------------------------------------------------------------

class FootBlenderEffectorSimTarget: public sim::SimTarget
{
public:

    FootBlenderEffectorSimTarget(RootBlender* rootBlender,
                                 FootBlender* footBlender);

    virtual void GetPos(rmt::Vector& v, float dt) const;
    virtual void GetTransform(rmt::Matrix& m, float dt) const;

protected:

    virtual ~FootBlenderEffectorSimTarget();

private:

    RootBlender* m_RootBlender;
    FootBlender* m_FootBlender;
};


//---------------------------------------------------------------------------
// class FootBlenderEffectorSimTarget - implementation
//---------------------------------------------------------------------------

FootBlenderEffectorSimTarget::FootBlenderEffectorSimTarget(RootBlender* rootBlender,
                                                           FootBlender* footBlender)
{
    P3DASSERT(rootBlender != 0);
    m_RootBlender = rootBlender;
    m_RootBlender->AddRef();

    P3DASSERT(footBlender != 0);
    m_FootBlender = footBlender;
    m_FootBlender->AddRef();
}

FootBlenderEffectorSimTarget::~FootBlenderEffectorSimTarget()
{
    m_FootBlender->Release();
    m_RootBlender->Release();
}

void FootBlenderEffectorSimTarget::GetPos(rmt::Vector& v, float dt) const
{
    // doesn't get called
}

void FootBlenderEffectorSimTarget::GetTransform(rmt::Matrix& m, float dt) const
{
    m = m_FootBlender->GetEffector().GetMatrix();
    m.Mult(m_RootBlender->GetRootTransform().GetMatrix());
}


//---------------------------------------------------------------------------
// class FootBlenderPoleVectorSimTarget - interface
//---------------------------------------------------------------------------

class FootBlenderPoleVectorSimTarget: public sim::SimTarget
{
public:

    FootBlenderPoleVectorSimTarget(RootBlender* rootBlender,
                                   FootBlender* footBlender);

    virtual void GetPos(rmt::Vector& v, float dt) const;
    virtual void GetTransform(rmt::Matrix& m, float dt) const;

protected:

    virtual ~FootBlenderPoleVectorSimTarget();

private:

    RootBlender* m_RootBlender;
    FootBlender* m_FootBlender;
};


//---------------------------------------------------------------------------
// class FootBlenderPoleVectorSimTarget - implementation
//---------------------------------------------------------------------------

FootBlenderPoleVectorSimTarget::FootBlenderPoleVectorSimTarget(RootBlender* rootBlender,
                                                               FootBlender* footBlender)
{
    P3DASSERT(rootBlender != 0);
    m_RootBlender = rootBlender;
    m_RootBlender->AddRef();

    P3DASSERT(footBlender != 0);
    m_FootBlender = footBlender;
    m_FootBlender->AddRef();
}

FootBlenderPoleVectorSimTarget::~FootBlenderPoleVectorSimTarget()
{
    m_FootBlender->Release();
    m_RootBlender->Release();
}

void FootBlenderPoleVectorSimTarget::GetPos(rmt::Vector& v, float dt) const
{
    m_RootBlender->GetRootTransform().GetMatrix().Transform(m_FootBlender->GetPoleVector(), &v);
}

void FootBlenderPoleVectorSimTarget::GetTransform(rmt::Matrix& m, float dt) const
{
    // doesn't get called
}


//---------------------------------------------------------------------------
// class Engine - implementation
//---------------------------------------------------------------------------

Engine::Engine(tPose* pose,
               Rig* rig,
               bool enableReplay,
               int maxPropCount):
    m_DeltaTime(0.0f)
{
    // rig
    m_Rig = rig;
    m_Rig->AddRef();
    
    // engine - 5 passes
    m_PoseEngine = new poser::PoseEngine(pose, 5);
    m_PoseEngine->AddRef();
    
    // root
    m_RootBlender = new RootBlender(m_Rig->GetJointIndex(RIG_JOINT_AI_ROOT));
    m_RootBlender->AddRef();
    m_RootFixupDriver = new RootFixupDriver(m_RootBlender);
    m_RootFixupDriver->AddRef();
    
    // animation blender
    m_JointBlender = new JointBlender(JointBlender::DEFAULT_MAX_DRIVER_COUNT,
        JointBlender::DEFAULT_MAX_BLENDED_PRIORITIES, maxPropCount);
    m_JointBlender->AddRef();
    m_JointBlender->SetMinimumJointIndex(m_Rig->GetJointIndex(RIG_JOINT_CHARACTER_ROOT));

    // leg controllers
    int legCount = m_Rig->GetLegCount();
    m_FootBlenders = new FootBlender* [legCount];
    //m_LegIKDrivers = new sim::IKDriver* [legCount];
    
    bool hasIKLegRig = false;
    int i;
    for (i = 0; i < legCount; ++i)
    {
        RigLeg* leg = m_Rig->GetLeg(i);
        
        FootBlender* footBlender = new FootBlender(m_Rig, leg);
        m_FootBlenders[i] = footBlender;
        footBlender->AddRef();
        
        if (leg->IsIKRigged())
        {
            rAssert( false );
            /*
            FootBlenderEffectorSimTarget* effectorTarget;
            effectorTarget = new FootBlenderEffectorSimTarget(m_RootBlender, footBlender);
            
            FootBlenderPoleVectorSimTarget* poleVectorTarget;
            poleVectorTarget = new FootBlenderPoleVectorSimTarget(m_RootBlender, footBlender);
            
            m_LegIKDrivers[i] = new sim::IKDriver(m_PoseEngine->GetPose(),
                                                  leg->GetThighJointIndex(),
                                                  leg->GetKneeJointIndex(),
                                                  leg->GetAnkleJointIndex(),
                                                  sim::IKLimbKnee,
                                                  effectorTarget,
                                                  poleVectorTarget);
            m_LegIKDrivers[i]->AddRef();

            hasIKLegRig = true;
            */
        }
        else
        {
            //m_LegIKDrivers[i] = 0;
        }
    };
    
    // if we had any ik-rigged legs, make some partitions and planters
    if (hasIKLegRig)
    {
        m_FootMaskPartition = new CompletePartition(m_Rig);
        m_FootMaskPartition->AddRef();
//        m_JointBlender->SetMaskPartition(m_FootMaskPartition);

        m_FootPlanter = new FootPlanter(m_Rig,
                                        m_RootBlender,
                                        m_FootBlenders,
                                        m_Rig->GetJointIndex(RIG_JOINT_BALANCE_ROOT));
        m_FootPlanter->AddRef();

        // enable the ik
        for (i = 0; i < legCount; ++i)
        {
            SetIsLegIKEnabled(i, true);
        }
    }
    else
    {
        m_FootMaskPartition = 0;
        m_FootPlanter = 0;
    }

    if (enableReplay)
    {
        // replay blender
        m_ReplayBlender = new ReplayBlender;
        m_ReplayBlender->AddRef();
    }
    else
    {
        m_ReplayBlender = 0;
    }
    
    // first pass:  ai root resolution
    m_PoseEngine->AddPoseDriver(0, m_RootBlender);
    
    // second pass:  animations (pose evaluation)
    m_PoseEngine->AddPoseDriver(1, m_RootFixupDriver);
    m_PoseEngine->AddPoseDriver(1, m_JointBlender);
    for (i = 0; i < m_Rig->GetLegCount(); ++i)
    {
        if (m_FootBlenders[i] != 0)
        {
            m_PoseEngine->AddPoseDriver(1, m_FootBlenders[i]);
        }
    }
    
    // third pass:  foot-ground contacts
    if (hasIKLegRig)
    {
        m_PoseEngine->AddPoseDriver(2, m_FootPlanter);
    }
    
    // fourth pass:  physics/ik fixup
    for (i = 0; i < m_Rig->GetLegCount(); ++i)
    {
        RigLeg* leg = m_Rig->GetLeg(i);
        
        if (leg->IsIKRigged())
        {
            rAssert( false );
            //m_PoseEngine->AddPoseDriver(3, m_LegIKDrivers[i]);
        }
    }

    // fifth pass:  replay
    if (m_ReplayBlender != 0)
    {
        m_PoseEngine->AddPoseDriver(4, m_ReplayBlender);
    }
}

Engine::~Engine()
{
    tRefCounted::Release(m_ReplayBlender);
    tRefCounted::Release(m_FootPlanter);
    
    for (int i = m_Rig->GetLegCount() - 1; i >= 0; --i)
    {
        //tRefCounted::Release(m_LegIKDrivers[i]);
        tRefCounted::Release(m_FootBlenders[i]);
    }
    
    //delete[] m_LegIKDrivers;
    delete[] m_FootBlenders;
    
    if (m_FootMaskPartition != 0)
    {
        m_FootMaskPartition->Release();
    }
    m_JointBlender->Release();
    m_RootFixupDriver->Release();
    m_RootBlender->Release();
    m_PoseEngine->Release();
    
    m_Rig->Release();
}

LODPartition* Engine::GetLODPartition() const
{
    return NULL;//m_JointBlender->GetLODPartition();
}

void Engine::SetLODPartition(LODPartition* lodPartition)
{
    //m_JointBlender->SetLODPartition(lodPartition);
}

FootBlender* Engine::GetFootBlender(int i) const
{
    P3DASSERT((i >= 0) && (i < m_Rig->GetLegCount()));
    if ((i < 0) || (i >= m_Rig->GetLegCount()))
        return 0;
    
    return m_FootBlenders[i];
}

int Engine::GetLegCount() const
{
    return m_Rig->GetLegCount();
}

bool Engine::IsLegIKEnabled(int i) const
{
    P3DASSERT((i >= 0) && (i < m_Rig->GetLegCount()));
    if ((i < 0) || (i >= m_Rig->GetLegCount()))
        return false;

    //if (m_LegIKDrivers[i] == 0)
    //    return false;
    //return m_LegIKDrivers[i]->IsEnabled();
    rAssert( false );
    return false;
}

void Engine::SetIsLegIKEnabled(int i, bool isEnabled)
{
    int legCount = m_Rig->GetLegCount();
    P3DASSERT((i >= 0) && (i < legCount));
    if ((i < 0) || (i >= legCount))
        return;

    //if (m_LegIKDrivers[i] == 0)
    //    return;
    return;

    m_FootBlenders[i]->SetIsIKEnabled(isEnabled);
    //m_LegIKDrivers[i]->SetIsEnabled(isEnabled);

    RigLeg* leg = m_Rig->GetLeg(i);
    m_FootMaskPartition->SetHasJoint(leg->GetThighJointIndex(), !isEnabled);
    m_FootMaskPartition->SetHasJoint(leg->GetKneeJointIndex(), !isEnabled);
    // FIXME:  need the ankle rotation
    //m_FootMaskPartition->SetHasJoint(leg->GetAnkleJointIndex(), !isEnabled);

    //for (int j = 0; j < legCount; ++j)
    //{
        //if (m_LegIKDrivers[i]->IsEnabled() != isEnabled)
        //{
        //    return;
        //}
    //}

    m_FootPlanter->SetIsEnabled(isEnabled);
    m_FootPlanter->Reset();
}

bool Engine::IsFootPlanted(int foot) const
{
    P3DASSERT((foot >= 0) && (foot < m_Rig->GetLegCount()));
    if ((foot < 0) || (foot >= m_Rig->GetLegCount()))
        return false;
    
    return m_FootBlenders[foot]->IsFootPlanted();
}

rmt::Vector Engine::GetFootPosition(int foot) const
{
    P3DASSERT((foot >= 0) && (foot < m_Rig->GetLegCount()));
    if ((foot < 0) || (foot >= m_Rig->GetLegCount()))
        return rmt::Vector(0.0f, 0.0f, 0.0f);
    
    RigLeg* leg = m_Rig->GetLeg(foot);
    int ankleIndex = leg->GetAnkleJointIndex();
    return GetPose()->GetJoint(ankleIndex)->GetWorldTranslation();
}

void Engine::SetFootGroundPoint(int foot, const rmt::Vector& groundPoint)
{
    if (m_FootPlanter != 0)
    {
        m_FootPlanter->SetGroundPoint(foot, groundPoint);
    }
}

rmt::Vector Engine::GetFootGroundPoint(int foot) const
{
    if (m_FootPlanter == 0)
        return rmt::Vector(0.0f, 0.0f, 0.0f);
    return m_FootPlanter->GetGroundPoint(foot);
}

bool Engine::AttachProp(int attachJoint, tPose* p3dPose)
{
    return m_JointBlender->AttachProp(attachJoint, p3dPose);
}

bool Engine::RemoveAttachedProp(tPose* p3dPose)
{
    return m_JointBlender->RemoveAttachedProp(p3dPose);
}

bool Engine::HasAttachedProp(tPose* p3dPose) const
{
    return m_JointBlender->HasAttachedProp(p3dPose);
}

int Engine::GetAttachedPropJoint(tPose* p3dPose) const
{
    return m_JointBlender->GetAttachedPropJoint(p3dPose);
}

void Engine::Reset()
{
    if (m_FootPlanter != 0)
    {
        m_FootPlanter->Reset();
    }
}

void Engine::Push(Driver* driver)
{
    driver->Apply(this);
}

void Engine::Advance(float deltaTime)
{
    // kludge:  sim pose drivers don't like separate advance/update calls,
    //          so just do the object-space ones
    for (int i = 0; i < 2; ++i)
    {
        m_PoseEngine->Advance(i, deltaTime);
    }

    m_DeltaTime += deltaTime;
}

void Engine::UpdateBegin()
{
    // FIXME:  removed so pose doesn't reset?
    //m_PoseEngine->Begin();
    m_PoseEngine->GetPose()->Clean();
}

void Engine::UpdateRoot()
{
    // do ai root pass
    m_PoseEngine->Update(0);
}

void Engine::UpdatePose()
{
    // do animation pass
    m_PoseEngine->Update(1);
}

void Engine::UpdateEnd()
{
    int passCount = m_PoseEngine->GetPassCount();

    // complete passes
    for (int i = 2; i < passCount; ++i)
    {
        m_PoseEngine->Advance(i, m_DeltaTime);
        m_PoseEngine->Update(i);
    }
    
    m_PoseEngine->End();

    // update props
//    m_JointBlender->UpdateProps(m_PoseEngine->GetPose());

    m_DeltaTime = 0.0f;
}

void Engine::AdvanceReplay(float dt)
{
    if (m_ReplayBlender == 0)
        return;
    m_ReplayBlender->AdvanceReplay(dt);
}

void Engine::UpdateReplay()
{
    if (m_ReplayBlender == 0)
        return;
    m_PoseEngine->GetPose()->Clean();
    m_ReplayBlender->UpdateReplay(m_PoseEngine->GetPose());
    m_PoseEngine->End();
}


} // namespace choreo


// End of file.
