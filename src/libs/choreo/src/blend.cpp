/*
 * choreo/blend.cpp
 */

#include <choreo/blend.hpp>
#include <choreo/utility.hpp>
#include <choreo/rig.hpp>
#include <choreo/partition.hpp>
#include <choreo/load.hpp>
#include <choreo/basebank.hpp>
#include <choreo/jointblenddriver.hpp>
#include <choreo/footblenddriver.hpp>
#include <choreo/engine.hpp>
#include <choreo/jointblender.hpp>
#include <choreo/footblender.hpp>

#include <p3d/anim/skeleton.hpp>
#include <p3d/error.hpp>

#include <string.h>


namespace choreo
{


//---------------------------------------------------------------------------
// class BlendSlot
//---------------------------------------------------------------------------

BlendSlot::~BlendSlot()
{
}

float BlendSlot::GetCycleDuration() const
{
    float framesPerSecond = GetFramesPerSecond();

    if (framesPerSecond > 0.0f)
    {
        return (GetFrameCount() - 1.0f) / framesPerSecond;
    }
    else
    {
        return 0.0f;
    }
}

float BlendSlot::FrameToPhase(float frame) const
{
    float phase = (frame - GetCycleStart()) / (GetFrameCount() - 1.0f);
    return UnitWrap(phase);
}

float BlendSlot::PhaseToFrame(float phase) const
{
    float frameCountMinusOne = GetFrameCount() - 1.0f;
    float frame = ((phase * frameCountMinusOne) + GetCycleStart());
    return Wrap(0.0f, frameCountMinusOne, frame);
}


//---------------------------------------------------------------------------
// class BlendSlotFootInfo
//---------------------------------------------------------------------------

BlendSlotFootInfo::BlendSlotFootInfo(Rig* rig, BlendSlot* blendSlot)
{
    P3DASSERT(rig != 0);
    P3DASSERT(blendSlot != 0);

    m_FootCount = rig->GetLegCount();
    m_Feet = new FootCycle [m_FootCount];

    float frameCount = blendSlot->GetFrameCount();

    int f;
    for (f = 0; f < m_FootCount; ++f)
    {
        FootCycle* foot = (m_Feet + f);
        foot->plant = -1.0f;
        foot->lift = -1.0f;

        RigLeg* rigLeg = rig->GetLeg(f);

        int extraAttribIndex = rigLeg->GetFootPlantJointIndex();
        int channelIndex = rigLeg->GetFootPlantChannelIndex();

        if ((extraAttribIndex >= 0) &&
            (channelIndex >= 0) &&
            blendSlot->HasJointTranslation(extraAttribIndex))
        {
            float plantValue = rigLeg->GetFootPlantValue();
            float liftValue = rigLeg->GetFootLiftValue();

            rmt::Vector tmp;
            float info, prevInfo;

            blendSlot->GetJointTranslation(extraAttribIndex, frameCount - 2.0f, tmp);
            prevInfo = ((float*)&tmp)[channelIndex];

            for (int iFrame = 0; iFrame < (int)(frameCount - 1.0f); ++iFrame)
            {
                float frame = (float)iFrame;

                blendSlot->GetJointTranslation(extraAttribIndex, frame, tmp);
                info = ((float*)&tmp)[channelIndex];

                static const float FOOT_EPSILON = 0.01f;

                if (rmt::Epsilon(info, plantValue, FOOT_EPSILON) && !rmt::Epsilon(prevInfo, plantValue, FOOT_EPSILON))
                {
                    if (foot->plant < 0.0f)
                    {
                        foot->plant = frame;
                    }
                }
                else if (rmt::Epsilon(info, liftValue, FOOT_EPSILON) && !rmt::Epsilon(prevInfo, liftValue, FOOT_EPSILON))
                {
                    if (foot->lift < 0.0f)
                    {
                        foot->lift = frame;
                    }
                }

                prevInfo = info;
            }
        }

        // default to zero if we didn't get any foot info
        if (foot->plant < 0.0f)
        {
            foot->plant = 0.0f;
        }
        if (foot->lift < 0.0f)
        {
            foot->lift = blendSlot->GetFrameCount() - 2.0f;
        }
    }

	if (m_FootCount > 0)
	{
		m_CycleStart = m_Feet[0].plant;

		for (f = 1; f < m_FootCount; ++f)
		{
			float curCycleStart;
			if ((f % 2) == 1)
			{
				curCycleStart = m_Feet[f].lift;
			}
			else
			{
				curCycleStart = m_Feet[f].plant;
			}

			if (rmt::Fabs(m_CycleStart - curCycleStart) <=
				((frameCount - 1.0f) * 0.5f))
			{
				m_CycleStart = (m_CycleStart + curCycleStart) * 0.5f;
			}
			else
			{
				m_CycleStart = (m_CycleStart + curCycleStart + (frameCount - 1.0f)) * 0.5f;
			}
		}
	}
	else
	{
        // everybody else gets a cycle start of zero
        m_CycleStart = 0.0f;
	}

    for (f = 0; f < m_FootCount; ++f)
    {
        FootCycle* foot = (m_Feet + f);

        foot->plant = FrameToPhase(foot->plant, frameCount);
        foot->lift = FrameToPhase(foot->lift, frameCount);

        if ((f % 2) == 0)
        {
            if (foot->plant > 0.5f)
            {
                foot->plant -= 1.0f;
            }
        }
        else
        {
            if (foot->lift < 0.5f)
            {
                foot->lift += 1.0f;
            }
        }

        foot->scale = foot->lift - foot->plant;
    }
}

BlendSlotFootInfo::~BlendSlotFootInfo()
{
    delete[] m_Feet;
}

float BlendSlotFootInfo::FrameToPhase(float frame, float frameCount) const
{
    float phase = (frame - m_CycleStart) / (frameCount - 1.0f);
    return UnitWrap(phase);
}

float BlendSlotFootInfo::PhaseToFrame(float phase, float frameCount) const
{
    float frameCountMinusOne = frameCount - 1.0f;
    float frame = ((phase * frameCountMinusOne) + m_CycleStart);
    return Wrap(0.0f, frameCountMinusOne, frame);
}

float BlendSlotFootInfo::GetFootPlant(int foot) const
{
    P3DASSERT((foot >= 0) && (foot < m_FootCount));
    if ((foot < 0) || (foot >= m_FootCount))
        return -1.0f;
    return m_Feet[foot].plant;
}

float BlendSlotFootInfo::GetFootLift(int foot) const
{
    P3DASSERT((foot >= 0) && (foot < m_FootCount));
    if ((foot < 0) || (foot >= m_FootCount))
        return -1.0f;
    return m_Feet[foot].lift;
}

float BlendSlotFootInfo::GetFootScale(int foot) const
{
    P3DASSERT((foot >= 0) && (foot < m_FootCount));
    if ((foot < 0) || (foot >= m_FootCount))
        return -1.0f;
    return m_Feet[foot].scale;
}


//---------------------------------------------------------------------------
// class Blend
//   - external class
//---------------------------------------------------------------------------

Blend::Blend(Rig* rig,
             int paramCount,
             const int* paramSlotCounts,
             Partition* partition):
    m_ParamCount(paramCount),

    m_Weight(1.0f),
    m_Priority(0),

