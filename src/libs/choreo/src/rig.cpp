/*
 * choreo/rig.cpp
 */

#include <choreo/rig.hpp>
#include <choreo/scriptreader.hpp>
#include <choreo/scriptwriter.hpp>
#include <choreo/basebank.hpp>
#include <choreo/utility.hpp>
#include <choreo/load.hpp>

#include <p3d/anim/skeleton.hpp>

#include <stdio.h>
#include <string.h>

#include <p3d/error.hpp>


namespace choreo
{


//---------------------------------------------------------------------------
// Script Tokens
//---------------------------------------------------------------------------

// rig tokens
static const char* TOKEN_RIG                           = "rig";
static const char* TOKEN_NAME                          = "name";
static const char* TOKEN_SKELETON                      = "skeleton";
static const char* TOKEN_LEG                           = "leg";
static const char* TOKEN_BALANCEROOTMINYRATE           = "balanceRootMinYRate";
static const char* TOKEN_BALANCEROOTYRATEVELOCITYSCALE = "balanceRootYRateVelocityScale";
static const char* TOKEN_MAXBALANCEROOTCOMPRESS        = "maxBalanceRootCompress";

// rig named joints
struct RigJointStruct
{
    const char* token;
    bool required;
};
// must match RigJointIndex enum order
static const RigJointStruct g_RigJoints[RIG_JOINT_COUNT] =
{
    { "AIRoot",          true  },
    { "MotionRoot",      false },
    { "OrientationRoot", false },
    { "BalanceRoot",     false },
    { "CharacterRoot",   false }
};

// rig leg tokens
static const char* TOKEN_FOOTPLANTCHANNEL       = "footPlantChannel";
static const char* TOKEN_FOOTPLANTCHANNELINDEX  = "footPlantChannelIndex";
static const char* TOKEN_FOOTPLANTVALUE         = "footPlantValue";
static const char* TOKEN_FOOTLIFTVALUE          = "footLiftValue";
static const char* TOKEN_MINYRATE               = "minYRate";
static const char* TOKEN_YRATEVELOCITYSCALE     = "yRateVelocityScale";
static const char* TOKEN_ISIKRIGGED             = "isIKRigged";

// foot plant channels
struct RigLegChannelStruct
{
    const char* token;
    int index;
};
static const RigLegChannelStruct g_RigLegChannels[] =
{
    { "x", 0 },
    { "X", 0 },
    { "y", 1 },
    { "Y", 1 },
    { "z", 2 },
    { "Z", 2 }
};
static const int g_RigLegChannelCount = sizeof(g_RigLegChannels) / sizeof(RigLegChannelStruct);

// leg joints
typedef void (RigLeg::* RigLegJointSetMethod)(int);
typedef int (RigLeg::* RigLegJointGetMethod)() const;
struct RigLegJointStruct
{
    const char* token;
    bool required;
    int range;
    RigLegJointSetMethod setMethod;
    RigLegJointGetMethod getMethod;
};
static const RigLegJointStruct g_RigLegJoints[] =
{
    { "FootPlant",    true,  -1, &RigLeg::SetFootPlantJointIndex,    &RigLeg::GetFootPlantJointIndex },
    { "FKMin",        true,  -1, &RigLeg::SetMinimumFKJointIndex,    &RigLeg::GetMinimumFKJointIndex },
    { "FKMax",        true,  -1, &RigLeg::SetMaximumFKJointIndex,    &RigLeg::GetMaximumFKJointIndex },
    { "Thigh",        false,  0, &RigLeg::SetThighJointIndex,        &RigLeg::GetThighJointIndex },
    { "Knee",         false,  0, &RigLeg::SetKneeJointIndex,         &RigLeg::GetKneeJointIndex },
    { "Ankle",        true,   0, &RigLeg::SetAnkleJointIndex,        &RigLeg::GetAnkleJointIndex },
    { "IKMin",        false, -1, &RigLeg::SetMinimumIKJointIndex,    &RigLeg::GetMinimumIKJointIndex },
    { "IKMax",        false, -1, &RigLeg::SetMaximumIKJointIndex,    &RigLeg::GetMaximumIKJointIndex },
    { "Effector",     false,  1, &RigLeg::SetEffectorJointIndex,     &RigLeg::GetEffectorJointIndex },
    { "PoleVector",   false,  1, &RigLeg::SetPoleVectorJointIndex,   &RigLeg::GetPoleVectorJointIndex },
    { "OrientOffset", false, -1, &RigLeg::SetOrientOffsetJointIndex, &RigLeg::GetOrientOffsetJointIndex }
};
static const int g_RigLegJointCount = sizeof(g_RigLegJoints) / sizeof(RigLegJointStruct);

// leg joint ranges (fk/ik)
struct RigLegJointRange
{
    int min;
    int max;
};
static const RigLegJointRange g_RigLegJointRanges[] =
{
    { 1, 2 },
    { 6, 7 }
};
static const int g_RigLegJointRangeCount = sizeof(g_RigLegJointRanges) / sizeof(RigLegJointRange);


//---------------------------------------------------------------------------
// class RigLeg
//---------------------------------------------------------------------------

RigLeg::RigLeg():
    m_Index(-1),

