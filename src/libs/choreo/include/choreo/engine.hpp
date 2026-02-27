/*
 * choreo/engine.hpp
 */

#ifndef CHOREO_ENGINE_HPP
#define CHOREO_ENGINE_HPP


#include <radmath/radmath.hpp>
#include <p3d/refcounted.hpp>
#include <poser/poseengine.hpp>
#include <choreo/rootblender.hpp>


class tPose;


namespace sim
{
    class SimJntMatTarget;
    class IKDriver;
}


namespace choreo
{


class Rig;
class Driver;
class JointBlender;
class ReplayBlender;
class FootBlender;
class FootPlanter;
class CompletePartition;
class LODPartition;


class Engine: public tRefCounted
{
public:

    enum
    {
        DEFAULT_MAX_PROP_COUNT = 0
    };

    Engine(tPose* pose, Rig* rig,
        bool enableReplay = false,
        int maxPropCount = DEFAULT_MAX_PROP_COUNT);
    
    // pose accessors
    poser::Pose* GetPose()
        { return m_PoseEngine->GetPose(); }
    const poser::Pose* GetPose() const
        { return m_PoseEngine->GetPose(); }
    tPose* GetP3DPose() const
        { return m_PoseEngine->GetP3DPose(); }
    
    // rig accessors
    Rig* GetRig() const
        { return m_Rig; }
    
    // component accessors
    poser::PoseEngine* GetPoseEngine() const
        { return m_PoseEngine; }
    RootBlender* GetRootBlender() const
        { return m_RootBlender; }
    JointBlender* GetJointBlender() const
        { return m_JointBlender; }
    ReplayBlender* GetReplayBlender() const
        { return m_ReplayBlender; }

    // LOD partition
    LODPartition* GetLODPartition() const;
    void SetLODPartition(LODPartition* lodPartition);

    // replay accessors
    bool IsReplayEnabled() const
        { return (m_ReplayBlender != 0); }
    
    // leg blender accessors
    FootBlender* GetFootBlender(int i) const;
    FootPlanter* GetFootPlanter() const
        { return m_FootPlanter; }
    
    // position & orientation
    const poser::Transform& GetRootTransform() const
        { return m_RootBlender->GetRootTransform(); }
    void SetRootTransform(const poser::Transform& rootTransform)
        { m_RootBlender->SetRootTransform(rootTransform); }

    void SetParentTransform( const poser::Transform& parentTransform )
    {
        m_RootBlender->SetParentTransform( parentTransform );
    }
    const poser::Transform& GetPrevRootTransform() const
        { return m_RootBlender->GetPrevRootTransform(); }

    const rmt::Vector& GetPrevPosition() const
        { return m_RootBlender->GetPrevPosition(); }
    const rmt::Vector& GetPosition() const
        { return m_RootBlender->GetPosition(); }
    void SetPosition(const rmt::Vector& pos)
        { m_RootBlender->SetPosition(pos); }
    
    float GetPrevOrientation() const
        { return m_RootBlender->GetPrevOrientation(); }
    float GetOrientation() const
        { return m_RootBlender->GetOrientation(); }
    void SetOrientation(float radians)
        { m_RootBlender->SetOrientation(radians); }
    
    rmt::Vector GetPrevFacingVector() const
        { return m_RootBlender->GetPrevFacingVector(); }
    rmt::Vector GetFacingVector() const
        { return m_RootBlender->GetFacingVector(); }
    void SetFacingVector(const rmt::Vector& facingVector)
        { m_RootBlender->SetFacingVector(facingVector); }

    // ground point
    void SetGroundPoint(const rmt::Vector& groundPoint)
        { m_RootBlender->SetGroundPoint(groundPoint); }
    const rmt::Vector& GetGroundPoint() const
        { return m_RootBlender->GetGroundPoint(); }
    
    // space conversion
    void WorldToRootSpace(const rmt::Vector& worldSpace, rmt::Vector& rootSpace)
        { m_RootBlender->WorldToRootSpace(worldSpace, rootSpace); }
    void RootToWorldSpace(const rmt::Vector& rootSpace, rmt::Vector& worldSpace)
        { m_RootBlender->RootToWorldSpace(rootSpace, worldSpace); }
    
    // velocity & turning
    const rmt::Vector& GetVelocity() const
        { return m_RootBlender->GetVelocity(); }
    float GetTurningVelocity() const
        { return m_RootBlender->GetTurningVelocity(); }
    
    // foot positioning
    int GetLegCount() const;
    bool IsLegIKEnabled(int i) const;
    void SetIsLegIKEnabled(int i, bool isEnabled);
    bool IsFootPlanted(int foot) const;
    rmt::Vector GetFootPosition(int foot) const;
    void SetFootGroundPoint(int foot, const rmt::Vector& groundPoint);
    rmt::Vector GetFootGroundPoint(int foot) const;

    // props
    bool AttachProp(int attachJoint, tPose* p3dPose);
    bool RemoveAttachedProp(tPose* p3dPose);
    bool HasAttachedProp(tPose* p3dPose) const;
    int GetAttachedPropJoint(tPose* p3dPose) const;

    // pose update
    int GetPassCount() const
        { return m_PoseEngine->GetPassCount(); }
    
    void Reset();
    
    // driver pushing
    void Push(Driver* driver);
    
    // time advancement
    void Advance(float dt);

    // pose update - should be called in this order
    void UpdateBegin();
    void UpdateRoot();
    void UpdatePose();
    void UpdateEnd();

    // replay playback
    void AdvanceReplay(float dt);
    void UpdateReplay();
    
protected:
    
    virtual ~Engine();
    
private:
    
    Rig* m_Rig;
    
    poser::PoseEngine* m_PoseEngine;
    RootBlender* m_RootBlender;
    RootFixupDriver* m_RootFixupDriver;

    JointBlender* m_JointBlender;
    CompletePartition* m_FootMaskPartition;

    ReplayBlender* m_ReplayBlender;
    
    FootBlender** m_FootBlenders;
    FootPlanter* m_FootPlanter;
    //sim::IKDriver** m_LegIKDrivers;

    float m_DeltaTime;
};


} // namespace choreo


#endif