    m_RootBlendInTime(0.0f),
    m_RootBlendOutTime(0.0f),
    m_BlendInTime(0.0f),
    m_BlendOutTime(0.0f)
{
    P3DASSERT(rig != 0);
    m_Rig = rig;
    m_Rig->AddRef();

    int i;

    m_ParamSlotCounts = new int [m_ParamCount];
    m_BlendSlotCount = 1;

    for (i = 0; i < m_ParamCount; ++i)
    {
        P3DASSERT(paramSlotCounts[i] > 0);
        m_ParamSlotCounts[i] = paramSlotCounts[i];
        m_BlendSlotCount *= paramSlotCounts[i];
    }

    m_BlendSlots = new BlendSlot* [m_BlendSlotCount];
    memset(m_BlendSlots, 0, sizeof(BlendSlot*) * m_BlendSlotCount);

    m_UniqueBlendSlotCount = 0;
    m_UniqueBlendSlots = new BlendSlot* [m_BlendSlotCount];

    if (partition != 0)
    {
        m_Partition = partition;
        m_Partition->AddRef();
    }
    else
    {
        m_Partition = 0;
    }
}

Blend::~Blend()
{
    tRefCounted::Release(m_Partition);

    int i;

    for (i = (m_UniqueBlendSlotCount - 1); i >= 0; --i)
    {
        tRefCounted::Release(m_UniqueBlendSlots[i]);
    }
    delete[] m_UniqueBlendSlots;

    for (i = (m_BlendSlotCount - 1); i >= 0; --i)
    {
        tRefCounted::Release(m_BlendSlots[i]);
    }
    delete[] m_BlendSlots;

    delete[] m_ParamSlotCounts;

    m_Rig->Release();
}

int Blend::GetParamSlotCount(int i) const
{
    P3DASSERT((i >= 0) && (i < m_ParamCount));
    if ((i < 0) || (i >= m_ParamCount))
        return -1;
    return m_ParamSlotCounts[i];
}

int Blend::GetBlendSlotIndex(const int* params) const
{
    int index = 0;

    for (int i = 0; i < m_ParamCount; ++i)
    {
        int curIndex = params[i];

        // clamp indices
        if (curIndex < 0)
        {
            curIndex = 0;
        }
        else if (curIndex >= m_ParamSlotCounts[i])
        {
            curIndex = m_ParamSlotCounts[i] - 1;
        }

        for (int j = (i + 1); j < m_ParamCount; ++j)
        {
            curIndex *= m_ParamSlotCounts[j];
        }

        index += curIndex;
    }

    return index;
}

const BlendSlot* Blend::GetBlendSlot(int index) const
{
    P3DASSERT((index >= 0) && (index < m_BlendSlotCount));
    if ((index < 0) || (index >= m_BlendSlotCount))
        return 0;

    return m_BlendSlots[index];
}

BlendSlot* Blend::GetBlendSlot(int index)
{
    P3DASSERT((index >= 0) && (index < m_BlendSlotCount));
    if ((index < 0) || (index >= m_BlendSlotCount))
        return 0;

    return m_BlendSlots[index];
}

bool Blend::SetBlendSlot(int index, BlendSlot* blendSlot)
{
    P3DASSERT((index >= 0) && (index < m_BlendSlotCount));
    if ((index < 0) || (index >= m_BlendSlotCount))
        return false;

    tRefCounted::Assign(m_BlendSlots[index], blendSlot);

    if (blendSlot != 0)
    {
        for (int i = 0; i < m_UniqueBlendSlotCount; ++i)
        {
            if (blendSlot == m_UniqueBlendSlots[i])
                return true;
        }

        m_UniqueBlendSlots[m_UniqueBlendSlotCount] = blendSlot;
        m_UniqueBlendSlots[m_UniqueBlendSlotCount]->AddRef();
        ++m_UniqueBlendSlotCount;
    }

    return true;
}

void Blend::SetPartition(Partition* partition)
{
    tRefCounted::Assign(m_Partition, partition);
}

PuppetDriver* Blend::NewDriver()
{
    return NewBlendDriver();
}

BlendDriver* Blend::NewBlendDriver()
{
    return new BlendDriver(this);
}

bool Blend::ReadFromScript(ScriptReader* script, BaseBank* bank)
{
    // TODO
    return true;
}

#ifdef CHOREO_TOOL
bool Blend::WriteToScriptHandler(ScriptWriter* script, BaseBank* bank)
{
    CHOREO_SCRIPT_WRITE_HANDLER(Blend, script, bank);
}

bool Blend::WriteToScript(ScriptWriter* script) const
{
    // TODO
    return true;
}
#endif // CHOREO_TOOL

    

//---------------------------------------------------------------------------
// class BlendPhase
//   - internal class
//---------------------------------------------------------------------------

class BlendPhase: public tRefCounted
{
public:
    
    BlendPhase(Blend* blend);

    Blend* GetBlend()
        { return m_Blend; }
    
    // blending params
    Partition* GetPartition() const
        { return m_Partition; }
    void SetPartition(Partition* partition)
        { tRefCounted::Assign(m_Partition, partition); }

    float GetWeight() const
        { return m_Weight; }
    void SetWeight(float weight)
        { m_Weight = weight; }
    
    int GetPriority() const
        { return m_Priority; }
    void SetPriority(int priority)
        { m_Priority = priority; }
    
    // blend times -- overrides those in Idle
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

    // param accessors
    float GetParamValue(int paramIndex) const;
    bool SetParamValue(int paramIndex, float paramValue);

    // cyclic accessors
    bool IsCyclic() const
        { return m_IsCyclic; }
    void SetIsCyclic(bool isCyclic)
        { m_IsCyclic = isCyclic; }

    // phase accessors
    float GetPhase() const
        { return m_Phase; }
    void SetPhase(float phase);

    // foot accessors
    float GetFootPlant(int foot) const;
    float GetFootLift(int foot) const;
    float GetFootScale(int foot) const;
    float GetFootPhase(int foot) const;
    bool IsFootPlanted(int foot) const;

    // blended animation accessors
    int GetMaxBlendSlotCount() const;
    int GetBlendSlotCount() const
        { return m_BlendSlotCount; }
    BlendSlot* GetBlendSlot(int i);

    float GetCycleDuration() const
        { return m_CycleDuration; }

    // joint channel accessors
    bool HasJointTranslation(int jointIndex) const;
    void GetJointTranslation(int jointIndex, rmt::Vector& translate) const;
    bool HasJointRotation(int jointIndex) const;
    void GetJointRotation(int jointIndex, rmt::Quaternion& rotate) const;

    float GetJointWeight(int jointIndex) const;
    int GetJointPriority(int jointIndex) const;

    float GetBlendSlotLegWeight(int i, int legIndex) const;
    float GetBlendSlotLegFrame(int i, int legIndex) const;

    // group channel accessors
    bool HasGroupTranslation(tUID group) const;
    void GetGroupTranslation(tUID group, rmt::Vector& translate) const;
    bool HasGroupRotation(tUID group) const;
    void GetGroupRotation(tUID group, rmt::Quaternion& rotate) const;

    float GetGroupWeight() const;
    int GetGroupPriority() const;

    // LOD accessors
    float GetBlendThreshold() const
        { return m_BlendThreshold; }
    void SetBlendThreshold(float blendThreshold)
        { m_BlendThreshold = blendThreshold; }

    int GetMaxBlendedSlotCount() const
        { return m_MaxBlendedSlotCount; }
    void SetMaxBlendedSlotCount(int maxSlotCount)
        { m_MaxBlendedSlotCount = maxSlotCount; }

    // update calls
    void UpdateBlends();
    void AdvancePhase(float deltaTime);
    void UpdateLegPhase();

    bool IsFinished() const;

protected:

    virtual ~BlendPhase();

private:

    bool AddBlendSlot(BlendSlot* blendSlot, float weight);

    Blend* m_Blend;

    Partition* m_Partition;
    float m_Weight;
    int m_Priority;

    float m_RootBlendInTime;
    float m_RootBlendOutTime;
    float m_BlendInTime;
    float m_BlendOutTime;

    float* m_ParamValues;
    int* m_ParamIndices;
    
    bool m_IsCyclic;
    float m_Phase;
    float* m_Frames;
    
    struct FootStruct
    {
        float plant;
        float lift;
        float scale;
        float phase;
        bool isPlanted;
        float* slotFrames;
        float* slotWeights;
    };
    FootStruct* m_Feet;

    int m_BlendSlotCount;
    BlendSlot** m_BlendSlots;
    float* m_BlendSlotWeights;

    float m_CycleDuration;

    float** m_JointFrames;
    float** m_JointWeights;

    float m_BlendThreshold;
    int m_MaxBlendedSlotCount;
};


//---------------------------------------------------------------------------
// class BlendPhase - implementation
//---------------------------------------------------------------------------

BlendPhase::BlendPhase(Blend* blend):
    m_Weight(1.0f),
    m_Priority(0),

    m_IsCyclic(false),
    m_Phase(0.0f),

    m_BlendSlotCount(0),

    m_CycleDuration(0.0f),

