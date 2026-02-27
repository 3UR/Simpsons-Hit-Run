/*
 * choreo/animation.hpp
 */

#ifndef CHOREO_ANIMATION_HPP
#define CHOREO_ANIMATION_HPP


#include <poser/constants.hpp>

#include <choreo/internalanimation.hpp>
#include <choreo/constants.hpp>
#include <choreo/driver.hpp>


namespace choreo
{


class Partition;
class ScriptReader;
class ScriptWriter;
class BaseBank;
class Rig;
class AnimationFrame;
class AnimationRootDriver;
class AnimationJointDriver;
class AnimationFootDriver;
class AnimationDriver;


//---------------------------------------------------------------------------
// class Animation
//---------------------------------------------------------------------------

class Animation: public PuppetDriverFactory
{
public:

    Animation(Rig* rig,
              PURE3D_ANIMATION* anim,
              Partition* partition = 0,
              GroundFixupEnum groundFixupEnum = GROUND_FIXUP_ANIMATION_START);
    
    Rig* GetRig() const
        { return m_Rig; }

    PURE3D_ANIMATION* GetP3DAnimation() const
        { return m_Animation.GetAnimation(); }
    
    Partition* GetPartition() const
        { return m_Partition; }
    void SetPartition(Partition* partition);
    
    // weight & priority
    float GetWeight() const
        { return m_Weight; }
    void SetWeight(float weight)
        { m_Weight = weight; }
    
    int GetPriority() const
        { return m_Priority; }
    void SetPriority(int priority)
        { m_Priority = priority; }

    GroundFixupEnum GetGroundFixupEnum() const
        { return m_GroundFixupEnum; }
    float GetGroundFixup(float frame) const;
    
    bool IsCyclic() const
        { return m_IsCyclic; }
    void SetIsCyclic(bool isCyclic)
        { m_IsCyclic = isCyclic; }
    
    float GetFramesPerSecond() const
        { return m_Animation.GetFramesPerSecond(); }
    
    float GetFrameCount() const
        { return m_Animation.GetFrameCount(); }

    float GetDuration() const
        { return m_Duration; }
    void SetDuration(float duration)
        { m_Duration = duration; }
    
    float GetSpeed() const
        { return m_Speed; }
    void SetSpeed(float speed)
        { m_Speed = speed; }
    
    float GetStartFrame() const
        { return m_StartFrame; }
    void SetStartFrame(float startFrame)
        { m_StartFrame = startFrame; }
    
    float GetEndFrame() const
        { return m_EndFrame; }
    void SetEndFrame(float endFrame)
        { m_EndFrame = endFrame; }

    static float MakeValidFrame(float frame, float startFrame, float endFrame, bool cyclic);

    float GetRootBlendInTime() const
        { return m_RootBlendInTime; }
    void SetRootBlendInTime(float rootBlendInTime)
        { m_RootBlendInTime = rootBlendInTime; }
    float GetRootBlendOutTime() const
        { return m_RootBlendOutTime; }
    void SetRootBlendOutTime(float rootBlendOutTime)
        { m_RootBlendOutTime = rootBlendOutTime; }
    
    float GetBlendInTime() const
        { return m_BlendInTime; }
    void SetBlendInTime(float blendInTime)
        { m_BlendInTime = blendInTime; }
    float GetBlendOutTime() const
        { return m_BlendOutTime; }
    void SetBlendOutTime(float blendOutTime)
        { m_BlendOutTime = blendOutTime; }
    
    bool HasJointTranslation(int jointIndex) const
        { return m_Animation.HasJointTranslation(jointIndex); }
    void GetJointTranslation(int jointIndex, float frame, rmt::Vector& translation) const
        { m_Animation.GetJointTranslation(jointIndex, frame, translation); }
    
    bool HasJointRotation(int jointIndex) const
        { return m_Animation.HasJointRotation(jointIndex); }
    void GetJointRotation(int jointIndex, float frame, rmt::Quaternion& rotation) const
        { m_Animation.GetJointRotation(jointIndex, frame, rotation); }
    
    bool HasGroupTranslation(tUID group) const
        { return m_Animation.HasGroupTranslation(group); }
    void GetGroupTranslation(tUID group, float frame, rmt::Vector& translation) const
        { m_Animation.GetGroupTranslation(group, frame, translation); }
    
    bool HasGroupRotation(tUID group) const
        { return m_Animation.HasGroupRotation(group); }
    void GetGroupRotation(tUID group, float frame, rmt::Quaternion& rotation) const
        { m_Animation.GetGroupRotation(group, frame, rotation); }
    
