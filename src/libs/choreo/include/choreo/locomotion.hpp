/*
 * choreo/locomotion.hpp
 */

#ifndef CHOREO_LOCOMOTION_HPP
#define CHOREO_LOCOMOTION_HPP


#include <choreo/buildconfig.hpp>
#include <choreo/internalanimation.hpp>
#include <choreo/blend.hpp>


namespace choreo
{


class Rig;
class Partition;
class Transition;
class LocomotionDriver;
class SynchronizationDriver;
class LocomotionRootDriver;
class AnimationDriver;


//---------------------------------------------------------------------------
// class LocomotionSlot
//   - external class
//---------------------------------------------------------------------------

class LocomotionSlot: public BlendSlot
{
public:

    LocomotionSlot(Rig* rig, PURE3D_ANIMATION* animation);

    float GetVelocity() const
        { return m_Velocity; }

    virtual const char* GetName() const
        { return m_Animation.GetName(); }
    PURE3D_ANIMATION* GetAnimation()
        { return m_Animation.GetAnimation(); }

    virtual float GetFrameCount() const
        { return m_Animation.GetFrameCount(); }
    virtual float GetFramesPerSecond() const
        { return m_Animation.GetFramesPerSecond(); }

    virtual float GetCycleStart() const
        { return m_FootInfo->GetCycleStart(); }

    virtual float GetFootPlant(int foot) const
        { return m_FootInfo->GetFootPlant(foot); }
    virtual float GetFootLift(int foot) const
        { return m_FootInfo->GetFootLift(foot); }
    virtual float GetFootScale(int foot) const
        { return m_FootInfo->GetFootScale(foot); }

    virtual bool HasJointTranslation(int jointIndex) const
        { return m_Animation.HasJointTranslation(jointIndex); }
    virtual void GetJointTranslation(int jointIndex,
                                     float frame,
                                     rmt::Vector& translate) const
        { m_Animation.GetJointTranslation(jointIndex, frame, translate); }

    virtual bool HasJointRotation(int jointIndex) const
        { return m_Animation.HasJointRotation(jointIndex); }
    virtual void GetJointRotation(int jointIndex,
                                  float frame,
                                  rmt::Quaternion& rotate) const
        { m_Animation.GetJointRotation(jointIndex, frame, rotate); }

    virtual bool HasGroupTranslation(tUID group) const
        { return m_Animation.HasGroupTranslation(group); }
    virtual void GetGroupTranslation(tUID group,
                                     float frame,
                                     rmt::Vector& translate) const
        { m_Animation.GetGroupTranslation(group, frame, translate); }

    virtual bool HasGroupRotation(tUID group) const
        { return m_Animation.HasGroupRotation(group); }
    virtual void GetGroupRotation(tUID group,
                                  float frame,
                                  rmt::Quaternion& rotate) const
        { m_Animation.GetGroupRotation(group, frame, rotate); }

protected:

    virtual ~LocomotionSlot();

private:

    InternalAnimation m_Animation;
    BlendSlotFootInfo* m_FootInfo;
    float m_Velocity;
};


//---------------------------------------------------------------------------
// class Locomotion
//   - external class
//---------------------------------------------------------------------------

class Locomotion: public PuppetDriverFactory
{
public:

    Locomotion(Rig* rig,
               int directionCount,
               int velocityCount,
               PURE3D_ANIMATION** animations,
               Partition* partition = 0,
               int transitionCount = 0,
               Transition** transitions = 0);
    
    // blend accessor
    Blend* GetBlend() const
        { return m_Blend; }

    // rig accessor
    Rig* GetRig() const
        { return m_Blend->GetRig(); }
    
    // animation count information
    int GetDirectionCount() const
        { return m_DirectionCount; }
    int GetVelocityCount() const
        { return m_VelocityCount; }
    
    // per joint partition/priority info
    Partition* GetPartition() const
        { return m_Blend->GetPartition(); }
    void SetPartition(Partition* partition)
        { m_Blend->SetPartition(partition); }
    
    // weight & priority
    float GetWeight() const
        { return m_Blend->GetWeight(); }
    void SetWeight(float weight)
        { m_Blend->SetWeight(weight); }
    
    int GetPriority() const
        { return m_Blend->GetPriority(); }
    void SetPriority(int priority)
        { m_Blend->SetPriority(priority); }
    