    m_BlendThreshold(0.0f)
{
    P3DASSERT(blend != 0);
    m_Blend = blend;
    m_Blend->AddRef();
    
    m_Partition = m_Blend->GetPartition();
    if (m_Partition != 0)
    {
        m_Partition->AddRef();
    }

    m_RootBlendInTime = m_Blend->GetRootBlendInTime();
    m_RootBlendOutTime = m_Blend->GetRootBlendOutTime();

    m_BlendInTime = m_Blend->GetBlendInTime();
    m_BlendOutTime = m_Blend->GetBlendOutTime();

    int i;

    // init param lists
    int paramCount = m_Blend->GetParamCount();
    m_ParamValues = new float [paramCount];
    memset(m_ParamValues, 0, sizeof(float) * paramCount);
    m_ParamIndices = new int [paramCount];

    int maxBlendSlotCount = GetMaxBlendSlotCount();

    // init foot struct
    Rig* rig = m_Blend->GetRig();
    int legCount = rig->GetLegCount();
    m_Feet = new FootStruct [legCount];

    for (i = 0; i < legCount; ++i)
    {
        m_Feet[i].plant = 0.0f;
        m_Feet[i].lift = 0.0f;
        m_Feet[i].scale = 0.0f;
        m_Feet[i].phase = 0.0f;
        m_Feet[i].isPlanted = false;

        m_Feet[i].slotFrames = new float [maxBlendSlotCount];
        m_Feet[i].slotWeights = new float [maxBlendSlotCount];

        for (int j = 0; j < maxBlendSlotCount; ++j)
        {
            m_Feet[i].slotFrames[j] = 0.0f;
            m_Feet[i].slotWeights[j] = 0.0f;
        }
    }

    // init blend slot lists
    m_Frames = new float [maxBlendSlotCount];
    m_BlendSlots = new BlendSlot* [maxBlendSlotCount];
    m_BlendSlotWeights = new float [maxBlendSlotCount];

    for (i = 0; i < maxBlendSlotCount; ++i)
    {
        m_Frames[i] = 0.0f;
        m_BlendSlots[i] = 0;
        m_BlendSlotWeights[i] = 0.0f;
    }

    // init joint maps
    int jointCount = rig->GetJointCount();

    m_JointFrames = new float* [jointCount];
    m_JointWeights = new float* [jointCount];

    for (i = 0; i < jointCount; ++i)
    {
        bool assigned = false;

        for (int j = 0; j < legCount; ++j)
        {
            RigLeg* rigLeg = rig->GetLeg(j);

            if (rigLeg->ContainsJointIndex(i))
            {
                m_JointFrames[i] = m_Feet[j].slotFrames;
                m_JointWeights[i] = m_Feet[j].slotWeights;
                assigned = true;
                break;
            }
        }

        if (!assigned)
        {
            m_JointFrames[i] = m_Frames;
            m_JointWeights[i] = m_BlendSlotWeights;
        }
    }

    m_MaxBlendedSlotCount = maxBlendSlotCount;
}

BlendPhase::~BlendPhase()
{
    int maxBlendSlotCount = GetMaxBlendSlotCount();

    int i;
    for (i = (maxBlendSlotCount - 1); i >= 0; --i)
    {
        tRefCounted::Release(m_BlendSlots[i]);
    }

    delete[] m_JointWeights;
    delete[] m_JointFrames;

    delete[] m_BlendSlotWeights;
    delete[] m_BlendSlots;
    delete[] m_Frames;

    int legCount = m_Blend->GetRig()->GetLegCount();
    for (i = (legCount - 1); i >= 0; --i)
    {
        delete[] m_Feet[i].slotWeights;
        delete[] m_Feet[i].slotFrames;
    }
    delete[] m_Feet;

    delete[] m_ParamIndices;
    delete[] m_ParamValues;

    tRefCounted::Release(m_Partition);
    
    m_Blend->Release();
}

float BlendPhase::GetParamValue(int paramIndex) const
{
    int paramCount = m_Blend->GetParamCount();
    P3DASSERT((paramIndex >= 0) && (paramIndex < paramCount));
    if ((paramIndex < 0) || (paramIndex >= paramCount))
        return -1.0f;

    return m_ParamValues[paramIndex];
}

bool BlendPhase::SetParamValue(int paramIndex, float paramValue)
{
    int paramCount = m_Blend->GetParamCount();
    P3DASSERT((paramIndex >= 0) && (paramIndex < paramCount));
    if ((paramIndex < 0) || (paramIndex >= paramCount))
        return false;

    m_ParamValues[paramIndex] = paramValue;
    return true;
}

void BlendPhase::SetPhase(float phase)
{
    m_Phase = phase;

    if (m_IsCyclic)
    {
        m_Phase = UnitWrap(m_Phase);
    }
    else
    {
        if (m_Phase < 0.0f)
        {
            m_Phase = 0.0f;
        }
        else if (m_Phase > 1.0f)
        {
            m_Phase = 1.0f;
        }
    }

    for (int i = 0; i < m_BlendSlotCount; ++i)
    {
        m_Frames[i] = m_BlendSlots[i]->PhaseToFrame(m_Phase);
    }
}

int BlendPhase::GetMaxBlendSlotCount() const
{
    int paramCount = m_Blend->GetParamCount();
    return (1 << paramCount);
}

BlendSlot* BlendPhase::GetBlendSlot(int i)
{
    int blendSlotCount = GetBlendSlotCount();
    P3DASSERT((i >= 0) && (i < blendSlotCount));
    if ((i < 0) || (i >= blendSlotCount))
        return 0;

    return m_BlendSlots[i];
}

bool BlendPhase::AddBlendSlot(BlendSlot* blendSlot, float weight)
{
    // see if slot already added
    int insertIndex = -1;
    for (int i = 0; i < m_BlendSlotCount; ++i)
    {
        if (blendSlot == m_BlendSlots[i])
        {
            // if so, increment weight
            m_BlendSlotWeights[i] += weight;

            // move slot up list according to weight increase
            while ((i > 0) && (m_BlendSlotWeights[i - 1] < m_BlendSlotWeights[i]))
            {
                float tmpWeight = m_BlendSlotWeights[i];
                m_BlendSlotWeights[i] = m_BlendSlotWeights[i - 1];
                m_BlendSlotWeights[i - 1] = tmpWeight;

                m_BlendSlots[i] = m_BlendSlots[i - 1];
                m_BlendSlots[i - 1] = blendSlot;

                --i;
            }

            return true;
        }
        else if ((insertIndex < 0) && (m_BlendSlotWeights[i] < weight))
        {
            insertIndex = i;
        }
    }

    if (insertIndex < 0)
    {
        insertIndex = m_BlendSlotCount;
    }

    // check to see if we have room (we should!)
    int maxBlendSlotCount = GetMaxBlendSlotCount();
    P3DASSERT(m_BlendSlotCount < maxBlendSlotCount);
    if (m_BlendSlotCount >= maxBlendSlotCount)
        return false;

    // add slot
    memmove(m_BlendSlots + insertIndex + 1,
            m_BlendSlots + insertIndex,
            (m_BlendSlotCount - insertIndex) * sizeof(m_BlendSlots[0]));
    memmove(m_BlendSlotWeights + insertIndex + 1,
            m_BlendSlotWeights + insertIndex,
            (m_BlendSlotCount - insertIndex) * sizeof(m_BlendSlotWeights[0]));
    m_BlendSlots[insertIndex] = blendSlot;
    m_BlendSlots[insertIndex]->AddRef();
    m_BlendSlotWeights[insertIndex] = weight;
    ++m_BlendSlotCount;

    return true;
}

void BlendPhase::UpdateBlends()
{
    int i;

    // calculate param indices from values
    // these param indices are the base blend slot indices to be blended
    int paramCount = m_Blend->GetParamCount();
    for (i = 0; i < paramCount; ++i)
    {
        int paramSlotCount = m_Blend->GetParamSlotCount(i);
        m_ParamIndices[i] = (int)m_ParamValues[i];
    }

    // init blend slots
    for (i = (m_BlendSlotCount - 1); i >= 0; --i)
    {
        tRefCounted::Release(m_BlendSlots[i]);
    }
    m_BlendSlotCount = 0;
    float totalWeight = 0.0f;

    // fill each blend slot
    // each slot will use the base index or the base index plus one of each param
    int maxBlendSlotCount = GetMaxBlendSlotCount();
    for (i = 0; i < maxBlendSlotCount; ++i)
    {
        float weight = 1.0f;

        int j;
        for (j = 0; j < paramCount; ++j)
        {
            int paramSlotCount = m_Blend->GetParamSlotCount(j);

            // offset param index & calculate base weight
            if ((i & (1 << j)) != 0)
            {
                weight *= (m_ParamValues[j] - m_ParamIndices[j]);
                ++(m_ParamIndices[j]);
            }
            else
            {
                weight *= (1.0f - (m_ParamValues[j] - m_ParamIndices[j]));
            }
        }

        // add slot
        int slotIndex = m_Blend->GetBlendSlotIndex(m_ParamIndices);
        BlendSlot* blendSlot = m_Blend->GetBlendSlot(slotIndex);

        if (blendSlot != 0)
        {
            // only add if we exceed LOD threshold
            if (weight > m_BlendThreshold)
            {
                AddBlendSlot(blendSlot, weight);
                totalWeight += weight;
            }
        }

        // revert param index if incremented
        for (j = 0; j < paramCount; ++j)
        {
            if ((i & (1 << j)) != 0)
            {
                --(m_ParamIndices[j]);
            }
        }
    }

    // max blended slot count -- ditch low-weight slots
    if (m_MaxBlendedSlotCount < m_BlendSlotCount)
    {
        m_BlendSlotCount = m_MaxBlendedSlotCount;

        totalWeight = 0.0f;
        for (i = 0; i < m_BlendSlotCount; ++i)
        {
            totalWeight += m_BlendSlotWeights[i];
        }
    }

    // redistribute weights
    // calculate blended cycle duration
    m_CycleDuration = 0.0f;
    for (i = 0; i < m_BlendSlotCount; ++i)
    {
        if (totalWeight > 0.0f)
        {
            m_BlendSlotWeights[i] = (m_BlendSlotWeights[i] / totalWeight);
        }
        else
        {
            m_BlendSlotWeights[i] = 0.0f;
        }

        float slotDuration = m_BlendSlots[i]->GetCycleDuration();
        m_CycleDuration += (slotDuration * m_BlendSlotWeights[i]);
    }
}

void BlendPhase::AdvancePhase(float deltaTime)
{
    float unitDelta;
    if (m_CycleDuration > 0.0f)
    {
        unitDelta = deltaTime / m_CycleDuration;
    }
    else
    {
        unitDelta = 0.0f;
    }

    SetPhase(GetPhase() + unitDelta);
}

void BlendPhase::UpdateLegPhase()
{
    int i;
    int legCount = m_Blend->GetRig()->GetLegCount();
    for (i = 0; i < legCount; ++i)
    {
        FootStruct* foot = (m_Feet + i);

        // calculate lift & plant
        float footLift = 0.0f;
        float footPlant = 0.0f;

        for (int j = 0; j < m_BlendSlotCount; ++j)
        {
            BlendSlot* blendSlot = m_BlendSlots[j];
            float blendSlotWeight = m_BlendSlotWeights[j];

            footLift += (blendSlot->GetFootLift(i) * blendSlotWeight);
            footPlant += (blendSlot->GetFootPlant(i) * blendSlotWeight);
        }

        // calculate foot scale
        float footScale = footLift - footPlant;
        foot->scale = footScale;

        // readjust feet info
        footPlant = (footPlant + 1.0f) - rmt::Floor(footPlant + 1.0f);
        foot->plant = footPlant;
        footLift = (footLift + 1.0f) - rmt::Floor(footLift + 1.0f);
        foot->lift = footLift;

        // calculate foot state
        if (((footLift > footPlant) && (GetPhase() >= footPlant) && (GetPhase() < footLift)) ||
            ((footLift < footPlant) && ((GetPhase() >= footPlant) || (GetPhase() < footLift))))
        {
            // foot is planted
            foot->isPlanted = true;

            // calculate foot phase within plant
            float footPhase;
            if (!rmt::Epsilon(footScale, 0.0f))
            {
                footPhase = (GetPhase() - footPlant);
                footPhase = ((footPhase + 1.0f) - rmt::Floor(footPhase + 1.0f)) / footScale;
            }
            else
            {
                footPhase = GetPhase();
            }

            foot->phase = footPhase;
        }
        else
        {
            // foot is not planted
            foot->isPlanted = false;

            // calculate foot phase within lift
            float footPhase;
            float invFootScale = 1.0f - footScale;
            if (!rmt::Epsilon(invFootScale, 0.0f))
            {
                footPhase = (GetPhase() - footLift);
                footPhase = ((footPhase + 1.0f) - rmt::Floor(footPhase + 1.0f)) / invFootScale;
            }
            else
            {
                footPhase = GetPhase();
            }

            foot->phase = footPhase;
        }
    }

    // calculate actual blended animation leg phases
    for (i = 0; i < m_BlendSlotCount; ++i)
    {
        BlendSlot* blendSlot = m_BlendSlots[i];

        float blendSlotCycleDuration = blendSlot->GetCycleDuration();
        if (blendSlotCycleDuration > 0.0f)
        {
            float scaleFactor = m_CycleDuration / blendSlotCycleDuration;
            float adjustedWeight = m_BlendSlotWeights[i] * scaleFactor;
            m_BlendSlotWeights[i] = adjustedWeight;
        }

        for (int f = 0; f < legCount; ++f)
        {
            FootStruct* foot = (m_Feet + f);

            float scale;

            if ((blendSlot->GetFootScale(f) >= 1.0f) ||
                (blendSlot->GetFootScale(f) <= 0.0f))
            {
                foot->slotFrames[i] = blendSlot->PhaseToFrame(GetPhase());
                scale = 1.0f;
            }
            else
            {
                if (foot->isPlanted)
                {
                    float footPhase = blendSlot->GetFootPlant(f) +
                        (foot->phase * blendSlot->GetFootScale(f));
                    foot->slotFrames[i] = blendSlot->PhaseToFrame(footPhase);
                }
                else
                {
                    float footPhase = blendSlot->GetFootLift(f) +
                        (foot->phase * (1.0f - blendSlot->GetFootScale(f)));
                    foot->slotFrames[i] = blendSlot->PhaseToFrame(footPhase);
                }

                scale = foot->scale / blendSlot->GetFootScale(f);

                // FIXME -- should scale ever be < 0?
                if (scale < 0.0f)
                {
                    scale = 1.0f;
                }
            }

            foot->slotWeights[i] = scale * m_BlendSlotWeights[i];
        }
    }
}

bool BlendPhase::IsFinished() const
{
    if (m_IsCyclic)
        return false;
    return (m_Phase >= 1.0f);
}

float BlendPhase::GetFootPlant(int foot) const
{
    int legCount = m_Blend->GetRig()->GetLegCount();
    P3DASSERT((foot >= 0) && (foot < legCount));
    if ((foot < 0) || (foot >= legCount))
        return 0.0f;

    return m_Feet[foot].plant;
}

float BlendPhase::GetFootLift(int foot) const
{
    int legCount = m_Blend->GetRig()->GetLegCount();
    P3DASSERT((foot >= 0) && (foot < legCount));
    if ((foot < 0) || (foot >= legCount))
        return 0.0f;

    return m_Feet[foot].lift;
}

float BlendPhase::GetFootScale(int foot) const
{
    int legCount = m_Blend->GetRig()->GetLegCount();
    P3DASSERT((foot >= 0) && (foot < legCount));
    if ((foot < 0) || (foot >= legCount))
        return 0.0f;

    return m_Feet[foot].scale;
}

float BlendPhase::GetFootPhase(int foot) const
{
    int legCount = m_Blend->GetRig()->GetLegCount();
    P3DASSERT((foot >= 0) && (foot < legCount));
    if ((foot < 0) || (foot >= legCount))
        return 0.0f;

    return m_Feet[foot].phase;
}

bool BlendPhase::IsFootPlanted(int foot) const
{
    int legCount = m_Blend->GetRig()->GetLegCount();
    P3DASSERT((foot >= 0) && (foot < legCount));
    if ((foot < 0) || (foot >= legCount))
        return false;

    return m_Feet[foot].isPlanted;
}

bool BlendPhase::HasJointTranslation(int jointIndex) const
{
    if (m_BlendSlotCount <= 0)
        return false;

    if (m_Partition != 0)
    {
        if (!m_Partition->HasJoint(jointIndex))
            return false;
    }

    for (int i = 0; i < m_BlendSlotCount; ++i)
    {
        if (m_BlendSlots[i]->HasJointTranslation(jointIndex))
            return true;
    }

    return false;
}

void BlendPhase::GetJointTranslation(int jointIndex, rmt::Vector& translation) const
{
    rmt::Vector tmpTranslate, newTranslate;
    float totalWeight = 0.0f;

    int i;
    for (i = 0; i < m_BlendSlotCount; ++i)
    {
        BlendSlot* blendSlot = m_BlendSlots[i];
        if (!blendSlot->HasJointTranslation(jointIndex))
            continue;

        totalWeight = m_JointWeights[jointIndex][i];

        float frame = m_JointFrames[jointIndex][i];
        blendSlot->GetJointTranslation(jointIndex, frame, translation);
        break;
    }

    for (++i; i < m_BlendSlotCount; ++i)
    {
        BlendSlot* blendSlot = m_BlendSlots[i];
        if (!blendSlot->HasJointTranslation(jointIndex))
            continue;

        float weight = m_JointWeights[jointIndex][i];
        totalWeight += weight;
        float alpha = weight / totalWeight;

        float frame = m_JointFrames[jointIndex][i];
        blendSlot->GetJointTranslation(jointIndex, frame, tmpTranslate);

        BlendVector(translation, tmpTranslate, alpha, newTranslate);
        translation = newTranslate;
    }
}

bool BlendPhase::HasJointRotation(int jointIndex) const
{
    if (m_BlendSlotCount <= 0)
        return false;

    if (m_Partition != 0)
    {
        if (!m_Partition->HasJoint(jointIndex))
            return false;
    }

    for (int i = 0; i < m_BlendSlotCount; ++i)
    {
        if (m_BlendSlots[i]->HasJointRotation(jointIndex))
            return true;
    }

    return false;
}

void BlendPhase::GetJointRotation(int jointIndex, rmt::Quaternion& rotation) const
{
    rmt::Quaternion tmpRotate, newRotate;
    float totalWeight = 0.0f;

    int i;
    for (i = 0; i < m_BlendSlotCount; ++i)
    {
        BlendSlot* blendSlot = m_BlendSlots[i];
        if (!blendSlot->HasJointRotation(jointIndex))
            continue;

        totalWeight = m_JointWeights[jointIndex][i];

        float frame = m_JointFrames[jointIndex][i];
        blendSlot->GetJointRotation(jointIndex, frame, rotation);
        break;
    }

    for (++i; i < m_BlendSlotCount; ++i)
    {
        BlendSlot* blendSlot = m_BlendSlots[i];
        if (!blendSlot->HasJointRotation(jointIndex))
            continue;

        float weight = m_JointWeights[jointIndex][i];
        totalWeight += weight;
        float alpha = weight / totalWeight;

        float frame = m_JointFrames[jointIndex][i];
        blendSlot->GetJointRotation(jointIndex, frame, tmpRotate);

        BlendQuaternion(rotation, tmpRotate, alpha, newRotate);
        rotation = newRotate;
    }
}

float BlendPhase::GetJointWeight(int jointIndex) const
{
    float weight = GetWeight();
    weight *= m_Blend->GetWeight();

    if (m_Partition != 0)
    {
        weight *= m_Partition->GetJointWeight(jointIndex);
    }

    return weight;
}

int BlendPhase::GetJointPriority(int jointIndex) const
{
    int priority = GetPriority();
    priority += m_Blend->GetPriority();

    if (m_Partition != 0)
    {
        priority += m_Partition->GetJointPriority(jointIndex);
    }

    return priority;
}

bool BlendPhase::HasGroupTranslation(tUID group) const
{
    if (m_BlendSlotCount <= 0)
        return false;

    for (int i = 0; i < m_BlendSlotCount; ++i)
    {
        if (m_BlendSlots[i]->HasGroupTranslation(group))
            return true;
    }

    return false;
}

void BlendPhase::GetGroupTranslation(tUID group, rmt::Vector& translation) const
{
    rmt::Vector tmpTranslate, newTranslate;
    float totalWeight = 0.0f;

    int i;
    for (i = 0; i < m_BlendSlotCount; ++i)
    {
        BlendSlot* blendSlot = m_BlendSlots[i];
        if (!blendSlot->HasGroupTranslation(group))
            continue;

        totalWeight = m_BlendSlotWeights[i];

        float frame = m_Frames[i];
        blendSlot->GetGroupTranslation(group, frame, translation);
        break;
    }

    for (++i; i < m_BlendSlotCount; ++i)
    {
        BlendSlot* blendSlot = m_BlendSlots[i];
        if (!blendSlot->HasGroupTranslation(group))
            continue;

        float weight = m_BlendSlotWeights[i];
        totalWeight += weight;
        float alpha = weight / totalWeight;

        float frame = m_Frames[i];
        blendSlot->GetGroupTranslation(group, frame, tmpTranslate);

        BlendVector(translation, tmpTranslate, alpha, newTranslate);
        translation = newTranslate;
    }
}

bool BlendPhase::HasGroupRotation(tUID group) const
{
    if (m_BlendSlotCount <= 0)
        return false;

    for (int i = 0; i < m_BlendSlotCount; ++i)
    {
        if (m_BlendSlots[i]->HasGroupRotation(group))
            return true;
    }

    return false;
}

void BlendPhase::GetGroupRotation(tUID group, rmt::Quaternion& rotation) const
{
    rmt::Quaternion tmpRotate, newRotate;
    float totalWeight = 0.0f;

    int i;
    for (i = 0; i < m_BlendSlotCount; ++i)
    {
        BlendSlot* blendSlot = m_BlendSlots[i];
        if (!blendSlot->HasGroupRotation(group))
            continue;

        totalWeight = m_BlendSlotWeights[i];

        float frame = m_Frames[i];
        blendSlot->GetGroupRotation(group, frame, rotation);
        break;
    }

    for (++i; i < m_BlendSlotCount; ++i)
    {
        BlendSlot* blendSlot = m_BlendSlots[i];
        if (!blendSlot->HasGroupRotation(group))
            continue;

        float weight = m_BlendSlotWeights[i];
        totalWeight += weight;
        float alpha = weight / totalWeight;

        float frame = m_Frames[i];
        blendSlot->GetGroupRotation(group, frame, tmpRotate);

        BlendQuaternion(rotation, tmpRotate, alpha, newRotate);
        rotation = newRotate;
    }
}

float BlendPhase::GetGroupWeight() const
{
    float weight = GetWeight();
    weight *= m_Blend->GetWeight();

    return weight;
}

int BlendPhase::GetGroupPriority() const
{
    int priority = GetPriority();
    priority += m_Blend->GetPriority();

    return priority;
}

float BlendPhase::GetBlendSlotLegWeight(int i, int legIndex) const
{
    P3DASSERT((i >= 0) && (i < m_BlendSlotCount));
    if ((i < 0) || (i >= m_BlendSlotCount))
        return 0.0f;

    int legCount = m_Blend->GetRig()->GetLegCount();
    P3DASSERT((legIndex >= 0) && (legIndex < legCount));
    if ((legIndex < 0) || (legIndex >= legCount))
        return 0.0f;

    return m_Feet[legIndex].slotWeights[i];
}

float BlendPhase::GetBlendSlotLegFrame(int i, int legIndex) const
{
    P3DASSERT((i >= 0) && (i < m_BlendSlotCount));
    if ((i < 0) || (i >= m_BlendSlotCount))
        return 0.0f;

    int legCount = m_Blend->GetRig()->GetLegCount();
    P3DASSERT((legIndex >= 0) && (legIndex < legCount));
    if ((legIndex < 0) || (legIndex >= legCount))
        return 0.0f;

    return m_Feet[legIndex].slotFrames[i];
}


//---------------------------------------------------------------------------
// class BlendJointDriver - interface
//   - internal class
//---------------------------------------------------------------------------

class BlendJointDriver: public JointBlendDriver
{
public:

