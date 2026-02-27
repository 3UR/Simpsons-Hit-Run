/*
 * choreo/puppet.hpp
 */

#ifndef CHOREO_PUPPET_HPP
#define CHOREO_PUPPET_HPP


#include <radmath/radmath.hpp>

#include <p3d/entity.hpp>

#include <poser/pose.hpp>

#include <choreo/buildconfig.hpp>
#include <choreo/engine.hpp>
#include <choreo/bank.hpp>
#include <choreo/rig.hpp>


namespace choreo
{


class PuppetDriver;
class ScriptReader;
class ScriptWriter;
class BaseBank;
class AnimationDriver;
class LocomotionDriver;
class SynchronizationDriver;
class Partition;


class Puppet: public tEntity
{
public:
    void AddRef(void)
    {
        tRefCounted::AddRef();
    }

    enum
    {
        DEFAULT_MAX_DRIVER_COUNT = 32,
        DEFAULT_MAX_PROP_COUNT = 0
    };
    
    Puppet(tPose* pose,
           Bank* bank,
           bool enableReplay = false,
           int maxDriverCount = DEFAULT_MAX_DRIVER_COUNT,
           int maxPropCount = DEFAULT_MAX_PROP_COUNT);
    
    // pose accessors
    poser::Pose* GetPose();
    const poser::Pose* GetPose() const;
    tPose* GetP3DPose() const;
    
    // engine accessors
    Engine* GetEngine() const
        { return m_Engine; }
    
    // bank accessors
    Bank* GetBank() const
        { return m_Bank; }
    bool SetBank(Bank* bank);
    
    // rig accessors
    Rig* GetRig() const
        { return m_Bank->GetRig(); }

    // skeleton accessors
    tSkeleton* GetSkeleton() const
        { return m_Bank->GetRig()->GetSkeleton(); }
    
    // position & orientation
    const poser::Transform& GetRootTransform() const
        { return m_Engine->GetRootTransform(); }
    void SetRootTransform(const poser::Transform& rootTransform)
        { m_Engine->SetRootTransform(rootTransform); }

    void SetParentTransform( const poser::Transform& parentTransform )
    {
        m_Engine->SetParentTransform( parentTransform );
    }
    const poser::Transform& GetPrevRootTransform() const
        { return m_Engine->GetPrevRootTransform(); }

    const rmt::Vector& GetPrevPosition() const
        { return m_Engine->GetPrevPosition(); }
    const rmt::Vector& GetPosition() const
        { return m_Engine->GetPosition(); }
    void SetPosition(const rmt::Vector& pos)
        { m_Engine->SetPosition(pos); }
    
    float GetPrevOrientation() const
        { return m_Engine->GetPrevOrientation(); }
    float GetOrientation() const
        { return m_Engine->GetOrientation(); }
    void SetOrientation(float radians)
        { m_Engine->SetOrientation(radians); }
    
    rmt::Vector GetPrevFacingVector() const
        { return m_Engine->GetPrevFacingVector(); }
    rmt::Vector GetFacingVector() const
        { return m_Engine->GetFacingVector(); }
    void SetFacingVector(const rmt::Vector& facingVector)
        { m_Engine->SetFacingVector(facingVector); }

    // ground positioning
    void SetGroundPoint(const rmt::Vector& groundPoint)
        { m_Engine->SetGroundPoint(groundPoint); }
    const rmt::Vector& GetGroundPoint() const
        { return m_Engine->GetGroundPoint(); }
    
    // space conversion
    void WorldToRootSpace(const rmt::Vector& worldSpace, rmt::Vector& rootSpace)
        { m_Engine->WorldToRootSpace(worldSpace, rootSpace); }
    void RootToWorldSpace(const rmt::Vector& rootSpace, rmt::Vector& worldSpace)
        { m_Engine->RootToWorldSpace(rootSpace, worldSpace); }

