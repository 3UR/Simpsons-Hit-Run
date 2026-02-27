/*
 * choreo/transition.cpp
 */

#include <choreo/transition.hpp>
#include <choreo/utility.hpp>
#include <choreo/scriptreader.hpp>
#include <choreo/scriptwriter.hpp>
#include <choreo/basebank.hpp>
#include <choreo/partition.hpp>
#include <choreo/load.hpp>

#include <p3d/anim/poseanimation.hpp>

#include <string.h>


namespace choreo
{


Transition::Transition(Rig* rig,
                       PURE3D_ANIMATION* anim,
                       Partition* partition):
    Animation(rig, anim, partition),
    
    m_CurrentMotionAngle(0.0f),
    m_CurrentMotionAngleArc(rmt::PI_2),
    
    m_DesiredMotionAngle(0.0f),
    m_DesiredMotionAngleArc(rmt::PI_2),
    
    m_DesiredFacingAngle(0.0f),
    m_DesiredFacingAngleArc(rmt::PI_2),
    
    m_MinCurrentVelocity(-1.0f),
    m_MaxCurrentVelocity(-1.0f),
    
    m_MinCurrentAcceleration(-1.0f),
    m_MaxCurrentAcceleration(-1.0f),
    
    m_MinDesiredVelocity(-1.0f),
    m_MaxDesiredVelocity(-1.0f)
{
}

Transition::~Transition()
{
}

inline bool Transition::MatchArc(float test, float angle, float arc)
{
    float smallestArc = GetSmallestArc(test, angle);
    return (rmt::Fabs(smallestArc) <= (arc * 0.5f));
}

inline bool Transition::MatchRange(float test, float min, float max)
{
	if ((min >= 0.0f) && (test < min))
		return false;
	if ((max >= 0.0f) && (test < max))
		return false;
	return true;
}

bool Transition::Match(float currentMotionAngle,
                       float currentFacingAngle,
                       float currentVelocity,
                       float currentAcceleration,
                       float desiredMotionAngle,
                       float desiredFacingAngle,
                       float desiredVelocity) const
{
    if (!MatchRange(currentAcceleration, m_MinCurrentAcceleration, m_MaxCurrentAcceleration))
        return false;
    
    if (!MatchRange(currentVelocity, m_MinCurrentVelocity, m_MaxCurrentVelocity))
        return false;
    
    if (!MatchRange(desiredVelocity, m_MinDesiredVelocity, m_MaxDesiredVelocity))
        return false;
    
    float relCurrentMotionAngle = GetSmallestArc(currentMotionAngle, currentFacingAngle);
    if (!MatchArc(relCurrentMotionAngle, m_CurrentMotionAngle, m_CurrentMotionAngleArc))
        return false;
    
    float relDesiredMotionAngle = GetSmallestArc(desiredMotionAngle, currentFacingAngle);
    float relMotionAngle = GetSmallestArc(relDesiredMotionAngle, relCurrentMotionAngle);
    if (!MatchArc(relMotionAngle, m_DesiredMotionAngle, m_DesiredMotionAngleArc))
        return false;
    
    float relDesiredFacingAngle = GetSmallestArc(desiredFacingAngle, currentFacingAngle);
    float relFacingAngle = GetSmallestArc(relDesiredFacingAngle, relCurrentMotionAngle);
    if (!MatchArc(relFacingAngle, m_DesiredFacingAngle, m_DesiredFacingAngleArc))
        return false;
    
    return true;
}

static const char* TOKEN_TRANSITION             = "transition";
static const char* TOKEN_NAME                   = "name";
static const char* TOKEN_ANIMATION              = "animation";
static const char* TOKEN_PARTITION              = "partition";
static const char* TOKEN_WEIGHT                 = "weight";
static const char* TOKEN_PRIORITY               = "priority";
static const char* TOKEN_ROOTBLENDINTIME        = "rootBlendInTime";
static const char* TOKEN_ROOTBLENDOUTTIME       = "rootBlendOutTime";
static const char* TOKEN_BLENDINTIME            = "blendInTime";
static const char* TOKEN_BLENDOUTTIME           = "blendOutTime";
static const char* TOKEN_CURRENTMOTIONANGLE     = "currentMotionAngle";
static const char* TOKEN_CURRENTMOTIONANGLEARC  = "currentMotionAngleArc";
static const char* TOKEN_DESIREDMOTIONANGLE     = "desiredMotionAngle";
static const char* TOKEN_DESIREDMOTIONANGLEARC  = "desiredMotionAngleArc";
static const char* TOKEN_DESIREDFACINGANGLE     = "desiredFacingAngle";
static const char* TOKEN_DESIREDFACINGANGLEARC  = "desiredFacingAngleArc";
static const char* TOKEN_MINCURRENTVELOCITY     = "minCurrentVelocity";
static const char* TOKEN_MAXCURRENTVELOCITY     = "maxCurrentVelocity";
static const char* TOKEN_MINCURRENTACCELERATION = "minCurrentAcceleration";
static const char* TOKEN_MAXCURRENTACCELERATION = "maxCurrentAcceleration";
static const char* TOKEN_MINDESIREDVELOCITY     = "minDesiredVelocity";
static const char* TOKEN_MAXDESIREDVELOCITY     = "maxDesiredVelocity";

bool Transition::ReadFromScript(ScriptReader* script, BaseBank* bank)
{
    if (!script->IsMoreStructs())
        return false;
    if (strcmp(script->GetNextStructIdentifier(), TOKEN_TRANSITION) != 0)
        return false;
    
    script->BeginStruct(TOKEN_TRANSITION);
    
    // required params
    char animName[256] = "";
    script->RequiredParamString(TOKEN_NAME, animName, sizeof(animName));
    
    // optional params
    char p3dAnimName[256] = "";
    script->ParamString(TOKEN_ANIMATION, p3dAnimName, sizeof(p3dAnimName));
    
    char partitionName[256] = "";
    script->ParamString(TOKEN_PARTITION, partitionName, sizeof(partitionName));
    
    float weight = 1.0f;
    script->ParamFloat(TOKEN_WEIGHT, &weight);
    
    float priority = 0.0f;
    script->ParamFloat(TOKEN_PRIORITY, &priority);
    
    float rootBlendInTime = 0.0f;
    script->ParamFloat(TOKEN_ROOTBLENDINTIME, &rootBlendInTime);
    
    float rootBlendOutTime = 0.0f;
    script->ParamFloat(TOKEN_ROOTBLENDOUTTIME, &rootBlendOutTime);
    
    float blendInTime = 0.0f;
    script->ParamFloat(TOKEN_BLENDINTIME, &blendInTime);
    
    float blendOutTime = 0.0f;
    script->ParamFloat(TOKEN_BLENDOUTTIME, &blendOutTime);
    
    float currentMotionAngle = 0.0f;
    script->ParamFloat(TOKEN_CURRENTMOTIONANGLE, &currentMotionAngle);
    
    float currentMotionAngleArc = 360.0f;
    script->ParamFloat(TOKEN_CURRENTMOTIONANGLEARC, &currentMotionAngleArc);
    
    float desiredMotionAngle = 0.0f;
    script->ParamFloat(TOKEN_DESIREDMOTIONANGLE, &desiredMotionAngle);
    
    float desiredMotionAngleArc = 360.0f;
    script->ParamFloat(TOKEN_DESIREDMOTIONANGLEARC, &desiredMotionAngleArc);
    
    float desiredFacingAngle = 0.0f;
    script->ParamFloat(TOKEN_DESIREDFACINGANGLE, &desiredFacingAngle);
    
    float desiredFacingAngleArc = 360.0f;
    script->ParamFloat(TOKEN_DESIREDFACINGANGLEARC, &desiredFacingAngleArc);
    
    float minCurrentVelocity = -1.0f;
    script->ParamFloat(TOKEN_MINCURRENTVELOCITY, &minCurrentVelocity);
    
    float maxCurrentVelocity = -1.0f;
    script->ParamFloat(TOKEN_MAXCURRENTVELOCITY, &maxCurrentVelocity);
    
    float minCurrentAcceleration = -1.0f;
    script->ParamFloat(TOKEN_MINCURRENTACCELERATION, &minCurrentAcceleration);
    
    float maxCurrentAcceleration = -1.0f;
    script->ParamFloat(TOKEN_MAXCURRENTACCELERATION, &maxCurrentAcceleration);
    
    float minDesiredVelocity = -1.0f;
    script->ParamFloat(TOKEN_MINDESIREDVELOCITY, &minDesiredVelocity);
    
    float maxDesiredVelocity = -1.0f;
    script->ParamFloat(TOKEN_MAXDESIREDVELOCITY, &maxDesiredVelocity);
    
    script->EndStruct();
    
    // resolve rig
    Rig* rig = bank->GetRig();
    if (rig == 0)
    {
        script->SetErrorString("Animation loaded outside of bank, cannot create without rig.");
    }
    
    // resolve pure3d animation
    char* p3dAnimSearchName;
    if (strlen(p3dAnimName) > 0)
    {
        p3dAnimSearchName = p3dAnimName;
    }
    else
    {
        p3dAnimSearchName = animName;
    }
    PURE3D_ANIMATION* p3dAnim = choreo::find<PURE3D_ANIMATION>(bank, p3dAnimSearchName);
    if (p3dAnim == 0)
    {
        script->SetErrorFormat("Could not find Pure3D animation named \"%s\".", p3dAnimSearchName);
    }
    
    // resolve partition (if any)
    Partition* partition = 0;
    if (strlen(partitionName) > 0)
    {
        partition = choreo::find<Partition>(bank, partitionName);
        if (partition == 0)
        {
            script->SetErrorFormat("Could not find partition named \"%s\".", partitionName);
        }
    }
    
    if (script->IsError())
    {
        return script->HandleError();
    }
    
    Transition* trans = new Transition(rig, p3dAnim, partition);
    trans->AddRef();
    trans->SetName(animName);
    trans->SetWeight(weight);
    trans->SetPriority((int)priority);
    trans->SetIsCyclic(false);
    trans->SetRootBlendInTime(rootBlendInTime);
    trans->SetRootBlendOutTime(rootBlendOutTime);
    trans->SetBlendInTime(blendInTime);
    trans->SetBlendOutTime(blendOutTime);
    trans->SetCurrentMotionAngle(rmt::DegToRadian(currentMotionAngle));
    trans->SetCurrentMotionAngleArc(rmt::DegToRadian(currentMotionAngleArc));
    trans->SetDesiredMotionAngle(rmt::DegToRadian(desiredMotionAngle));
    trans->SetDesiredMotionAngleArc(rmt::DegToRadian(desiredMotionAngleArc));
    trans->SetDesiredFacingAngle(rmt::DegToRadian(desiredFacingAngle));
    trans->SetDesiredFacingAngleArc(rmt::DegToRadian(desiredFacingAngleArc));
    trans->SetMinCurrentVelocity(minCurrentVelocity);
    trans->SetMaxCurrentVelocity(maxCurrentVelocity);
    trans->SetMinCurrentAcceleration(minCurrentAcceleration);
    trans->SetMaxCurrentAcceleration(maxCurrentAcceleration);
    trans->SetMinDesiredVelocity(minDesiredVelocity);
    trans->SetMaxDesiredVelocity(maxDesiredVelocity);
    
    bank->Store(trans);
    
    trans->Release();
    return true;
}

#ifdef CHOREO_TOOL
bool Transition::WriteToScriptHandler(ScriptWriter* script, BaseBank* bank)
{
    CHOREO_SCRIPT_WRITE_HANDLER(Transition, script, bank);
}

bool Transition::WriteToScript(ScriptWriter* script) const
{
    script->BeginStruct(TOKEN_TRANSITION);

    // required params
    script->RequiredParamString(TOKEN_NAME, GetName());
    
    // optional params
    script->ParamString(TOKEN_ANIMATION, GetP3DAnimation()->GetName());

	if (GetPartition() != 0)
	{
		script->ParamString(TOKEN_PARTITION, GetPartition()->GetName());
	}

    script->ParamFloat(TOKEN_WEIGHT, GetWeight());
    script->ParamFloat(TOKEN_PRIORITY, (float)GetPriority());
    script->ParamFloat(TOKEN_ROOTBLENDINTIME, GetRootBlendInTime());
    script->ParamFloat(TOKEN_ROOTBLENDOUTTIME, GetRootBlendOutTime());
    script->ParamFloat(TOKEN_BLENDINTIME, GetBlendInTime());
    script->ParamFloat(TOKEN_BLENDOUTTIME, GetBlendOutTime());

    script->ParamFloat(TOKEN_CURRENTMOTIONANGLE, rmt::RadianToDeg(m_CurrentMotionAngle));
    script->ParamFloat(TOKEN_CURRENTMOTIONANGLEARC, rmt::RadianToDeg(m_CurrentMotionAngleArc));
    script->ParamFloat(TOKEN_DESIREDMOTIONANGLE, rmt::RadianToDeg(m_DesiredMotionAngle));
    script->ParamFloat(TOKEN_DESIREDMOTIONANGLEARC, rmt::RadianToDeg(m_DesiredMotionAngleArc));
    script->ParamFloat(TOKEN_DESIREDFACINGANGLE, rmt::RadianToDeg(m_DesiredFacingAngle));
    script->ParamFloat(TOKEN_DESIREDFACINGANGLEARC, rmt::RadianToDeg(m_DesiredFacingAngleArc));
    script->ParamFloat(TOKEN_MINCURRENTVELOCITY, m_MinCurrentVelocity);
    script->ParamFloat(TOKEN_MAXCURRENTVELOCITY, m_MaxCurrentVelocity);
    script->ParamFloat(TOKEN_MINCURRENTACCELERATION, m_MinCurrentAcceleration);
    script->ParamFloat(TOKEN_MAXCURRENTACCELERATION, m_MaxCurrentAcceleration);
    script->ParamFloat(TOKEN_MINDESIREDVELOCITY, m_MinDesiredVelocity);
    script->ParamFloat(TOKEN_MAXDESIREDVELOCITY, m_MaxDesiredVelocity);
    
    script->EndStruct();
	return true;
}
#endif // CHOREO_TOOL


} // namespace choreo


// End of file.