    m_FootPlantJointIndex(-1),
    m_FootPlantChannelIndex(0),
    m_FootPlantValue(RIG_LEG_FOOT_PLANT_VALUE_DEFAULT),
    m_FootLiftValue(RIG_LEG_FOOT_LIFT_VALUE_DEFAULT),

    m_MinimumFKJointIndex(-1),
    m_MaximumFKJointIndex(-1),

    m_ThighJointIndex(-1),
    m_KneeJointIndex(-1),
    m_AnkleJointIndex(-1),

    m_MinimumIKJointIndex(-1),
    m_MaximumIKJointIndex(-1),

    m_EffectorJointIndex(-1),
    m_PoleVectorJointIndex(-1),
    m_OrientOffsetJointIndex(-1),

    m_MinYRate(RIG_LEG_MIN_Y_RATE_DEFAULT),
    m_YRateVelocityScale(RIG_LEG_Y_RATE_VELOCITY_SCALE_DEFAULT),

    m_IsIKRigged(false)
{
}

RigLeg::~RigLeg()
{
}

bool RigLeg::ContainsJointIndex(int jointIndex) const
{
    return (((jointIndex >= m_MinimumFKJointIndex) &&
             (jointIndex <= m_MaximumFKJointIndex)) ||
            ((jointIndex >= m_MinimumIKJointIndex) &&
             (jointIndex <= m_MaximumIKJointIndex)));
}

bool RigLeg::ReadFromScript(ScriptReader* script, BaseBank* bank, Rig* rig)
{
    struct JointStruct
    {
        JointStruct():
            indexf(-1.0f),
            lineNo(-1),
            index(-1)
        {
            name[0] = 0;
        }

        char name[64];
        float indexf;
        int lineNo;
        int index;
    };
    JointStruct joints[g_RigLegJointCount];

    if (!script->IsMoreStructs())
        return false;
    if (strcmp(script->GetNextStructIdentifier(), TOKEN_LEG) != 0)
        return false;
    
    script->BeginStruct(TOKEN_LEG);
    
    // required params
    char legName[256] = "";
    script->RequiredParamString(TOKEN_NAME, legName, sizeof(legName));
    
    // optional params
    int footPlantChannelLine = -1;
    char footPlantChannel[256] = "";
    script->ParamString(TOKEN_FOOTPLANTCHANNEL,
                        footPlantChannel,
                        sizeof(footPlantChannel),
                        &footPlantChannelLine);
    float footPlantChannelIndexf = -1.0f;
    script->ParamFloat(TOKEN_FOOTPLANTCHANNELINDEX,
                       &footPlantChannelIndexf,
                       &footPlantChannelLine);
    
    float footPlantValue = RIG_LEG_FOOT_PLANT_VALUE_DEFAULT;
    script->ParamFloat(TOKEN_FOOTPLANTVALUE, &footPlantValue);
    
    float footLiftValue = RIG_LEG_FOOT_LIFT_VALUE_DEFAULT;
    script->ParamFloat(TOKEN_FOOTLIFTVALUE, &footLiftValue);
    
    float minYRate = RIG_LEG_MIN_Y_RATE_DEFAULT;
    script->ParamFloat(TOKEN_MINYRATE, &minYRate);
    
    float yRateVelocityScale = RIG_LEG_Y_RATE_VELOCITY_SCALE_DEFAULT;
    script->ParamFloat(TOKEN_YRATEVELOCITYSCALE, &yRateVelocityScale);
    
    float isIKRiggedf = 0.0f;
    script->ParamFloat(TOKEN_ISIKRIGGED, &isIKRiggedf);
    
    // joints
    char tmp[256];
    int i;
    for (i = 0; i < g_RigLegJointCount; ++i)
    {
        StrPrintf(tmp, sizeof(tmp), "joint_%s", g_RigLegJoints[i].token);
        script->ParamString(tmp, joints[i].name, sizeof(joints[i].name), &(joints[i].lineNo));
        StrPrintf(tmp, sizeof(tmp), "jointIndex_%s", g_RigLegJoints[i].token);
        script->ParamFloat(tmp, &(joints[i].indexf), &(joints[i].lineNo));
    }
    
    script->EndStruct();
    
    // resolve bool params
    bool isIKRigged = !rmt::Epsilon(isIKRiggedf, 0.0f);
    
    // resolve foot plant channel
    int footPlantChannelIndex = -1;
    if (strlen(footPlantChannel) > 0)
    {
        if (footPlantChannelIndexf >= 0.0f)
        {
            script->SetErrorString(footPlantChannelLine,
                "Ambiguous definition of foot plant channel.");
        }
        else
        {
            for (i = 0; i < g_RigLegChannelCount; ++i)
            {
                if (strcmp(footPlantChannel, g_RigLegChannels[i].token) == 0)
                {
                    footPlantChannelIndex = g_RigLegChannels[i].index;
                    break;
                }
            }
            
            if (footPlantChannelIndex < 0)
            {
                script->SetErrorFormat(footPlantChannelLine,
                    "Unknown foot plant channel token \"%s\".",
                    footPlantChannel);
            }
        }
    }
    else
    {
        footPlantChannelIndex = (int)footPlantChannelIndexf;
    }
    // verify foot plant channel index
    if ((footPlantChannelIndex < 0) || (footPlantChannelIndex > 2))
    {
        script->SetErrorFormat(footPlantChannelLine,
            "Foot plant channel index %d out of range.",
            footPlantChannelIndex);
    }
    
    // resolve joints
    for (i = 0; i < g_RigLegJointCount; ++i)
    {
        JointStruct* joint = (joints + i);

        if (strlen(joint->name) > 0)
        {
            if (joint->indexf >= 0.0f)
            {
                script->SetErrorFormat(joints[i].lineNo,
                    "Duplicate definition for joint \"%s\".",
                    g_RigLegJoints[i].token);
            }
            else
            {
                joint->index = rig->GetSkeleton()->FindJointIndex(joint->name);
                
                if (joint->index < 0)
                {
                    script->SetErrorFormat(joint->lineNo,
                        "Could not find joint named \"%s\".",
                        joint->name);
                }
            }
        }
        else
        {
            joint->index = (int)joint->indexf;
        }
    }
    
    // handle setting range min/max
    for (int range = 0; range < g_RigLegJointRangeCount; ++range)
    {
        for (i = 0; i < g_RigLegJointCount; ++i)
        {
            JointStruct* joint = (joints + i);

            if ((g_RigLegJoints[i].range == range) &&
                (joint->index >= 0))
            {
                if ((joints[g_RigLegJointRanges[range].min].index < 0) ||
                    (joint->index < joints[g_RigLegJointRanges[range].min].index))
                {
                    joints[g_RigLegJointRanges[range].min].index = joint->index;
                }
                
                if ((joints[g_RigLegJointRanges[range].max].index < 0) ||
                    (joint->index > joints[g_RigLegJointRanges[range].max].index))
                {
                    joints[g_RigLegJointRanges[range].max].index = joint->index;
                }
            }
        }
    }
    
    // verify required joints
    for (i = 0; i < g_RigLegJointCount; ++i)
    {
        JointStruct* joint = (joints + i);

        if (g_RigLegJoints[i].required)
        {
            if ((joint->index < 0) ||
                (joint->index >= rig->GetJointCount()))
            {
                if (joint->lineNo >= 0)
                {
                    script->SetErrorFormat(joint->lineNo,
                        "Required joint index \"%s\" out of range.",
                        g_RigLegJoints[i].token);
                }
                else
                {
                    script->SetErrorFormat("Required joint index \"%s\" not set.",
                        g_RigLegJoints[i].token);
                }
            }
        }
        else
        {
            if ((joint->index < -1) ||
                (joint->index >= rig->GetJointCount()))
            {
                script->SetErrorFormat(joint->lineNo,
                    "Joint index \"%s\" out of range.",
                    g_RigLegJoints[i].token);
            }
        }
    }
    
    // verify ik joints, if rigged
    if (isIKRigged)
    {
        for (i = 0; i < g_RigLegJointCount; ++i)
        {
            JointStruct* joint = (joints + i);

            if ((joint->index < 0) ||
                (joint->index >= rig->GetJointCount()))
            {
                if (joint->lineNo >= 0)
                {
                    script->SetErrorFormat(joint->lineNo,
                        "Required IK joint index \"%s\" out of range.",
                        g_RigLegJoints[i].token);
                }
                else
                {
                    script->SetErrorFormat("Required IK joint \"%s\" not set.",
                        g_RigLegJoints[i].token);
                }
            }
        }
    }
    
    if (script->IsError())
    {
        return script->HandleError();
    }
    
    // allocate leg
    RigLeg* leg = new RigLeg();
    leg->AddRef();
    
    // set attribs
    leg->SetName(legName);
    leg->SetFootPlantChannelIndex(footPlantChannelIndex);
    leg->SetFootPlantValue(footPlantValue);
    leg->SetFootLiftValue(footLiftValue);
    leg->SetMinYRate(minYRate);
    leg->SetYRateVelocityScale(yRateVelocityScale);
    leg->SetIsIKRigged(isIKRigged);
    
    // set joints
    for (i = 0; i < g_RigLegJointCount; ++i)
    {
        (leg->*(g_RigLegJoints[i].setMethod))(joints[i].index);
    }
    
    // add leg
    rig->AddLeg(leg);
    
    leg->Release();
    return true;
}

#ifdef CHOREO_TOOL
bool RigLeg::WriteToScript(ScriptWriter* script) const
{
    script->BeginStruct(TOKEN_LEG);

    // required params
    script->RequiredParamString(TOKEN_NAME, GetName());

    // optional params
    script->ParamFloat(TOKEN_FOOTPLANTCHANNELINDEX, (float)m_FootPlantChannelIndex);
    script->ParamFloat(TOKEN_FOOTPLANTVALUE, m_FootPlantValue);
    script->ParamFloat(TOKEN_FOOTLIFTVALUE, m_FootLiftValue);
    script->ParamFloat(TOKEN_MINYRATE, m_MinYRate);
    script->ParamFloat(TOKEN_YRATEVELOCITYSCALE, m_YRateVelocityScale);
    script->ParamFloat(TOKEN_ISIKRIGGED, (float)m_IsIKRigged);

    // joints
    char tmp[256];
    for (int i = 0; i < g_RigLegJointCount; ++i)
    {
        int jointIndex = (this->*(g_RigLegJoints[i].getMethod))();
        StrPrintf(tmp, sizeof(tmp), "jointIndex_%s", g_RigLegJoints[i].token);
        script->ParamFloat(tmp, (float)jointIndex);
    }

    script->EndStruct();
    return true;
}
#endif // CHOREO_TOOL


//---------------------------------------------------------------------------
// class Rig
//---------------------------------------------------------------------------

Rig::Rig(tSkeleton* skeleton,
         int maxLegCount):
    m_MaxLegCount(maxLegCount),
    m_LegCount(0),