    // puppet space == root space, it just might be less confusing
    void WorldToPuppetSpace(const rmt::Vector& worldSpace, rmt::Vector& puppetSpace)
        { m_Engine->WorldToRootSpace(worldSpace, puppetSpace); }
    void PuppetToWorldSpace(const rmt::Vector& puppetSpace, rmt::Vector& worldSpace)
        { m_Engine->RootToWorldSpace(puppetSpace, worldSpace); }
    
    // velocity & turning
    const rmt::Vector& GetVelocity() const
        { return m_Engine->GetVelocity(); }
    float GetTurningVelocity() const
        { return m_Engine->GetTurningVelocity(); }
    
    // LOD partition
    LODPartition* GetLODPartition() const
        { return m_Engine->GetLODPartition(); }
    void SetLODPartition(LODPartition* lodPartition)
        { m_Engine->SetLODPartition(lodPartition); }

    // foot positioning
    int GetLegCount() const
        { return m_Engine->GetLegCount(); }
    bool IsLegIKEnabled(int leg) const
        { return m_Engine->IsLegIKEnabled(leg); }
    void SetIsLegIKEnabled(int leg, bool isEnabled) const
        { m_Engine->SetIsLegIKEnabled(leg, isEnabled); }
    bool IsFootPlanted(int foot) const
        { return m_Engine->IsFootPlanted(foot); }
    rmt::Vector GetFootPosition(int foot) const
        { return m_Engine->GetFootPosition(foot); }
    void SetFootGroundPoint(int foot, const rmt::Vector& groundPoint)
        { m_Engine->SetFootGroundPoint(foot, groundPoint); }
    rmt::Vector GetFootGroundPoint(int foot) const
        { return m_Engine->GetFootGroundPoint(foot); }
    
    // replay accessors
    bool IsReplayEnabled() const
        { return m_Engine->IsReplayEnabled(); }

    // props
    bool AttachProp(int attachJoint, tPose* p3dPose)
        { return m_Engine->AttachProp(attachJoint, p3dPose); }
    bool RemoveAttachedProp(tPose* p3dPose)
        { return m_Engine->RemoveAttachedProp(p3dPose); }
    bool HasAttachedProp(tPose* p3dPose) const
        { return m_Engine->HasAttachedProp(p3dPose); }
    int GetAttachedPropJoint(tPose* p3dPose) const
        { return m_Engine->GetAttachedPropJoint(p3dPose); }

    // animation playback
    AnimationDriver* PlayAnimation(tUID animUID,
                                   float duration = -1.0f,
                                   float speed = 1.0f,
                                   float startFrame = 0.0f,
                                   float endFrame = -1.0f,
                                   float weight = 1.0f,
                                   int priority = 0,
                                   Partition* partition = 0);
    AnimationDriver* PlayAnimation(const char* animName,
                                   float duration = -1.0f,
                                   float speed = 1.0f,
                                   float startFrame = 0.0f,
                                   float endFrame = -1.0f,
                                   float weight = 1.0f,
                                   int priority = 0,
                                   Partition* partition = 0);
    
    bool StopAnimation(tUID animUID);
    bool StopAnimation(const char* animName);
    
    bool IsAnimationFinished(tUID animUID) const;
    bool IsAnimationFinished(const char* animName) const;
    
    bool IsAnimationPlaying(tUID animUID) const;
    bool IsAnimationPlaying(const char* animName) const;
    
    float GetAnimationTimeRemaining(tUID animUID) const;
    float GetAnimationTimeRemaining(const char* animName) const;
    
    float IsAnimationIndefinite(tUID animUID) const;
    float IsAnimationIndefinite(const char* animName) const;
    
    // locomotion playback
    LocomotionDriver* PlayLocomotion(tUID locoUID,
                                     float motionAngle,
                                     float facingAngle,
                                     float velocity,
                                     float weight = 1.0f,
                                     int priority = 0,
                                     Partition* partition = 0);
    LocomotionDriver* PlayLocomotion(const char* locoName,
                                     float motionAngle,
                                     float facingAngle,
                                     float velocity,
                                     float weight = 1.0f,
                                     int priority = 0,
                                     Partition* partition = 0);
    