    BlendJointDriver(BlendPhase* blendPhase);

    virtual void Reset();
    virtual void Advance(float dt);

    // blend times
    virtual float GetBlendInTime() const;
    virtual float GetBlendOutTime() const;

    // weight & priority
    virtual float GetJointWeight(int jointIndex) const;
    virtual int GetJointPriority(int jointIndex) const;

    // translation
    virtual bool HasJointTranslation(int jointIndex) const;
    virtual void GetJointTranslation(int jointIndex, rmt::Vector& translation) const;

    // rotation
    virtual bool HasJointRotation(int jointIndex) const;
    virtual void GetJointRotation(int jointIndex, rmt::Quaternion& rotation) const;

    // weight & priority
    virtual float GetGroupWeight(tUID group) const;
    virtual int GetGroupPriority(tUID group) const;

    // translation
    virtual bool HasGroupTranslation(tUID group) const;
    virtual void GetGroupTranslation(tUID group, rmt::Vector& translation) const;

    // rotation
    virtual bool HasGroupRotation(tUID group) const;
    virtual void GetGroupRotation(tUID group, rmt::Quaternion& rotation) const;

protected:

    virtual ~BlendJointDriver();

private:

    BlendPhase* m_BlendPhase;
};


//---------------------------------------------------------------------------
// class BlendJointDriver - implementation
//---------------------------------------------------------------------------

BlendJointDriver::BlendJointDriver(BlendPhase* blendPhase)
{
    P3DASSERT(blendPhase != 0);
    m_BlendPhase = blendPhase;
    m_BlendPhase->AddRef();
}

BlendJointDriver::~BlendJointDriver()
{
    m_BlendPhase->Release();
}

void BlendJointDriver::Reset()
{
    // do nothing
}

void BlendJointDriver::Advance(float dt)
{
    // do nothing
}

float BlendJointDriver::GetBlendInTime() const
{
    return m_BlendPhase->GetBlendInTime();
}

float BlendJointDriver::GetBlendOutTime() const
{
    return m_BlendPhase->GetBlendOutTime();
}

float BlendJointDriver::GetJointWeight(int jointIndex) const
{
    return m_BlendPhase->GetJointWeight(jointIndex);
}

int BlendJointDriver::GetJointPriority(int jointIndex) const
{
    return m_BlendPhase->GetJointPriority(jointIndex);
}

bool BlendJointDriver::HasJointTranslation(int jointIndex) const
{
    return m_BlendPhase->HasJointTranslation(jointIndex);
}

void BlendJointDriver::GetJointTranslation(int jointIndex, rmt::Vector& translation) const
{
    m_BlendPhase->GetJointTranslation(jointIndex, translation);
}

bool BlendJointDriver::HasJointRotation(int jointIndex) const
{
    return m_BlendPhase->HasJointRotation(jointIndex);
}

void BlendJointDriver::GetJointRotation(int jointIndex, rmt::Quaternion& rotation) const
{
    m_BlendPhase->GetJointRotation(jointIndex, rotation);
}

float BlendJointDriver::GetGroupWeight(tUID group) const
{
    return m_BlendPhase->GetGroupWeight();
}

int BlendJointDriver::GetGroupPriority(tUID group) const
{
    return m_BlendPhase->GetGroupPriority();
}

bool BlendJointDriver::HasGroupTranslation(tUID group) const
{
    return m_BlendPhase->HasGroupTranslation(group);
}

void BlendJointDriver::GetGroupTranslation(tUID group, rmt::Vector& translation) const
{
    m_BlendPhase->GetGroupTranslation(group, translation);
}

bool BlendJointDriver::HasGroupRotation(tUID group) const
{
    return m_BlendPhase->HasGroupRotation(group);
}

void BlendJointDriver::GetGroupRotation(tUID group, rmt::Quaternion& rotation) const
{
    m_BlendPhase->GetGroupRotation(group, rotation);
}


//---------------------------------------------------------------------------
// class BlendFootDriver - interface
//---------------------------------------------------------------------------

class BlendFootDriver: public FootBlendDriver
{
public:

