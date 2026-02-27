/*
 * choreo/rig.hpp
 */

#ifndef CHOREO_RIG_HPP
#define CHOREO_RIG_HPP


#include <p3d/entity.hpp>

#include <choreo/buildconfig.hpp>


class tSkeleton;


namespace choreo
{


class ScriptReader;
class ScriptWriter;
class BaseBank;
class Rig;


//---------------------------------------------------------------------------
// Constants
//---------------------------------------------------------------------------

enum RigJointIndex
{
    // root joints
    RIG_JOINT_AI_ROOT = 0,
    RIG_JOINT_MOTION_ROOT,
    RIG_JOINT_ORIENTATION_ROOT,
    RIG_JOINT_BALANCE_ROOT,
    RIG_JOINT_CHARACTER_ROOT,

    RIG_JOINT_COUNT
};


static const float RIG_LEG_FOOT_PLANT_VALUE_DEFAULT      = 1.0f;
static const float RIG_LEG_FOOT_LIFT_VALUE_DEFAULT       = 0.0f;
static const float RIG_LEG_MIN_Y_RATE_DEFAULT            = 2.0f;
static const float RIG_LEG_Y_RATE_VELOCITY_SCALE_DEFAULT = 1.0f;

static const float RIG_BALANCE_ROOT_MIN_Y_RATE_DEFAULT            = 1.0f;
static const float RIG_BALANCE_ROOT_Y_RATE_VELOCITY_SCALE_DEFAULT = 0.5f;
static const float RIG_MAX_BALANCE_ROOT_COMPRESS_DEFAULT          = 0.0f;


//---------------------------------------------------------------------------
// class RigLeg
//---------------------------------------------------------------------------

class RigLeg: public tEntity
{
public:

    RigLeg();
    
    // leg index
    int GetIndex() const
        { return m_Index; }
    void SetIndex(int index)
        { m_Index = index; }
    
    // foot plant (extra attributes) joint
    int GetFootPlantJointIndex() const
        { return m_FootPlantJointIndex; }
    void SetFootPlantJointIndex(int footPlantJointIndex)
        { m_FootPlantJointIndex = footPlantJointIndex; }
    int GetFootPlantChannelIndex() const
        { return m_FootPlantChannelIndex; }
    void SetFootPlantChannelIndex(int footPlantChannelIndex)
        { m_FootPlantChannelIndex = footPlantChannelIndex; }
    float GetFootPlantValue() const
        { return m_FootPlantValue; }
    void SetFootPlantValue(float footPlantValue)
        { m_FootPlantValue = footPlantValue; }
    float GetFootLiftValue() const
        { return m_FootLiftValue; }
    void SetFootLiftValue(float footLiftValue)
        { m_FootLiftValue = footLiftValue; }
    
    // fk joint ranges -- for leg phase
    int GetMinimumFKJointIndex() const
        { return m_MinimumFKJointIndex; }
    void SetMinimumFKJointIndex(int minFKJointIndex)
        { m_MinimumFKJointIndex = minFKJointIndex; }
    int GetMaximumFKJointIndex() const
        { return m_MaximumFKJointIndex; }
    void SetMaximumFKJointIndex(int maxFKJointIndex)
        { m_MaximumFKJointIndex = maxFKJointIndex; }
    
    // fk joints (those that will be filled by ik, if enabled)
    // if no ik, only ankle joint is required
    int GetThighJointIndex() const
        { return m_ThighJointIndex; }
    void SetThighJointIndex(int thighJointIndex)
        { m_ThighJointIndex = thighJointIndex; }
    int GetKneeJointIndex() const
        { return m_KneeJointIndex; }
    void SetKneeJointIndex(int kneeJointIndex)
        { m_KneeJointIndex = kneeJointIndex; }
    int GetAnkleJointIndex() const
        { return m_AnkleJointIndex; }
    void SetAnkleJointIndex(int ankleJointIndex)
        { m_AnkleJointIndex = ankleJointIndex; }
    
    // ik joint ranges
    int GetMinimumIKJointIndex() const
        { return m_MinimumIKJointIndex; }
    void SetMinimumIKJointIndex(int minIKJointIndex)
        { m_MinimumIKJointIndex = minIKJointIndex; }
    int GetMaximumIKJointIndex() const
        { return m_MaximumIKJointIndex; }
    void SetMaximumIKJointIndex(int maxIKJointIndex)
        { m_MaximumIKJointIndex = maxIKJointIndex; }
    