    // slot access
    int GetLocomotionSlotCount() const;
    LocomotionSlot* GetLocomotionSlot(int i) const;
    LocomotionSlot* GetLocomotionSlot(int velIndex, int dirIndex) const;
    float GetLocomotionSlotVelocity(int i) const;
    float GetLocomotionSlotVelocity(int velIndex, int dirIndex) const;
    
    // root blend times
    float GetRootBlendInTime() const
        { return m_Blend->GetRootBlendInTime(); }
    void SetRootBlendInTime(float rootBlendInTime)
        { m_Blend->SetRootBlendInTime(rootBlendInTime); }
    float GetRootBlendOutTime() const
        { return m_Blend->GetRootBlendOutTime(); }
    void SetRootBlendOutTime(float rootBlendOutTime)
        { m_Blend->SetRootBlendOutTime(rootBlendOutTime); }
    
    // pose blend times
    float GetBlendInTime() const
        { return m_Blend->GetBlendInTime(); }
    void SetBlendInTime(float blendInTime)
        { m_Blend->SetBlendInTime(blendInTime); }
    float GetBlendOutTime() const
        { return m_Blend->GetBlendOutTime(); }
    void SetBlendOutTime(float blendOutTime)
        { m_Blend->SetBlendOutTime(blendOutTime); }
    
    // acceleration
    float GetAcceleration() const
        { return m_Acceleration; }
    void SetAcceleration(float acceleration)
        { m_Acceleration = acceleration; }
    
    // velocity
    float GetAnimMaxVelocity(float relativeFacingAngle) const;
    float GetAnimMaxVelocity(float motionAngle, float facingAngle) const;
    
    // turning velocity
    float GetTurningVelocity() const
        { return m_TurningVelocity; }
    void SetTurningVelocity(float turningVelocity)
        { m_TurningVelocity = turningVelocity; }
    
    // transition accessors
    int GetTransitionCount() const
        { return m_TransitionCount; }
    Transition* GetTransition(int i) const;
    
    // driver creation
    virtual PuppetDriver* NewDriver();
    LocomotionDriver* NewLocomotionDriver();
    SynchronizationDriver* NewSynchronizationDriver();
    
    static bool ReadFromScript(ScriptReader* script, BaseBank* bank);
    // convenience for 4-direction locomotions
    static bool ReadFromScript4(ScriptReader* script, BaseBank* bank);
    // convenience for 8-direction locomotions
    static bool ReadFromScript8(ScriptReader* script, BaseBank* bank);
    
#ifdef CHOREO_TOOL
    static bool WriteToScriptHandler(ScriptWriter* script, BaseBank* bank);
    bool WriteToScript(ScriptWriter* script) const;
#endif // CHOREO_TOOL

protected:
    
    virtual ~Locomotion();
    
private:

    void FillNullVelocities();
    bool SetLocomotionSlot(int velIndex, int dirIndex, PURE3D_ANIMATION* animation);

    int m_DirectionCount;
    int m_VelocityCount;
    Blend* m_Blend;
    LocomotionSlot** m_LocomotionSlots;
    float* m_LocomotionSlotVelocities;

    float m_Acceleration;
    float m_TurningVelocity;

    int m_TransitionCount;
    Transition** m_Transitions;
};


//---------------------------------------------------------------------------
// class LocomotionDriver
//   - external class
//---------------------------------------------------------------------------

class LocomotionDriver: public PuppetDriver
{
public:
    
    LocomotionDriver(Locomotion* loco);
    
    virtual PuppetDriver* Clone() const;
    
    // locomotion accessor
    Locomotion* GetLocomotion() const
        { return m_Locomotion; }
    
    // partition, weight & priority
    Partition* GetPartition() const;
    void SetPartition(Partition* partition);
    
    float GetWeight() const;
    void SetWeight(float weight);
    
    int GetPriority() const;
    void SetPriority(int priority);

    // blend times -- overrides those in Locomotion
    float GetRootBlendInTime() const;
    void SetRootBlendInTime(float rootBlendInTime);
    float GetRootBlendOutTime() const;
    void SetRootBlendOutTime(float rootBlendOutTime);
    
    float GetBlendInTime() const;
    void SetBlendInTime(float blendInTime);
    float GetBlendOutTime() const;
    void SetBlendOutTime(float blendOutTime);

    // motion angle accessors
    float GetActualMotionAngle() const;
    void SetActualMotionAngle(float motionAngle);
    float GetDesiredMotionAngle() const
        { return m_DesiredMotionAngle; }
    void SetDesiredMotionAngle(float motionAngle);
    