    // locomotion synchronization playback
    SynchronizationDriver* PlaySynchronization(tUID locoUID,
                                               float duration,
                                               const rmt::Vector& targetPos,
                                               float targetOrient,
                                               float weight = 1.0f,
                                               int priority = 0,
                                               Partition* partition = 0);
    SynchronizationDriver* PlaySynchronization(const char* locoName,
                                               float duration,
                                               const rmt::Vector& targetPos,
                                               float targetOrient,
                                               float weight = 1.0f,
                                               int priority = 0,
                                               Partition* partition = 0);
    
    bool SetSynchronizationTarget(tUID locoUID,
                                  const rmt::Vector& targetPos,
                                  float targetOrient);
    bool SetSynchronizationTarget(const char* locoName,
                                  const rmt::Vector& targetPos,
                                  float targetOrient);
    
    bool GetSynchronizationTarget(tUID locoUID,
                                  rmt::Vector& targetPos,
                                  float& targetOrient) const;
    bool GetSynchronizationTarget(const char* locoName,
                                  rmt::Vector& targetPos,
                                  float& targetOrient) const;
    
    bool StopSynchronization(tUID locoUID);
    bool StopSynchronization(const char* locoName);
    
    bool IsSynchronizationFinished(tUID locoUID) const;
    bool IsSynchronizationFinished(const char* locoName) const;
    
    bool IsSynchronizationPlaying(tUID locoUID) const;
    bool IsSynchronizationPlaying(const char* locoName) const;
    
    float GetSynchronizationTimeRemaining(tUID locoUID) const;
    float GetSynchronizationTimeRemaining(const char* locoName) const;
    
    // driver playback
    bool PlayDriver(PuppetDriver* driver, float duration, bool restart = true);
    bool StopDriver(PuppetDriver* driver);
    
    bool IsDriverFinished(PuppetDriver* driver) const;
    bool IsDriverPlaying(PuppetDriver* driver) const
        { return !IsDriverFinished(driver); }
    
    float GetDriverTimeRemaining(PuppetDriver* driver) const;
    bool IsDriverIndefinite(PuppetDriver* driver) const;
    bool GetDriverTimings(PuppetDriver* driver, bool& isIndefinite, float& timeRemaining) const;

    bool SetDriverTimeRemaining(PuppetDriver* driver, float timeRemaining);
    bool SetIsDriverIndefinite(PuppetDriver* driver, bool isIndefinite);
    bool SetDriverTimings(PuppetDriver* driver, bool isIndefinite, float timeRemaining);
    
    void StopAllDrivers();

    // current driver accessors
    int GetDriverCount() const
        { return m_DriverCount; }
    PuppetDriver* GetDriver(int i) const;

    // returns true if ANY drivers are currently unfinished
    bool IsBusy() const;
    
    // currently running (unfinished) driver accessors
    int GetBusyDriverCount() const;
    PuppetDriver* GetBusyDriver(int i) const;
    
    // reset all positioning
    void Reset()
        { m_Engine->Reset(); }
    
    // advancement
    void Advance(float deltaTime);

    // pose update - should be called in this order
    void UpdateBegin();
    void UpdateRoot();
    void UpdatePose();
    void UpdateEnd();

    // replay
    void AdvanceReplay(float deltaTime);
    void UpdateReplay();
    
    // loading
    static bool ReadFromScript(ScriptReader* script, BaseBank* bank);

#ifdef CHOREO_TOOL
    static bool WriteToScriptHandler(ScriptWriter* script, BaseBank* bank);
    bool WriteToScript(ScriptWriter* script) const;
#endif // CHOREO_TOOL

protected:
    
    virtual ~Puppet();
    
private:
    
    bool AddDriver(PuppetDriver* driver, float duration, bool indefinite);
    
    Engine* m_Engine;
    Bank* m_Bank;
    
    struct DriverStruct
    {
        PuppetDriver* driver;
        float duration;
        bool indefinite;
    };
    
    int m_MaxDriverCount;
    int m_DriverCount;
    DriverStruct* m_Drivers;
};


} // namespace choreo


#endif