    // ik joints -- orient offset is optional
    int GetEffectorJointIndex() const
        { return m_EffectorJointIndex; }
    void SetEffectorJointIndex(int effectorJointIndex)
        { m_EffectorJointIndex = effectorJointIndex; }
    int GetPoleVectorJointIndex() const
        { return m_PoleVectorJointIndex; }
    void SetPoleVectorJointIndex(int poleVectorJointIndex)
        { m_PoleVectorJointIndex = poleVectorJointIndex; }
    int GetOrientOffsetJointIndex() const
        { return m_OrientOffsetJointIndex; }
    void SetOrientOffsetJointIndex(int orientOffsetJointIndex)
        { m_OrientOffsetJointIndex = orientOffsetJointIndex; }
    
    // uses fk/ik ranges to detect whether joint belongs to this leg
    bool ContainsJointIndex(int jointIndex) const;
    
    // rate at which fixed-up foot changes in y
    float GetMinYRate() const
        { return m_MinYRate; }
    void SetMinYRate(float minYRate)
        { m_MinYRate = minYRate; }
    
    float GetYRateVelocityScale() const
        { return m_YRateVelocityScale; }
    void SetYRateVelocityScale(float yRateVelocityScale)
        { m_YRateVelocityScale = yRateVelocityScale; }
    
    bool IsIKRigged() const
        { return m_IsIKRigged; }
    void SetIsIKRigged(bool isIKRigged)
        { m_IsIKRigged = isIKRigged; }

    // script loading
    static bool ReadFromScript(ScriptReader* script, BaseBank* bank, Rig* rig);

#ifdef CHOREO_TOOL
    bool WriteToScript(ScriptWriter* script) const;
#endif // CHOREO_TOOL

protected:
    
    virtual ~RigLeg();
    
private:
    
    int m_Index;
    
    int m_FootPlantJointIndex;
    int m_FootPlantChannelIndex;
    float m_FootPlantValue;
    float m_FootLiftValue;
    
    int m_MinimumFKJointIndex;
    int m_MaximumFKJointIndex;
    
    int m_ThighJointIndex;
    int m_KneeJointIndex;
    int m_AnkleJointIndex;
    
    int m_MinimumIKJointIndex;
    int m_MaximumIKJointIndex;
    
    int m_EffectorJointIndex;
    int m_PoleVectorJointIndex;
    int m_OrientOffsetJointIndex;
    
    float m_MinYRate;
    float m_YRateVelocityScale;
    
    bool m_IsIKRigged;
};


//---------------------------------------------------------------------------
// class Rig
//---------------------------------------------------------------------------

class Rig: public tEntity
{
public:

    Rig(tSkeleton* skeleton,
        int maxLegCount);

    // skeleton
    tSkeleton* GetSkeleton() const
        { return m_Skeleton; }
    
    // joint information
    int GetJointCount() const;
    
    // joint mappings
    int GetJointIndex(RigJointIndex index) const;
    void SetJointIndex(RigJointIndex index, int jointIndex);
    
    // legs
    int GetLegCount() const
        { return m_LegCount; }
    RigLeg* GetLeg(int i) const;
    
    RigLeg* FindLeg(tUID legUID) const;
    RigLeg* FindLeg(const char* legName) const
        { return FindLeg(tEntity::MakeUID(legName)); }
    
    int FindLegIndex(tUID legUID) const;
    int FindLegIndex(const char* legName) const
        { return FindLegIndex(tEntity::MakeUID(legName)); }
    
    bool AddLeg(RigLeg* leg);
    
    // balance root
    float GetBalanceRootMinYRate() const
        { return m_BalanceRootMinYRate; }
    void SetBalanceRootMinYRate(float minYRate)
        { m_BalanceRootMinYRate = minYRate; }
    
    float GetBalanceRootYRateVelocityScale() const
        { return m_BalanceRootYRateVelocityScale; }
    void SetBalanceRootYRateVelocityScale(float yRateVelocityScale)
        { m_BalanceRootYRateVelocityScale = yRateVelocityScale; }
    
    float GetMaxBalanceRootCompress() const
        { return m_MaxBalanceRootCompress; }
    void SetMaxBalanceRootCompress(float maxCompress)
        { m_MaxBalanceRootCompress = maxCompress; }
    
    // script loading
    static bool ReadFromScript(ScriptReader* script, BaseBank* bank);

#ifdef CHOREO_TOOL
    static bool WriteToScriptHandler(ScriptWriter* script, BaseBank* bank);
    bool WriteToScript(ScriptWriter* script) const;
#endif // CHOREO_TOOL

protected:
    
    virtual ~Rig();
    
private:
    
    tSkeleton* m_Skeleton;
    
    int m_JointIndices[RIG_JOINT_COUNT];
    
    int m_MaxLegCount;
    int m_LegCount;
    RigLeg** m_Legs;
    
    float m_BalanceRootMinYRate;
    float m_BalanceRootYRateVelocityScale;
    float m_MaxBalanceRootCompress;
};


} // namespace choreo


#endif