    rmt::Vector GetActualMotionVector() const;
    void SetActualMotionVector(const rmt::Vector& motion);
    rmt::Vector GetDesiredMotionVector() const;
    void SetDesiredMotionVector(const rmt::Vector& motion);
    
    // facing angle accessors
    float GetActualFacingAngle() const;
    void SetActualFacingAngle(float facingAngle);
    float GetDesiredFacingAngle() const
        { return m_DesiredFacingAngle; }
    void SetDesiredFacingAngle(float facingAngle);
    
    rmt::Vector GetActualFacingVector() const;
    void SetActualFacingVector(const rmt::Vector& facing);
    rmt::Vector GetDesiredFacingVector() const;
    void SetDesiredFacingVector(const rmt::Vector& facing);
    
    // velocity accessors
    float GetActualVelocity() const;
    void SetActualVelocity(float velocity);
    float GetDesiredVelocity() const
        { return m_DesiredVelocity; }
    void SetDesiredVelocity(float velocity);
    
    rmt::Vector GetActualVelocityVector() const;
    void SetActualVelocityVector(const rmt::Vector& velocity);
    rmt::Vector GetDesiredVelocityVector() const;
    void SetDesiredVelocityVector(const rmt::Vector& velocity);

    float GetAcceleration() const
        { return m_Acceleration; }
    void SetAcceleration(float acceleration)
        { m_Acceleration = acceleration; }

    float GetActualAcceleration() const
        { return m_ActualAcceleration; }

    float GetActualAnimMaxVelocity() const;
    float GetDesiredAnimMaxVelocity() const;
    float GetAnimMaxVelocity(float motionAngle, float facingAngle) const;
    
    // turning velocity accessors
    float GetTurningVelocity() const
        { return m_TurningVelocity; }
    void SetTurningVelocity(float turningVelocity)
        { m_TurningVelocity = turningVelocity; }
    
    // phase accessors
    float GetPhase() const;
    void SetPhase(float phase);
    
    // foot info accessors
    float GetFootPlant(int foot) const;
    float GetFootLift(int foot) const;
    float GetFootScale(int foot) const;
    float GetFootPhase(int foot) const;
    bool IsFootPlanted(int foot) const;
    
    float GetMaxFrameCount() const;

    // joint blend accessors
    float GetJointWeight(int jointIndex) const;
    int GetJointPriority(int jointIndex) const;

    // LOD accessors
    float GetBlendThreshold() const
        { return m_BlendDriver->GetBlendThreshold(); }
    void SetBlendThreshold(float blendThreshold)
        { m_BlendDriver->SetBlendThreshold(blendThreshold); }

    int GetMaxBlendedSlotCount() const
        { return m_BlendDriver->GetMaxBlendedSlotCount(); }
    void SetMaxBlendedSlotCount(int maxBlendedSlotCount)
        { m_BlendDriver->SetMaxBlendedSlotCount(maxBlendedSlotCount); }

    // enable root displacement
    bool IsRootDriverEnabled() const
        { return m_IsRootDriverEnabled; }
    void SetIsRootDriverEnabled(bool isEnabled)
        { m_IsRootDriverEnabled = isEnabled; }

    // initializes motion params from engine
    void InitMotion(Engine* engine);
    // initializes facing params from engine
    void InitFacing(Engine* engine);
    // initializes both motion & facing from engine
    void Init(Engine* engine);
    // initializes phase given foot info in engine
    // NOTE:  expensive!
    void FootMatch(Engine* engine);

    virtual void Begin(Puppet* puppet);
    virtual void Advance(float deltaTime);
    virtual bool IsFinished() const;
    virtual void End(Puppet* puppet);

    virtual void Apply(Engine* engine);
    
protected:
    
    virtual ~LocomotionDriver();
    
private:
    
    void UpdateTransition(float deltaTime);
    void UpdateFacing(float deltaTime);
    void UpdateMotion(float deltaTime);
    float UpdateBlends();
    
    Locomotion* m_Locomotion;
    LocomotionRootDriver* m_LocomotionRootDriver;
    BlendDriver* m_BlendDriver;

    float m_Weight;
    float m_AnimWeight;
    
    // used only in Begin() as temporary storage
    rmt::Vector* m_FootPos;

    float m_Acceleration;
    float m_TurningVelocity;

    float m_DesiredMotionAngle;
    float m_DesiredFacingAngle;
    float m_DesiredVelocity;

    float m_ActualAcceleration;

    AnimationDriver* m_TransitionDriver;

    bool m_IsRootDriverEnabled;
};


} // namespace choreo


#endif