    virtual PuppetDriver* NewDriver();
    AnimationDriver* NewAnimationDriver();
    
    static bool ReadFromScript(ScriptReader* script, BaseBank* bank);
    
#ifdef CHOREO_TOOL
    static bool WriteToScriptHandler(ScriptWriter* script, BaseBank* bank);
    bool WriteToScript(ScriptWriter* script) const;
#endif // CHOREO_TOOL

protected:
    
    virtual ~Animation();
    
private:
    
    Rig* m_Rig;
    InternalAnimation m_Animation;
    Partition* m_Partition;
    float m_Weight;
    int m_Priority;

    GroundFixupEnum m_GroundFixupEnum;
    float* m_GroundFixupFrames;
    
    bool m_IsCyclic;
    float m_Duration;
    float m_Speed;
    float m_StartFrame;
    float m_EndFrame;
    
    float m_RootBlendInTime;
    float m_RootBlendOutTime;
    
    float m_BlendInTime;
    float m_BlendOutTime;
};


//---------------------------------------------------------------------------
// class AnimationDriver
//---------------------------------------------------------------------------

class AnimationDriver: public PuppetDriver
{
public:
    
    AnimationDriver(Animation* anim);
    
    virtual PuppetDriver* Clone() const;
    
    Animation* GetAnimation() const
        { return m_Animation; }
    
    // partition, weight & priority
    Partition* GetPartition() const;
    void SetPartition(Partition* partition);
    
    float GetWeight() const;
    void SetWeight(float weight);
    
    int GetPriority() const;
    void SetPriority(int priority);

    // blend times -- overrides those in Animation
    float GetRootBlendInTime() const;
    void SetRootBlendInTime(float rootBlendInTime);
    float GetRootBlendOutTime() const;
    void SetRootBlendOutTime(float rootBlendOutTime);
    
    float GetBlendInTime() const;
    void SetBlendInTime(float blendInTime);
    float GetBlendOutTime() const;
    void SetBlendOutTime(float blendOutTime);

    // animation parameters
    bool IsCyclic() const
        { return m_IsCyclic; }
    void SetIsCyclic(bool isCyclic)
        { m_IsCyclic = isCyclic; }
    
    float GetFramesPerSecond() const
        { return m_Animation->GetFramesPerSecond(); }
    
    float GetSpeed() const
        { return m_Speed; }
    void SetSpeed(float speed);
    
    float GetStartFrame() const
        { return m_StartFrame; }
    void SetStartFrame(float startFrame);
    
    float GetEndFrame() const
        { return m_EndFrame; }
    void SetEndFrame(float endFrame);
    
    void SetPlaybackTimings(float duration,
                            float speed,
                            float startFrame,
                            float endFrame);
    float GetCycleDuration() const;
    float GetDuration() const;
    float GetTimeRemaining() const;
    
    // frame accessors
    float GetFrameCount() const
        { return m_Animation->GetFrameCount(); }
    
    float GetValidFrame(float frame) const;
    float GetFrame() const;
    float GetNextFrame(float deltaTime) const;
    void SetFrame(float frame);
    
    void Reset();

    bool GetHoldEndFrame() const
        { return m_HoldEndFrame; }
    void SetHoldEndFrame(bool holdEndFrame)
        { m_HoldEndFrame = holdEndFrame; }
    
    // enable root displacement
    bool HasRootTranslation() const;
    void SetHasRootTranslation(bool isEnabled);

    // driver entry points
    virtual void Begin(Puppet* puppet);
    virtual void Advance(float deltaTime);
    virtual bool IsFinished() const;
    virtual void End(Puppet* puppet);
    
    virtual void Apply(Engine* engine);
    
protected:
    
    virtual ~AnimationDriver();
    
private:
    float MakeValidFrame(float frame, float startFrame, float endFrame) const;
    
    Animation* m_Animation;
    AnimationFrame* m_AnimationFrame;
    AnimationRootDriver* m_AnimationRootDriver;
    AnimationJointDriver* m_AnimationJointDriver;
    AnimationFootDriver** m_AnimationFootDrivers;
    
    bool m_IsCyclic;
    float m_Speed;
    float m_StartFrame;
    float m_EndFrame;

    bool m_Reset;
    bool m_HoldEndFrame;
    Puppet* m_Puppet;
};


} // namespace choreo


#endif