    BlendFootDriver(RigLeg* rigLeg,
                    BlendPhase* blendPhase);

    // blending
    virtual float GetFootBlendInTime() const;
    virtual float GetFootBlendOutTime() const;

    // weight & priority
    virtual float GetFootWeight() const;
    virtual int GetFootPriority() const;

    // effector
    virtual bool HasFootEffectorTranslation() const;
    virtual void GetFootEffectorTranslation(rmt::Vector& effTranslate) const;

    virtual bool HasFootEffectorRotation() const;
    virtual void GetFootEffectorRotation(rmt::Quaternion& effRotate) const;

    // pole vector
    virtual bool HasFootPoleVectorTranslation() const;
    virtual void GetFootPoleVectorTranslation(rmt::Vector& pvTranslate) const;

    // plant
    virtual bool HasFootPlanted() const;
    virtual bool IsFootPlanted() const;

    // ground fixup
    virtual bool HasFootGroundFixup() const;
    virtual float GetFootGroundFixup() const;

protected:

    virtual ~BlendFootDriver();

private:

    void GetFootEffectorTranslationIK(rmt::Vector& effTranslate) const;
    void GetFootEffectorTranslationIKWorld(rmt::Vector& effTranslate) const;
    void GetFootEffectorTranslationNoIK(rmt::Vector& effTranslate) const;

