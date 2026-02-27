/*
 * choreo/blend.hpp
 */

#ifndef CHOREO_BLEND_HPP
#define CHOREO_BLEND_HPP


#include <choreo/buildconfig.hpp>
#include <choreo/driver.hpp>


namespace choreo
{


class Rig;
class Partition;
class BlendDriver;
class ScriptReader;
class ScriptWriter;
class BaseBank;
class BlendPhase;
class BlendJointDriver;
class BlendFootDriver;


//---------------------------------------------------------------------------
// class BlendSlot
//   - abstract slot interface
//---------------------------------------------------------------------------

class BlendSlot: public tRefCounted
{
public:

    virtual const char* GetName() const = 0;

    virtual float GetFrameCount() const = 0;
    virtual float GetFramesPerSecond() const = 0;

    virtual float GetCycleDuration() const;
    virtual float GetCycleStart() const
        { return 0.0f; }

    float FrameToPhase(float frame) const;
    float PhaseToFrame(float phase) const;

    virtual float GetFootPlant(int foot) const
        { return 0.0f; }
    virtual float GetFootLift(int foot) const
        { return 0.0f; }
    virtual float GetFootScale(int foot) const
        { return 0.0f; }

    virtual bool HasJointTranslation(int jointIndex) const = 0;
    virtual void GetJointTranslation(int jointIndex,
                                     float frame,
                                     rmt::Vector& translate) const = 0;

    virtual bool HasJointRotation(int jointIndex) const = 0;
    virtual void GetJointRotation(int jointIndex,
                                  float frame,
                                  rmt::Quaternion& rotate) const = 0;

    virtual bool HasGroupTranslation(tUID group) const = 0;
    virtual void GetGroupTranslation(tUID group,
                                     float frame,
                                     rmt::Vector& translate) const = 0;

    virtual bool HasGroupRotation(tUID group) const = 0;
    virtual void GetGroupRotation(tUID group,
                                  float frame,
                                  rmt::Quaternion& rotate) const = 0;

protected:

    virtual ~BlendSlot();
};


//---------------------------------------------------------------------------
// class BlendSlotFootInfo
//   - utility class for extracting foot info
//---------------------------------------------------------------------------

class BlendSlotFootInfo: public tRefCounted
{
public:

    BlendSlotFootInfo(Rig* rig, BlendSlot* blendSlot);

    float GetFootPlant(int foot) const;
    float GetFootLift(int foot) const;
    float GetFootScale(int foot) const;

    float GetCycleStart() const
        { return m_CycleStart; }

protected:

    virtual ~BlendSlotFootInfo();

private:

    float FrameToPhase(float frame, float frameCount) const;
    float PhaseToFrame(float phase, float frameCount) const;

    struct FootCycle
    {
        float plant;
        float lift;
        float scale;
    };

    int m_FootCount;
    FootCycle* m_Feet;

    float m_CycleStart;
};


//---------------------------------------------------------------------------
// class Blend
//   - external class
//---------------------------------------------------------------------------

class Blend: public PuppetDriverFactory
{
public:

    Blend(Rig* rig,
          int paramCount,
          const int* paramSlotCounts,
          Partition* partition = 0);

    // rig accessor
    Rig* GetRig() const
        { return m_Rig; }
    
    // blend slot information
    int GetParamCount() const
        { return m_ParamCount; }
    int GetParamSlotCount(int i) const;
    int GetBlendSlotIndex(const int* params) const;
    const BlendSlot* GetBlendSlot(int index) const;
    BlendSlot* GetBlendSlot(int index);
    bool SetBlendSlot(int index, BlendSlot* blendSlot);
    
    // per joint partition/priority info
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

    // root blend times
    float GetRootBlendInTime() const
        { return m_RootBlendInTime; }
    void SetRootBlendInTime(float rootBlendInTime)
        { m_RootBlendInTime = rootBlendInTime; }
    float GetRootBlendOutTime() const
        { return m_RootBlendOutTime; }
    void SetRootBlendOutTime(float rootBlendOutTime)
        { m_RootBlendOutTime = rootBlendOutTime; }