    m_BalanceRootMinYRate(RIG_BALANCE_ROOT_MIN_Y_RATE_DEFAULT),
    m_BalanceRootYRateVelocityScale(RIG_BALANCE_ROOT_Y_RATE_VELOCITY_SCALE_DEFAULT),
    m_MaxBalanceRootCompress(RIG_MAX_BALANCE_ROOT_COMPRESS_DEFAULT)
{
    m_Skeleton = skeleton;
    m_Skeleton->AddRef();
    
    for (int i = 0; i < RIG_JOINT_COUNT; ++i)
    {
        m_JointIndices[i] = -1;
    }
    
    m_Legs = new RigLeg* [m_MaxLegCount];
}

Rig::~Rig()
{
    for (int i = m_LegCount - 1; i >= 0; --i)
    {
        m_Legs[i]->Release();
    }
    delete[] m_Legs;
    
    m_Skeleton->Release();
}

int Rig::GetJointCount() const
{
    return m_Skeleton->GetNumJoint();
}

int Rig::GetJointIndex(RigJointIndex index) const
{
    return m_JointIndices[index];
}

void Rig::SetJointIndex(RigJointIndex index, int jointIndex)
{
    m_JointIndices[index] = jointIndex;
}

RigLeg* Rig::GetLeg(int i) const
{
    P3DASSERT((i >= 0) && (i < m_LegCount));
    if ((i < 0) || (i >= m_LegCount))
        return 0;
    
    return m_Legs[i];
}

RigLeg* Rig::FindLeg(tUID legUID) const
{
    for (int i = 0; i < m_LegCount; ++i)
    {
        if (m_Legs[i]->GetUID() == legUID)
        {
            return m_Legs[i];
        }
    }
    
    return 0;
}

int Rig::FindLegIndex(tUID legUID) const
{
    for (int i = 0; i < m_LegCount; ++i)
    {
        if (m_Legs[i]->GetUID() == legUID)
        {
            return i;
        }
    }
    
    return -1;
}

bool Rig::AddLeg(RigLeg* leg)
{
    P3DASSERT(m_LegCount < m_MaxLegCount);
    if (m_LegCount >= m_MaxLegCount)
        return false;
    
    m_Legs[m_LegCount] = leg;
    m_Legs[m_LegCount]->AddRef();
    m_Legs[m_LegCount]->SetIndex(m_LegCount);
    ++m_LegCount;
    
    return true;
}

bool Rig::ReadFromScript(ScriptReader* script, BaseBank* bank)
{
    struct JointStruct
    {
        JointStruct():
            indexf(-1.0f),
            index(-1),
            lineNo(-1)
        {
            name[0] = 0;
        }

        char name[64];
        float indexf;
        int index;
        int lineNo;
    };
    JointStruct joints[RIG_JOINT_COUNT];
    
    if (!script->IsMoreStructs())
        return false;
    if (strcmp(script->GetNextStructIdentifier(), TOKEN_RIG) != 0)
        return false;
    
    script->BeginStruct(TOKEN_RIG);
    
    // required params
    char rigName[256] = "";
    script->RequiredParamString(TOKEN_NAME, rigName, sizeof(rigName));
    
    char skeletonName[256] = "";
    script->RequiredParamString(TOKEN_SKELETON, skeletonName, sizeof(skeletonName));
    
    tSkeleton* skeleton = choreo::find<tSkeleton>(bank, skeletonName);
    if (skeleton == 0)
    {
        script->SetErrorFormat("Could not find skeleton named \"%s\".", skeletonName);
    }
    
    // joints
    int i;
    for (i = 0; i < RIG_JOINT_COUNT; ++i)
    {
        char tmp[256];
        
        StrPrintf(tmp, sizeof(tmp), "joint_%s", g_RigJoints[i].token);
        script->ParamString(tmp, joints[i].name, sizeof(joints[i].name), &(joints[i].lineNo));
        
        StrPrintf(tmp, sizeof(tmp), "jointIndex_%s", g_RigJoints[i].token);
        script->ParamFloat(tmp, &(joints[i].indexf), &(joints[i].lineNo));
    }
    
    // optional params
    float balanceRootMinYRate = RIG_BALANCE_ROOT_MIN_Y_RATE_DEFAULT;
    script->ParamFloat(TOKEN_BALANCEROOTMINYRATE, &balanceRootMinYRate);
    
    float balanceRootYRateVelocityScale = RIG_BALANCE_ROOT_Y_RATE_VELOCITY_SCALE_DEFAULT;
    script->ParamFloat(TOKEN_BALANCEROOTYRATEVELOCITYSCALE, &balanceRootYRateVelocityScale);
    
    float maxBalanceRootCompress = RIG_MAX_BALANCE_ROOT_COMPRESS_DEFAULT;
    script->ParamFloat(TOKEN_MAXBALANCEROOTCOMPRESS, &maxBalanceRootCompress);
    
    // leg counting
    Rig* rig = 0;
    int legCount = script->GetStructCount(TOKEN_LEG);
    
    if (!script->IsError())
    {
        // allocate rig
        rig = new Rig(skeleton, legCount);
        rig->AddRef();
        rig->SetName(rigName);
        
        while (script->IsMoreStructs())
        {
            bool rc = false;
            
            rc = (RigLeg::ReadFromScript(script, bank, rig) || rc);
            
            if (!rc && script->IsMoreStructs())
            {
                script->SetErrorFormat("Unexpected token \"%s\".",
                    script->GetNextStructIdentifier());
            }
        }
    }
    
    script->EndStruct();
    
    if (skeleton != 0)
    {
        for (i = 0; i < RIG_JOINT_COUNT; ++i)
        {
            if (strlen(joints[i].name) > 0)
            {
                if (joints[i].indexf > 0.0f)
                {
                    script->SetErrorFormat(joints[i].lineNo,
                        "Duplicate definition for joint \"%s\".",
                        g_RigJoints[i].token);
                }
                else
                {
                    joints[i].index = skeleton->FindJointIndex(joints[i].name);
                    
                    if (joints[i].index < 0)
                    {
                        script->SetErrorFormat(joints[i].lineNo,
                            "Could not find joint named \"%s\" in skeleton \"%s\".",
                            joints[i].name,
                            skeletonName);
                    }
                }
            }
            else
            {
                joints[i].index = (int)joints[i].indexf;
            }
            
            // verify joint index
            if (g_RigJoints[i].required)
            {
                if ((joints[i].index < 0) ||
                    (joints[i].index >= skeleton->GetNumJoint()))
                {
                    if (joints[i].lineNo >= 0)
                    {
                        script->SetErrorFormat(joints[i].lineNo,
                            "Required joint \"%s\" out of range.",
                            g_RigJoints[i].token);
                    }
                    else
                    {
                        script->SetErrorFormat("Required joint \"%s\" not set.",
                            g_RigJoints[i].token);
                    }
                }
            }
            else
            {
                if ((joints[i].index < -1) ||
                    (joints[i].index >= skeleton->GetNumJoint()))
                {
                    script->SetErrorFormat(joints[i].lineNo,
                        "Joint \"%s\" out of range.",
                        g_RigJoints[i].token);
                }
            }
        }
    }
    
    if (script->IsError())
    {
        if (rig != 0)
        {
            rig->Release();
        }

        return script->HandleError();
    }
    
    // default motion root
    if (joints[RIG_JOINT_MOTION_ROOT].index < 0)
    {
        joints[RIG_JOINT_MOTION_ROOT].index = joints[RIG_JOINT_AI_ROOT].index;
    }
    
    // default orientation root
    if (joints[RIG_JOINT_ORIENTATION_ROOT].index < 0)
    {
        joints[RIG_JOINT_ORIENTATION_ROOT].index = joints[RIG_JOINT_AI_ROOT].index;
    }
    
    // set joints
    for (i = 0; i < RIG_JOINT_COUNT; ++i)
    {
        rig->SetJointIndex((RigJointIndex)i, joints[i].index);
    }
    
    // set attrs
    rig->SetBalanceRootMinYRate(balanceRootMinYRate);
    rig->SetBalanceRootYRateVelocityScale(balanceRootYRateVelocityScale);
    rig->SetMaxBalanceRootCompress(maxBalanceRootCompress);
    
    bank->Store(rig);
    rig->Release();
    return true;
}

#ifdef CHOREO_TOOL
bool Rig::WriteToScriptHandler(ScriptWriter* script, BaseBank* bank)
{
    CHOREO_SCRIPT_WRITE_HANDLER(Rig, script, bank);
}

bool Rig::WriteToScript(ScriptWriter* script) const
{
    script->BeginStruct(TOKEN_RIG);

    // required params
    script->RequiredParamString(TOKEN_NAME, GetName());
    script->RequiredParamString(TOKEN_SKELETON, m_Skeleton->GetName());
    
    // joints
    char tmp[256];
    int i;
    for (i = 0; i < RIG_JOINT_COUNT; ++i)
    {
        StrPrintf(tmp, sizeof(tmp), "jointIndex_%s", g_RigJoints[i].token);
        script->ParamFloat(tmp, (float)GetJointIndex((RigJointIndex)i));
    }

    // optional params
    script->ParamFloat(TOKEN_BALANCEROOTMINYRATE, m_BalanceRootMinYRate);
    script->ParamFloat(TOKEN_BALANCEROOTYRATEVELOCITYSCALE, m_BalanceRootYRateVelocityScale);
    script->ParamFloat(TOKEN_MAXBALANCEROOTCOMPRESS, m_MaxBalanceRootCompress);

    for (i = 0; i < m_LegCount; ++i)
    {
        m_Legs[i]->WriteToScript(script);
    }
    
    script->EndStruct();
    return true;
}
#endif // CHOREO_TOOL


} // namespace choreo


// End of file.