    void GetFootEffectorRotationIK(rmt::Quaternion& effRotate) const;
    void GetFootEffectorRotationIKWorld(rmt::Quaternion& effRotate) const;

    RigLeg* m_RigLeg;
    BlendPhase* m_BlendPhase;

    void (BlendFootDriver::* m_GetFootEffectorTranslation)(rmt::Vector&) const;
    void (BlendFootDriver::* m_GetFootEffectorRotation)(rmt::Quaternion&) const;
};


//---------------------------------------------------------------------------
// class BlendFootDriver - implementation
//---------------------------------------------------------------------------

BlendFootDriver::BlendFootDriver(RigLeg* rigLeg,
                                 BlendPhase* blendPhase)
{
    P3DASSERT(rigLeg != 0);
    m_RigLeg = rigLeg;
    m_RigLeg->AddRef();

    P3DASSERT(blendPhase != 0);
    m_BlendPhase = blendPhase;
    m_BlendPhase->AddRef();

    // set up retriever function pointers
    if (m_RigLeg->IsIKRigged())
    {
        Rig* rig = m_BlendPhase->GetBlend()->GetRig();

        int motionRootIndex = rig->GetJointIndex(RIG_JOINT_MOTION_ROOT);
        int effectorIndex = m_RigLeg->GetEffectorJointIndex();
        int aiRootIndex = rig->GetJointIndex(RIG_JOINT_AI_ROOT);

        if ((aiRootIndex < effectorIndex) && (motionRootIndex > effectorIndex))
        {
            m_GetFootEffectorTranslation = &BlendFootDriver::GetFootEffectorTranslationIKWorld;
            m_GetFootEffectorRotation = &BlendFootDriver::GetFootEffectorRotationIKWorld;
        }
        else
        {
            m_GetFootEffectorTranslation = &BlendFootDriver::GetFootEffectorTranslationIK;
            m_GetFootEffectorRotation = &BlendFootDriver::GetFootEffectorRotationIK;
        }
    }
    else
    {
        m_GetFootEffectorTranslation = &BlendFootDriver::GetFootEffectorTranslationNoIK;
        m_GetFootEffectorRotation = 0;
    }
}

BlendFootDriver::~BlendFootDriver()
{
    m_BlendPhase->Release();
    m_RigLeg->Release();
}

float BlendFootDriver::GetFootBlendInTime() const
{
    return m_BlendPhase->GetBlendInTime();
}

float BlendFootDriver::GetFootBlendOutTime() const
{
    return m_BlendPhase->GetBlendOutTime();
}

float BlendFootDriver::GetFootWeight() const
{
    return m_BlendPhase->GetJointWeight(m_RigLeg->GetAnkleJointIndex());
}

int BlendFootDriver::GetFootPriority() const
{
    return m_BlendPhase->GetJointPriority(m_RigLeg->GetAnkleJointIndex());
}

bool BlendFootDriver::HasFootEffectorTranslation() const
{
    return m_BlendPhase->HasJointTranslation(m_RigLeg->GetEffectorJointIndex());
}

void BlendFootDriver::GetFootEffectorTranslationIK(rmt::Vector& effTranslate) const
{
    int effectorIndex = m_RigLeg->GetEffectorJointIndex();

    rmt::Vector tmpTranslate, finalTranslate;
    float totalWeight = 0.0f;

    int i;
    for (i = 0; i < m_BlendPhase->GetBlendSlotCount(); ++i)
    {
        BlendSlot* blendSlot = m_BlendPhase->GetBlendSlot(i);
        if (!blendSlot->HasJointTranslation(effectorIndex))
            continue;

        totalWeight = m_BlendPhase->GetBlendSlotLegWeight(i, m_RigLeg->GetIndex());

        // get translation
        float frame = m_BlendPhase->GetBlendSlotLegFrame(i, m_RigLeg->GetIndex());
        blendSlot->GetJointTranslation(effectorIndex, frame, effTranslate);
        break;
    }

    for (++i; i < m_BlendPhase->GetBlendSlotCount(); ++i)
    {
        BlendSlot* blendSlot = m_BlendPhase->GetBlendSlot(i);
        if (!blendSlot->HasJointTranslation(effectorIndex))
            continue;

        // calculate alpha
        float weight = m_BlendPhase->GetBlendSlotLegWeight(i, m_RigLeg->GetIndex());
        totalWeight += weight;
        float alpha = weight / totalWeight;

        // get translation
        float frame = m_BlendPhase->GetBlendSlotLegFrame(i, m_RigLeg->GetIndex());
        blendSlot->GetJointTranslation(effectorIndex, frame, tmpTranslate);

        // blend
        BlendVector(effTranslate, tmpTranslate, alpha, finalTranslate);
        effTranslate = finalTranslate;
    }
}

void BlendFootDriver::GetFootEffectorTranslationIKWorld(rmt::Vector& effTranslate) const
{
    // FIXME:
    // this is for legacy skeletons (such as DA's max)!  -- we should ditch this code someday

    int effectorIndex = m_RigLeg->GetEffectorJointIndex();
    Rig* rig = m_BlendPhase->GetBlend()->GetRig();
    int motionRootIndex = rig->GetJointIndex(RIG_JOINT_MOTION_ROOT);

    rmt::Vector tmpTranslate, finalTranslate, mrootTranslate;
    float totalWeight = 0.0f;

    int i;
    for (i = 0; i < m_BlendPhase->GetBlendSlotCount(); ++i)
    {
        BlendSlot* blendSlot = m_BlendPhase->GetBlendSlot(i);
        if (!blendSlot->HasJointTranslation(effectorIndex))
            continue;

        totalWeight = m_BlendPhase->GetBlendSlotLegWeight(i, m_RigLeg->GetIndex());

        // get translation
        float frame = m_BlendPhase->GetBlendSlotLegFrame(i, m_RigLeg->GetIndex());
        blendSlot->GetJointTranslation(effectorIndex, frame, effTranslate);

        // transform into character root space
        blendSlot->GetJointTranslation(motionRootIndex, frame, mrootTranslate);
        effTranslate.Sub(mrootTranslate);
        break;
    }

    for (++i; i < m_BlendPhase->GetBlendSlotCount(); ++i)
    {
        BlendSlot* blendSlot = m_BlendPhase->GetBlendSlot(i);
        if (!blendSlot->HasJointTranslation(effectorIndex))
            continue;

        // calculate alpha
        float weight = m_BlendPhase->GetBlendSlotLegWeight(i, m_RigLeg->GetIndex());
        totalWeight += weight;
        float alpha = weight / totalWeight;

        // get translation
        float frame = m_BlendPhase->GetBlendSlotLegFrame(i, m_RigLeg->GetIndex());
        blendSlot->GetJointTranslation(effectorIndex, frame, tmpTranslate);

        // transform into character root space
        blendSlot->GetJointTranslation(motionRootIndex, frame, mrootTranslate);
        tmpTranslate.Sub(mrootTranslate);

        // blend
        BlendVector(effTranslate, tmpTranslate, alpha, finalTranslate);
        effTranslate = finalTranslate;
    }
}

void BlendFootDriver::GetFootEffectorTranslationNoIK(rmt::Vector& effTranslate) const
{
    // NOTE:
    // this will only ever get called for foot matching, and not during the
    // normal blending cycle, eg., during LocomotionDriver::Reset()
    // -- should perhaps investigate better solution that trades memory for speed

    Rig* rig = m_BlendPhase->GetBlend()->GetRig();
    int motionRootIndex = rig->GetJointIndex(RIG_JOINT_MOTION_ROOT);
    tSkeleton* skel = rig->GetSkeleton();

    rmt::Vector tmpTranslate;
    float totalWeight = 0.0f;

    rmt::Matrix tmpMatrix;
    tmpMatrix.Identity();
    rmt::Quaternion tmpRotate;

    int ankleIndex = m_RigLeg->GetAnkleJointIndex();

    for (int i = 0; i < m_BlendPhase->GetBlendSlotCount(); ++i)
    {
        BlendSlot* blendSlot = m_BlendPhase->GetBlendSlot(i);
        if (!blendSlot->HasJointTranslation(ankleIndex))
            continue;

        float weight = m_BlendPhase->GetBlendSlotLegWeight(i, m_RigLeg->GetIndex());
        totalWeight += weight;
        float alpha = weight / totalWeight;

        // get translation
        float frame = m_BlendPhase->GetBlendSlotLegFrame(i, m_RigLeg->GetIndex());
        blendSlot->GetJointTranslation(ankleIndex, frame, tmpTranslate);

        tmpMatrix.FillTranslate(tmpTranslate);

        // transform into character root space
        int jointIndex = skel->GetJoint(ankleIndex)->parentIndex;
        while ((jointIndex > 0) &&
               (jointIndex > motionRootIndex))
        {
            rmt::Matrix tmp;

            if (blendSlot->HasJointTranslation(jointIndex))
            {
                blendSlot->GetJointTranslation(jointIndex, frame, tmpTranslate);
            }
            else
            {
                tmpTranslate = skel->GetJoint(jointIndex)->restPose.Row(3);
            }

            if (blendSlot->HasJointRotation(jointIndex))
            {
                blendSlot->GetJointRotation(jointIndex, frame, tmpRotate);
            }
            else
            {
                tmpRotate.BuildFromMatrix(skel->GetJoint(jointIndex)->restPose);
            }

            tmp.Identity();
            tmpRotate.ConvertToMatrix(&tmp);
            tmp.FillTranslate(tmpTranslate);

            tmpMatrix.Mult(tmp);

            jointIndex = skel->GetJoint(jointIndex)->parentIndex;
        }

        // blend
        rmt::Vector finalTranslate;
        BlendVector(effTranslate, tmpMatrix.Row(3), alpha, finalTranslate);
        effTranslate = finalTranslate;
    }
}

void BlendFootDriver::GetFootEffectorTranslation(rmt::Vector& effTranslate) const
{
    // dispatch appropriate member
    (this->*m_GetFootEffectorTranslation)(effTranslate);
}

bool BlendFootDriver::HasFootEffectorRotation() const
{
    return m_BlendPhase->HasJointRotation(m_RigLeg->GetEffectorJointIndex());
}

void BlendFootDriver::GetFootEffectorRotationIK(rmt::Quaternion& effRotate) const
{
    int effectorIndex = m_RigLeg->GetEffectorJointIndex();

    rmt::Quaternion tmpRotate, finalRotate;
    float totalWeight = 0.0f;

    int i;
    for (i = 0; i < m_BlendPhase->GetBlendSlotCount(); ++i)
    {
        BlendSlot* blendSlot = m_BlendPhase->GetBlendSlot(i);
        if (!blendSlot->HasJointRotation(effectorIndex))
            continue;

        totalWeight = m_BlendPhase->GetBlendSlotLegWeight(i, m_RigLeg->GetIndex());

        // get rotation
        float frame = m_BlendPhase->GetBlendSlotLegFrame(i, m_RigLeg->GetIndex());
        blendSlot->GetJointRotation(effectorIndex, frame, effRotate);
        break;
    }

    for (++i; i < m_BlendPhase->GetBlendSlotCount(); ++i)
    {
        BlendSlot* blendSlot = m_BlendPhase->GetBlendSlot(i);
        if (!blendSlot->HasJointRotation(effectorIndex))
            continue;

        float weight = m_BlendPhase->GetBlendSlotLegWeight(i, m_RigLeg->GetIndex());
        totalWeight += weight;
        float alpha = weight / totalWeight;

        // get rotation
        float frame = m_BlendPhase->GetBlendSlotLegFrame(i, m_RigLeg->GetIndex());
        blendSlot->GetJointRotation(effectorIndex, frame, tmpRotate);

        // blend
        BlendQuaternion(effRotate, tmpRotate, alpha, finalRotate);
        effRotate = finalRotate;
    }
}

void BlendFootDriver::GetFootEffectorRotationIKWorld(rmt::Quaternion& effRotate) const
{
    // FIXME:
    // this is for legacy skeletons (such as DA's max)!  -- we should ditch this code someday

    int effectorIndex = m_RigLeg->GetEffectorJointIndex();
    Rig* rig = m_BlendPhase->GetBlend()->GetRig();
    int motionRootIndex = rig->GetJointIndex(RIG_JOINT_MOTION_ROOT);

    rmt::Quaternion tmpRotate, finalRotate, mrootRotate;
    float totalWeight = 0.0f;

    int i;
    for (i = 0; i < m_BlendPhase->GetBlendSlotCount(); ++i)
    {
        BlendSlot* blendSlot = m_BlendPhase->GetBlendSlot(i);
        if (!blendSlot->HasJointRotation(effectorIndex))
            continue;

        totalWeight = m_BlendPhase->GetBlendSlotLegWeight(i, m_RigLeg->GetIndex());

        // get rotation
        float frame = m_BlendPhase->GetBlendSlotLegFrame(i, m_RigLeg->GetIndex());
        blendSlot->GetJointRotation(effectorIndex, frame, effRotate);

        // transform into character root space
        blendSlot->GetJointRotation(motionRootIndex, frame, mrootRotate);
        mrootRotate.Inverse();
        effRotate.Mult(mrootRotate);
        break;
    }

    for (++i; i < m_BlendPhase->GetBlendSlotCount(); ++i)
    {
        BlendSlot* blendSlot = m_BlendPhase->GetBlendSlot(i);
        if (!blendSlot->HasJointRotation(effectorIndex))
            continue;

        float weight = m_BlendPhase->GetBlendSlotLegWeight(i, m_RigLeg->GetIndex());
        totalWeight += weight;
        float alpha = weight / totalWeight;

        // get rotation
        float frame = m_BlendPhase->GetBlendSlotLegFrame(i, m_RigLeg->GetIndex());
        blendSlot->GetJointRotation(effectorIndex, frame, tmpRotate);

        // transform into character root space
        blendSlot->GetJointRotation(motionRootIndex, frame, mrootRotate);
        mrootRotate.Inverse();
        tmpRotate.Mult(mrootRotate);

        // blend
        BlendQuaternion(effRotate, tmpRotate, alpha, finalRotate);
        effRotate = finalRotate;
    }
}

void BlendFootDriver::GetFootEffectorRotation(rmt::Quaternion& effRotate) const
{
    // dispatch appropriate member
    (this->*m_GetFootEffectorRotation)(effRotate);
}

bool BlendFootDriver::HasFootPoleVectorTranslation() const
{
    return m_BlendPhase->HasJointTranslation(m_RigLeg->GetPoleVectorJointIndex());
}

void BlendFootDriver::GetFootPoleVectorTranslation(rmt::Vector& pvTranslate) const
{
    rmt::Vector tmpTranslate, finalTranslate;
    float totalWeight = 0.0f;

    int poleVectorIndex = m_RigLeg->GetPoleVectorJointIndex();

    int i;
    for (i = 0; i < m_BlendPhase->GetBlendSlotCount(); ++i)
    {
        BlendSlot* blendSlot = m_BlendPhase->GetBlendSlot(i);
        if (!blendSlot->HasJointTranslation(poleVectorIndex))
            continue;

        totalWeight = m_BlendPhase->GetBlendSlotLegWeight(i, m_RigLeg->GetIndex());

        // get pv
        float frame = m_BlendPhase->GetBlendSlotLegFrame(i, m_RigLeg->GetIndex());
        blendSlot->GetJointTranslation(poleVectorIndex, frame, pvTranslate);
        break;
    }

    for (++i; i < m_BlendPhase->GetBlendSlotCount(); ++i)
    {
        BlendSlot* blendSlot = m_BlendPhase->GetBlendSlot(i);
        if (!blendSlot->HasJointTranslation(poleVectorIndex))
            continue;

        float weight = m_BlendPhase->GetBlendSlotLegWeight(i, m_RigLeg->GetIndex());
        totalWeight += weight;
        float alpha = weight / totalWeight;

        // get pv
        float frame = m_BlendPhase->GetBlendSlotLegFrame(i, m_RigLeg->GetIndex());
        blendSlot->GetJointTranslation(poleVectorIndex, frame, tmpTranslate);

        // blend
        rmt::Vector finalTranslate;
        BlendVector(pvTranslate, tmpTranslate, alpha, finalTranslate);
        pvTranslate = finalTranslate;
    }
}

bool BlendFootDriver::HasFootPlanted() const
{
    return true;
}

bool BlendFootDriver::IsFootPlanted() const
{
    return m_BlendPhase->IsFootPlanted(m_RigLeg->GetIndex());
}

bool BlendFootDriver::HasFootGroundFixup() const
{
    return true;
}

float BlendFootDriver::GetFootGroundFixup() const
{
    // FIXME:  this should be derived from animation
    return 1.0f;
}


//---------------------------------------------------------------------------
// class BlendDriver
//---------------------------------------------------------------------------

BlendDriver::BlendDriver(Blend* blend)
{
    P3DASSERT(blend != 0);
    CopyName(blend);

    m_Blend = blend;
    m_Blend->AddRef();

    m_BlendPhase = new BlendPhase(m_Blend);
    m_BlendPhase->AddRef();

    m_BlendJointDriver = new BlendJointDriver(m_BlendPhase);
    m_BlendJointDriver->AddRef();

    Rig* rig = m_Blend->GetRig();
    int legCount = rig->GetLegCount();
    m_BlendFootDrivers = new BlendFootDriver* [legCount];

    for (int i = 0; i < legCount; ++i)
    {
        m_BlendFootDrivers[i] = new BlendFootDriver(rig->GetLeg(i),
                                                    m_BlendPhase);
        m_BlendFootDrivers[i]->AddRef();
    }
}

BlendDriver::~BlendDriver()
{
    int legCount = m_Blend->GetRig()->GetLegCount();
    for (int i = (legCount - 1); i >= 0; --i)
    {
        m_BlendFootDrivers[i]->Release();
    }
    delete[] m_BlendFootDrivers;

    m_BlendJointDriver->Release();
    m_BlendPhase->Release();
    m_Blend->Release();
}

PuppetDriver* BlendDriver::Clone() const
{
    return new BlendDriver(m_Blend);
}

Partition* BlendDriver::GetPartition() const
{
    return m_BlendPhase->GetPartition();
}

void BlendDriver::SetPartition(Partition* partition)
{
    m_BlendPhase->SetPartition(partition);
}

float BlendDriver::GetWeight() const
{
    return m_BlendPhase->GetWeight();
}

void BlendDriver::SetWeight(float weight)
{
    m_BlendPhase->SetWeight(weight);
}

int BlendDriver::GetPriority() const
{
    return m_BlendPhase->GetPriority();
}

void BlendDriver::SetPriority(int priority)
{
    m_BlendPhase->SetPriority(priority);
}

float BlendDriver::GetRootBlendInTime() const
{
    return m_BlendPhase->GetRootBlendInTime();
}

void BlendDriver::SetRootBlendInTime(float rootBlendInTime)
{
    m_BlendPhase->SetRootBlendInTime(rootBlendInTime);
}

float BlendDriver::GetRootBlendOutTime() const
{
    return m_BlendPhase->GetRootBlendOutTime();
}

void BlendDriver::SetRootBlendOutTime(float rootBlendOutTime)
{
    m_BlendPhase->SetRootBlendOutTime(rootBlendOutTime);
}

float BlendDriver::GetBlendInTime() const
{
    return m_BlendPhase->GetBlendInTime();
}

void BlendDriver::SetBlendInTime(float blendInTime)
{
    m_BlendPhase->SetBlendInTime(blendInTime);
}

float BlendDriver::GetBlendOutTime() const
{
    return m_BlendPhase->GetBlendOutTime();
}

void BlendDriver::SetBlendOutTime(float blendOutTime)
{
    m_BlendPhase->SetBlendOutTime(blendOutTime);
}

float BlendDriver::GetParamValue(int i) const
{
    return m_BlendPhase->GetParamValue(i);
}

void BlendDriver::SetParamValue(int i, float value)
{
    m_BlendPhase->SetParamValue(i, value);
}

bool BlendDriver::IsCyclic() const
{
    return m_BlendPhase->IsCyclic();
}

void BlendDriver::SetIsCyclic(bool isCyclic)
{
    m_BlendPhase->SetIsCyclic(isCyclic);
}

float BlendDriver::GetPhase() const
{
    return m_BlendPhase->GetPhase();
}

void BlendDriver::SetPhase(float phase)
{
    m_BlendPhase->SetPhase(phase);
}

float BlendDriver::GetFootPlant(int foot) const
{
    return m_BlendPhase->GetFootPlant(foot);
}

float BlendDriver::GetFootLift(int foot) const
{
    return m_BlendPhase->GetFootLift(foot);
}

float BlendDriver::GetFootScale(int foot) const
{
    return m_BlendPhase->GetFootScale(foot);
}

float BlendDriver::GetFootPhase(int foot) const
{
    return m_BlendPhase->GetFootPhase(foot);
}

bool BlendDriver::IsFootPlanted(int foot) const
{
    return m_BlendPhase->IsFootPlanted(foot);
}

void BlendDriver::GetFootEffectorTranslation(int foot, rmt::Vector& effTranslate) const
{
    int legCount = m_Blend->GetRig()->GetLegCount();
    P3DASSERT((foot >= 0) && (foot < legCount));
    if ((foot < 0) || (foot >= legCount))
        return;

    m_BlendFootDrivers[foot]->GetFootEffectorTranslation(effTranslate);
}

float BlendDriver::GetJointWeight(int jointIndex) const
{
    return m_BlendPhase->GetJointWeight(jointIndex);
}

int BlendDriver::GetJointPriority(int jointIndex) const
{
    return m_BlendPhase->GetJointPriority(jointIndex);
}

int BlendDriver::GetBlendSlotCount() const
{
    return m_BlendPhase->GetBlendSlotCount();
}

BlendSlot* BlendDriver::GetBlendSlot(int i) const
{
    return m_BlendPhase->GetBlendSlot(i);
}

float BlendDriver::GetBlendThreshold() const
{
    return m_BlendPhase->GetBlendThreshold();
}

void BlendDriver::SetBlendThreshold(float blendThreshold)
{
    m_BlendPhase->SetBlendThreshold(blendThreshold);
}

int BlendDriver::GetMaxBlendedSlotCount() const
{
    return m_BlendPhase->GetMaxBlendedSlotCount();
}

void BlendDriver::SetMaxBlendedSlotCount(int maxSlotCount)
{
    m_BlendPhase->SetMaxBlendedSlotCount(maxSlotCount);
}

void BlendDriver::Begin(Puppet* puppet)
{
    // do nothing
}

void BlendDriver::Advance(float deltaTime)
{
    m_BlendPhase->UpdateBlends();
    m_BlendPhase->AdvancePhase(deltaTime);
    m_BlendPhase->UpdateLegPhase();
}

bool BlendDriver::IsFinished() const
{
    return m_BlendPhase->IsFinished();
}

void BlendDriver::End(Puppet* puppet)
{
    // do nothing
}

void BlendDriver::Apply(Engine* engine)
{
    if (m_BlendPhase->GetWeight() > 0.0f)
    {
        engine->GetJointBlender()->Push(m_BlendJointDriver);
        
        int legCount = m_Blend->GetRig()->GetLegCount();
        for (int i = 0; i < legCount; ++i)
        {
            engine->GetFootBlender(i)->Push(m_BlendFootDrivers[i]);
        }
    }
}


} // namespace choreo


// End of file.