    // pose blend times
    float GetBlendInTime() const
        { return m_BlendInTime; }
    void SetBlendInTime(float blendInTime)
        { m_BlendInTime = blendInTime; }
    float GetBlendOutTime() const
        { return m_BlendOutTime; }
    void SetBlendOutTime(float blendOutTime)
        { m_BlendOutTime = blendOutTime; }
    
    // driver creation
    virtual PuppetDriver* NewDriver();
    BlendDriver* NewBlendDriver();
    
    static bool ReadFromScript(ScriptReader* script, BaseBank* bank);
    
#ifdef CHOREO_TOOL
    static bool WriteToScriptHandler(ScriptWriter* script, BaseBank* bank);
    bool WriteToScript(ScriptWriter* script) const;
#endif // CHOREO_TOOL

protected:
    
    virtual ~Blend();
    
private:

    Rig* m_Rig;

    int m_ParamCount;
    int* m_ParamSlotCounts;

    int m_BlendSlotCount;
    BlendSlot** m_BlendSlots;

    int m_UniqueBlendSlotCount;
    BlendSlot** m_UniqueBlendSlots;

    Partition* m_Partition;
    float m_Weight;
    int m_Priority;

    float m_RootBlendInTime;
    float m_RootBlendOutTime;
    float m_BlendInTime;
    float m_BlendOutTime;
};


//---------------------------------------------------------------------------
// class BlendDriver
//   - external class
//---------------------------------------------------------------------------

class BlendDriver: public PuppetDriver
{
public:
    
    BlendDriver(Blend* blend);
    
    virtual PuppetDriver* Clone() const;
    
    // blend accessor
    Blend* GetBlend() const
        { return m_Blend; }

    // partition, weight & priority
    Partition* GetPartition() const;
    void SetPartition(Partition* partition);
    
    float GetWeight() const;
    void SetWeight(float weight);
    
    int GetPriority() const;
    void SetPriority(int priority);

    // blend times -- overrides those in Blend
    float GetRootBlendInTime() const;
    void SetRootBlendInTime(float rootBlendInTime);
    float GetRootBlendOutTime() const;
    void SetRootBlendOutTime(float rootBlendOutTime);
    
    float GetBlendInTime() const;
    void SetBlendInTime(float blendInTime);
    float GetBlendOutTime() const;
    void SetBlendOutTime(float blendOutTime);

    // param accessors -- param values range from 0.0 - 1.0
    int GetParamCount() const
        { return m_Blend->GetParamCount(); }
    float GetParamValue(int i) const;
    void SetParamValue(int i, float value);

    // cyclic accessors
    bool IsCyclic() const;
    void SetIsCyclic(bool isCyclic);

    // phase accessors
    float GetPhase() const;
    void SetPhase(float phase);
    
    // foot info accessors
    float GetFootPlant(int foot) const;
    float GetFootLift(int foot) const;
    float GetFootScale(int foot) const;
    float GetFootPhase(int foot) const;
    bool IsFootPlanted(int foot) const;
    void GetFootEffectorTranslation(int foot, rmt::Vector& effTranslate) const;

    // joint blend accessors
    float GetJointWeight(int jointIndex) const;
    int GetJointPriority(int jointIndex) const;

    // currently blended slot accessors
    int GetBlendSlotCount() const;
    BlendSlot* GetBlendSlot(int i) const;

    // LOD accessors
    float GetBlendThreshold() const;
    void SetBlendThreshold(float blendThreshold);

    int GetMaxBlendedSlotCount() const;
    void SetMaxBlendedSlotCount(int maxSlotCount);

    // driver entry points
    virtual void Begin(Puppet* puppet);
    virtual void Advance(float deltaTime);
    virtual bool IsFinished() const;
    virtual void End(Puppet* puppet);
    
    virtual void Apply(Engine* engine);
    
protected:
    
    virtual ~BlendDriver();
    
private:

    Blend* m_Blend;
    BlendPhase* m_BlendPhase;
    BlendJointDriver* m_BlendJointDriver;
    BlendFootDriver** m_BlendFootDrivers;
};


} // namespace choreo


#endif
