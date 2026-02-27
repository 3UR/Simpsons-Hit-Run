/*
 * choreo/animation.cpp
 */

#include <choreo/animation.hpp>
#include <choreo/rig.hpp>
#include <choreo/partition.hpp>
#include <choreo/scriptreader.hpp>
#include <choreo/scriptwriter.hpp>
#include <choreo/basebank.hpp>
#include <choreo/rootblenddriver.hpp>
#include <choreo/jointblenddriver.hpp>
#include <choreo/footblenddriver.hpp>
#include <choreo/utility.hpp>
#include <choreo/engine.hpp>
#include <choreo/rootblender.hpp>
#include <choreo/jointblender.hpp>
#include <choreo/puppet.hpp>
#include <choreo/footblender.hpp>
#include <choreo/load.hpp>

#include <string.h>


namespace choreo
{


//---------------------------------------------------------------------------
// class Animation
//---------------------------------------------------------------------------

Animation::Animation(Rig* rig,
                     PURE3D_ANIMATION* anim,
                     Partition* partition,
                     GroundFixupEnum groundFixupEnum):
    m_Animation(rig->GetSkeleton(), anim),
    m_Weight(1.0f),
    m_Priority(0),
    m_GroundFixupEnum(groundFixupEnum),
    m_Duration(-1.0f),
    m_Speed(1.0f),
    m_StartFrame(0.0f),
    m_EndFrame(-1.0f),
    m_RootBlendInTime(0.0f),
    m_RootBlendOutTime(0.0f),
    m_BlendInTime(0.0f),
    m_BlendOutTime(0.0f)
{
    P3DASSERT(rig != 0);
    m_Rig = rig;
    m_Rig->AddRef();
    
    m_Partition = partition;
    if (m_Partition != 0)
    {
        m_Partition->AddRef();
    }
    
    m_IsCyclic = m_Animation.IsCyclic();

    if (m_GroundFixupEnum == GROUND_FIXUP_ANIMATION_START)
    {
        int motionRootIndex = m_Rig->GetJointIndex(RIG_JOINT_MOTION_ROOT);
        if (m_Animation.HasJointTranslation(motionRootIndex))
        {
            int frameCount = (int)GetFrameCount();
            m_GroundFixupFrames = new float [frameCount];

            if (frameCount > 0)
            {
                m_GroundFixupFrames[0] = 1.0f;

                rmt::Vector origRoot;
                m_Animation.GetJointTranslation(motionRootIndex, 0.0f, origRoot);

                bool prev0 = false;
                for (int i = 1; i < frameCount; ++i)
                {
                    rmt::Vector curRoot;
                    m_Animation.GetJointTranslation(motionRootIndex, (float)i, curRoot);

                    if (!rmt::Epsilon(curRoot.y, origRoot.y))
                    {
                        // groundFixup = 0

                        m_GroundFixupFrames[i] = 0.0f;

                        if (m_GroundFixupFrames[i - 1] != 0.0f)
                        {
                            m_GroundFixupFrames[i - 1] = 0.0f;
                        }

                        prev0 = true;
                    }
                    else
                    {
                        // groundFixup = 1

                        if (prev0)
                        {
                            m_GroundFixupFrames[i] = 0.0f;
                        }
                        else
                        {
                            m_GroundFixupFrames[i] = 1.0f;
                        }

                        prev0 = false;
                    }
                }
            }
        }
        else
        {
            m_GroundFixupFrames = 0;
        }
    }
    else if (m_GroundFixupEnum == GROUND_FIXUP_ANIMATION_RUN)
    {
        int motionRootIndex = m_Rig->GetJointIndex(RIG_JOINT_MOTION_ROOT);
        if (m_Animation.HasJointTranslation(motionRootIndex))
        {
            int frameCount = (int)GetFrameCount();
            m_GroundFixupFrames = new float [frameCount];

            if (frameCount > 0)
            {
                m_GroundFixupFrames[0] = 1.0f;

                rmt::Vector prevRoot;
                m_Animation.GetJointTranslation(motionRootIndex, 0.0f, prevRoot);

                bool prev0 = false;
                for (int i = 1; i < frameCount; ++i)
                {
                    rmt::Vector curRoot;
                    m_Animation.GetJointTranslation(motionRootIndex, (float)i, curRoot);

                    if (!rmt::Epsilon(curRoot.y, prevRoot.y))
                    {
                        // groundFixup = 0

                        m_GroundFixupFrames[i] = 0.0f;

                        if (m_GroundFixupFrames[i - 1] != 0.0f)
                        {
                            m_GroundFixupFrames[i - 1] = 0.0f;
                        }

                        prev0 = true;
                    }
                    else
                    {
                        // groundFixup = 1

                        if (prev0)
                        {
                            m_GroundFixupFrames[i] = 0.0f;
                        }
                        else
                        {
                            m_GroundFixupFrames[i] = 1.0f;
                        }

                        prev0 = false;
                    }

                    prevRoot = curRoot;
                }
            }
        }
        else
        {
            m_GroundFixupFrames = 0;
        }
    }
    else
    {
        m_GroundFixupFrames = 0;
    }
}

Animation::~Animation()
{
    delete[] m_GroundFixupFrames;

    if (m_Partition != 0)
    {
        m_Partition->Release();
    }
    
    m_Rig->Release();
}

void Animation::SetPartition(Partition* partition)
{
    tRefCounted::Assign(m_Partition, partition);
}

float Animation::GetGroundFixup(float frame) const
{
    P3DASSERT((frame >= 0.0f) && (frame <= GetFrameCount() - 1.0f));

    switch (m_GroundFixupEnum)
    {
        case GROUND_FIXUP_ALWAYS:
            return 1.0f;

        case GROUND_FIXUP_NEVER:
            return 0.0f;

        default:
            break;
    }

    if (m_GroundFixupFrames == 0)
    {
        return 1.0f;
    }

    int baseFrame = (int)frame;

    if (baseFrame >= ((int)GetFrameCount() - 1))
    {
        return m_GroundFixupFrames[baseFrame];
    }
    else
    {
        float alpha = frame - baseFrame;

        return BlendFloat(m_GroundFixupFrames[baseFrame],
                          m_GroundFixupFrames[baseFrame + 1],
                          alpha);
    }
}

float Animation::MakeValidFrame(float frame, float startFrame, float endFrame, bool cyclic)
{
    // wrap
    if (cyclic)
    {
        return Wrap(startFrame, endFrame, frame);
    }
    
    // clamp
    if (frame < startFrame)
        return startFrame;
    if (frame > endFrame)
        return endFrame;
    
    return frame;
}

PuppetDriver* Animation::NewDriver()
{
    return NewAnimationDriver();
}

AnimationDriver* Animation::NewAnimationDriver()
{
    return new AnimationDriver(this);
}

static const char* TOKEN_ANIMATION        = "animation";
static const char* TOKEN_NAME             = "name";
static const char* TOKEN_PARTITION        = "partition";
static const char* TOKEN_WEIGHT           = "weight";
static const char* TOKEN_PRIORITY         = "priority";
static const char* TOKEN_ISCYCLIC         = "isCyclic";
static const char* TOKEN_DURATION         = "duration";
static const char* TOKEN_SPEED            = "speed";
static const char* TOKEN_STARTFRAME       = "startFrame";
static const char* TOKEN_ENDFRAME         = "endFrame";
static const char* TOKEN_ROOTBLENDINTIME  = "rootBlendInTime";
static const char* TOKEN_ROOTBLENDOUTTIME = "rootBlendOutTime";
static const char* TOKEN_BLENDINTIME      = "blendInTime";
static const char* TOKEN_BLENDOUTTIME     = "blendOutTime";
static const char* TOKEN_GROUNDFIXUP      = "groundFixup";

bool Animation::ReadFromScript(ScriptReader* script,
                               BaseBank* bank)
{
    if (!script->IsMoreStructs())
        return false;
    if (strcmp(script->GetNextStructIdentifier(), TOKEN_ANIMATION) != 0)
        return false;
    
    script->BeginStruct(TOKEN_ANIMATION);
    
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
    
    float isCyclic = 0.0f;
    script->ParamFloat(TOKEN_ISCYCLIC, &isCyclic);
    
    float duration = -1.0f;
    script->ParamFloat(TOKEN_DURATION, &duration);
    
    float speed = 1.0f;
    script->ParamFloat(TOKEN_SPEED, &speed);
    
    float startFrame = 0.0f;
    script->ParamFloat(TOKEN_STARTFRAME, &startFrame);
    
    float endFrame = -1.0f;
    script->ParamFloat(TOKEN_ENDFRAME, &endFrame);
    
    float rootBlendInTime = 0.0f;
    script->ParamFloat(TOKEN_ROOTBLENDINTIME, &rootBlendInTime);
    
    float rootBlendOutTime = 0.0f;
    script->ParamFloat(TOKEN_ROOTBLENDOUTTIME, &rootBlendOutTime);
    
    float blendInTime = 0.0f;
    script->ParamFloat(TOKEN_BLENDINTIME, &blendInTime);
    
    float blendOutTime = 0.0f;
    script->ParamFloat(TOKEN_BLENDOUTTIME, &blendOutTime);

    float groundFixupf = 1.0f;
    int lineNo;
    script->ParamFloat(TOKEN_GROUNDFIXUP, &groundFixupf, &lineNo);
    
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

    // resolve ground fixup
    int groundFixup = (int)groundFixupf + 1;
    if ((groundFixup < 0) || (groundFixup >= 4))
    {
        script->SetErrorFormat(lineNo, "Invalid groundFixup value; must be one of -1, 0, 1 or 2.");
    }
    
    if (script->IsError())
    {
        return script->HandleError();
    }
    
    Animation* anim = new Animation(rig, p3dAnim, partition, (GroundFixupEnum)groundFixup);
    anim->AddRef();
    anim->SetName(animName);
    anim->SetWeight(weight);
    anim->SetPriority((int)priority);
    anim->SetIsCyclic(isCyclic != 0.0f);
    anim->SetDuration(duration);
    anim->SetSpeed(speed);
    anim->SetStartFrame(startFrame);
    anim->SetEndFrame(endFrame);
    anim->SetRootBlendInTime(rootBlendInTime);
    anim->SetRootBlendOutTime(rootBlendOutTime);
    anim->SetBlendInTime(blendInTime);
    anim->SetBlendOutTime(blendOutTime);
    
    bank->Store(anim);
    
    anim->Release();
    return true;
}

#ifdef CHOREO_TOOL
bool Animation::WriteToScriptHandler(ScriptWriter* script, BaseBank* bank)
{
    CHOREO_SCRIPT_WRITE_HANDLER(Animation, script, bank);
}

bool Animation::WriteToScript(ScriptWriter* script) const
{
    script->BeginStruct(TOKEN_ANIMATION);
    
    // required params
    script->RequiredParamString(TOKEN_NAME, GetName());
    
    // optional params
    script->ParamString(TOKEN_ANIMATION, m_Animation.GetName());

	if (m_Partition != 0)
	{
		script->ParamString(TOKEN_PARTITION, m_Partition->GetName());
	}
    
    script->ParamFloat(TOKEN_WEIGHT, m_Weight);
    script->ParamFloat(TOKEN_PRIORITY, (float)m_Priority);
    script->ParamFloat(TOKEN_ISCYCLIC, (float)m_IsCyclic);
    script->ParamFloat(TOKEN_ROOTBLENDINTIME, m_RootBlendInTime);
    script->ParamFloat(TOKEN_ROOTBLENDOUTTIME, m_RootBlendOutTime);
    script->ParamFloat(TOKEN_BLENDINTIME, m_BlendInTime);
    script->ParamFloat(TOKEN_BLENDOUTTIME, m_BlendOutTime);
    script->ParamFloat(TOKEN_GROUNDFIXUP, (float)((int)m_GroundFixupEnum - 1));

    script->EndStruct();
	return true;
}
#endif // CHOREO_TOOL


//---------------------------------------------------------------------------
// class AnimationFrame - interface
//---------------------------------------------------------------------------

class AnimationFrame: public tRefCounted
{
public:
    
    AnimationFrame(Animation* anim);

    // partition, weight & priority -- overrides those in Animation
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
    
    // blend times -- overrides those in Animation
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

    // frame information
    float GetPreviousFrame() const
        { return m_PreviousFrame; }
    void SetPreviousFrame(float previousFrame)
        { m_PreviousFrame = previousFrame; }
    
    float GetCurrentFrame() const
        { return m_CurrentFrame; }
    void SetCurrentFrame(float currentFrame)
        { m_CurrentFrame = currentFrame; }
    
    // joint accessors
    int GetJointPriority(int jointIndex) const;
    float GetJointWeight(int jointIndex) const;
    
    bool HasJointTranslation(int jointIndex) const;
    bool HasJointRotation(int jointIndex) const;

    // group accessors
    int GetGroupPriority() const;
    float GetGroupWeight() const;
    
protected:
    
    virtual ~AnimationFrame();
    
private:

    Animation* m_Animation;
    
    Partition* m_Partition;
    float m_Weight;
    int m_Priority;

    float m_RootBlendInTime;
    float m_RootBlendOutTime;
    float m_BlendInTime;
    float m_BlendOutTime;

    float m_PreviousFrame;
    float m_CurrentFrame;
};


//---------------------------------------------------------------------------
// class AnimationFrame - implementation
//---------------------------------------------------------------------------

AnimationFrame::AnimationFrame(Animation* anim):
    m_Weight(1.0f),
    m_Priority(0),

    m_PreviousFrame(0.0f),
    m_CurrentFrame(0.0f)
{
    P3DASSERT(anim != 0);
    m_Animation = anim;
    m_Animation->AddRef();

    m_Partition = m_Animation->GetPartition();
    if (m_Partition != 0)
    {
        m_Partition->AddRef();
    }

    m_RootBlendInTime = m_Animation->GetRootBlendInTime();
    m_RootBlendOutTime = m_Animation->GetRootBlendOutTime();

    m_BlendInTime = m_Animation->GetBlendInTime();
    m_BlendOutTime = m_Animation->GetBlendOutTime();
}

AnimationFrame::~AnimationFrame()
{
    if (m_Partition != 0)
    {
        m_Partition->Release();
    }

    m_Animation->Release();
}

float AnimationFrame::GetJointWeight(int jointIndex) const
{
    float weight = GetWeight();
    weight *= m_Animation->GetWeight();

    if (m_Partition != 0)
    {
        weight *= m_Partition->GetJointWeight(jointIndex);
    }

    return weight;
}

int AnimationFrame::GetJointPriority(int jointIndex) const
{
    int priority = GetPriority();
    priority += m_Animation->GetPriority();

    if (m_Partition != 0)
    {
        priority += m_Partition->GetJointPriority(jointIndex);
    }

    return priority;
}

bool AnimationFrame::HasJointTranslation(int jointIndex) const
{
    if (!m_Animation->HasJointTranslation(jointIndex))
        return false;
    if (m_Partition == 0)
        return true;
    return m_Partition->HasJoint(jointIndex);
}

bool AnimationFrame::HasJointRotation(int jointIndex) const
{
    if (!m_Animation->HasJointRotation(jointIndex))
        return false;
    if (m_Partition == 0)
        return true;
    return m_Partition->HasJoint(jointIndex);
}

int AnimationFrame::GetGroupPriority() const
{
    int priority = GetPriority();
    priority += m_Animation->GetPriority();

    return priority;
}

float AnimationFrame::GetGroupWeight() const
{
    float weight = GetWeight();
    weight *= m_Animation->GetWeight();

    return weight;
}


//---------------------------------------------------------------------------
// class AnimationRootDriver - interface
//   - internal to the AnimationDriver
//---------------------------------------------------------------------------

class AnimationRootDriver: public RootBlendDriver
{
public:
    
    AnimationRootDriver(Animation* anim,
                        AnimationFrame* animFrame);
    
    float GetPreviousFrame() const
        { return m_AnimationFrame->GetPreviousFrame(); }
    float GetCurrentFrame() const
        { return m_AnimationFrame->GetCurrentFrame(); }
    
    virtual void Reset(RootBlender* rootBlender);
    
    virtual float GetRootBlendInTime() const;
    virtual float GetRootBlendOutTime() const;
    
    virtual float GetRootWeight() const;
    virtual int GetRootPriority() const;
    
    virtual bool HasRootTranslation() const;
    void SetHasRootTranslation(bool isEnabled)
        { m_HasRootTranslation = isEnabled; }
    virtual void GetRootTranslation(const poser::Transform& oldTransform,
                                    rmt::Vector& newTranslation) const;
    
    virtual bool HasRootRotation() const;
    virtual void GetRootRotation(const poser::Transform& oldTransform,
                                 rmt::Quaternion& newRotation) const;
    
    virtual bool HasRootGroundFixup() const;
    virtual float GetRootGroundFixup() const;
    
protected:
    
    virtual ~AnimationRootDriver();
    
private:
    
    Animation* m_Animation;
    AnimationFrame* m_AnimationFrame;
    bool m_HasRootTranslation;
};


//---------------------------------------------------------------------------
// class AnimationRootDriver - implementation
//---------------------------------------------------------------------------

AnimationRootDriver::AnimationRootDriver(Animation* anim,
                                         AnimationFrame* animFrame):
    m_HasRootTranslation(true)
{
    P3DASSERT(anim != 0);
    m_Animation = anim;
    m_Animation->AddRef();
    
    P3DASSERT(animFrame != 0);
    m_AnimationFrame = animFrame;
    m_AnimationFrame->AddRef();
}

AnimationRootDriver::~AnimationRootDriver()
{
    m_AnimationFrame->Release();
    m_Animation->Release();
}

void AnimationRootDriver::Reset(RootBlender* rootBlender)
{
    // do nothing
}

float AnimationRootDriver::GetRootBlendInTime() const
{
    return m_AnimationFrame->GetRootBlendInTime();
}

float AnimationRootDriver::GetRootBlendOutTime() const
{
    return m_AnimationFrame->GetRootBlendOutTime();
}

float AnimationRootDriver::GetRootWeight() const
{
    int aiRootIndex = m_Animation->GetRig()->GetJointIndex(RIG_JOINT_AI_ROOT);
    return m_AnimationFrame->GetJointWeight(aiRootIndex);
}

int AnimationRootDriver::GetRootPriority() const
{
    int aiRootIndex = m_Animation->GetRig()->GetJointIndex(RIG_JOINT_AI_ROOT);
    return m_AnimationFrame->GetJointPriority(aiRootIndex);
}

bool AnimationRootDriver::HasRootTranslation() const
{
    if (!m_HasRootTranslation)
        return false;
    int motionRootIndex = m_Animation->GetRig()->GetJointIndex(RIG_JOINT_MOTION_ROOT);
    return m_AnimationFrame->HasJointTranslation(motionRootIndex);
}

void AnimationRootDriver::GetRootTranslation(const poser::Transform& oldTransform,
                                             rmt::Vector& newTranslation) const
{
    // FIXME
    // Should handle case where deltaTime is HUGE and wraps
    // the animation multiple times...
    
    int motionRootIndex = m_Animation->GetRig()->GetJointIndex(RIG_JOINT_MOTION_ROOT);
    int orientRootIndex = m_Animation->GetRig()->GetJointIndex(RIG_JOINT_ORIENTATION_ROOT);
    
    rmt::Vector tmp;
    rmt::Vector delta(0.0f, 0.0f, 0.0f);
    
    if (GetPreviousFrame() >
        GetCurrentFrame())
    {
        m_Animation->GetJointTranslation(motionRootIndex,
                                         GetPreviousFrame(),
                                         tmp);
        delta.Sub(tmp);
        
        m_Animation->GetJointTranslation(motionRootIndex,
                                         m_Animation->GetFrameCount() - 1.0f,
                                         tmp);
        delta.Add(tmp);
        
        m_Animation->GetJointTranslation(motionRootIndex,
                                         0.0f,
                                         tmp);
        delta.Sub(tmp);
        
        m_Animation->GetJointTranslation(motionRootIndex,
                                         GetCurrentFrame(),
                                         tmp);
        delta.Add(tmp);
    }
    else
    {
        m_Animation->GetJointTranslation(motionRootIndex,
                                         GetPreviousFrame(),
                                         tmp);
        delta.Sub(tmp);
        
        m_Animation->GetJointTranslation(motionRootIndex,
                                         GetCurrentFrame(),
                                         tmp);
        delta.Add(tmp);
    }
    
    rmt::Matrix initOrient;

    if (m_AnimationFrame->HasJointRotation(orientRootIndex))
    {
        rmt::Quaternion initRootOrient = oldTransform.GetQuaternion();
    
        rmt::Quaternion initAnimOrient;
        m_Animation->GetJointRotation(orientRootIndex,
                                      GetPreviousFrame(),
                                      initAnimOrient);
    
        initAnimOrient.Inverse();
        initRootOrient.Mult(initAnimOrient);

        initRootOrient.ConvertToMatrix(&initOrient);
    }
    else
    {
        initOrient = oldTransform.GetMatrix();
    }

    rmt::Vector rotatedDelta;
    initOrient.RotateVector(delta, &rotatedDelta);
    
    newTranslation = oldTransform.GetTranslation();
    newTranslation.Add(rotatedDelta);
}

bool AnimationRootDriver::HasRootRotation() const
{
    int orientRootIndex = m_Animation->GetRig()->GetJointIndex(RIG_JOINT_ORIENTATION_ROOT);
    return m_AnimationFrame->HasJointRotation(orientRootIndex);
}

void AnimationRootDriver::GetRootRotation(const poser::Transform& oldTransform,
                                          rmt::Quaternion& newRotation) const
{
    // FIXME
    // Should handle case where deltaTime is HUGE and wraps
    // the animation multiple times...
    
    int orientRootIndex = m_Animation->GetRig()->GetJointIndex(RIG_JOINT_ORIENTATION_ROOT);
    
    rmt::Quaternion tmp;
    
    newRotation = oldTransform.GetQuaternion();
    
    if (GetPreviousFrame() >
        GetCurrentFrame())
    {
        m_Animation->GetJointRotation(orientRootIndex,
                                      GetPreviousFrame(),
                                      tmp);
        tmp.Inverse();
        newRotation.Mult(tmp);
        
        m_Animation->GetJointRotation(orientRootIndex,
                                      m_Animation->GetFrameCount() - 1.0f,
                                      tmp);
        newRotation.Mult(tmp);
        
        m_Animation->GetJointRotation(orientRootIndex,
                                      0.0f,
                                      tmp);
        tmp.Inverse();
        newRotation.Mult(tmp);
        
        m_Animation->GetJointRotation(orientRootIndex,
                                      GetCurrentFrame(),
                                      tmp);
        newRotation.Mult(tmp);
    }
    else
    {
        m_Animation->GetJointRotation(orientRootIndex,
                                      GetPreviousFrame(),
                                      tmp);
        tmp.Inverse();
        newRotation.Mult(tmp);
        
        m_Animation->GetJointRotation(orientRootIndex,
                                      GetCurrentFrame(),
                                      tmp);
        newRotation.Mult(tmp);
    }

    newRotation.Normalize();
}

bool AnimationRootDriver::HasRootGroundFixup() const
{
    int motionRootIndex = m_Animation->GetRig()->GetJointIndex(RIG_JOINT_MOTION_ROOT);
    return m_AnimationFrame->HasJointTranslation(motionRootIndex);
}

float AnimationRootDriver::GetRootGroundFixup() const
{
    return m_Animation->GetGroundFixup(GetCurrentFrame());
}


//---------------------------------------------------------------------------
// class AnimationJointDriver - interface
//   - internal to the AnimationDriver
//---------------------------------------------------------------------------

class AnimationJointDriver: public JointBlendDriver
{
public:
    
    AnimationJointDriver(Animation* anim,
                         AnimationFrame* animFrame);
    
    float GetFrame() const
        { return m_AnimationFrame->GetCurrentFrame(); }
    
    virtual void Reset();
    virtual void Advance(float deltaTime);
    
    virtual float GetBlendInTime() const;
    virtual float GetBlendOutTime() const;
    
    virtual float GetJointWeight(int jointIndex) const;
    virtual int GetJointPriority(int jointIndex) const;
    
    virtual bool HasJointTranslation(int jointIndex) const;
    virtual void GetJointTranslation(int jointIndex, rmt::Vector& translation) const;
    
    virtual bool HasJointRotation(int jointIndex) const;
    virtual void GetJointRotation(int jointIndex, rmt::Quaternion& rotation) const;
    
    virtual float GetGroupWeight(tUID group) const;
    virtual int GetGroupPriority(tUID group) const;
    
    virtual bool HasGroupTranslation(tUID group) const;
    virtual void GetGroupTranslation(tUID group, rmt::Vector& translation) const;
    
    virtual bool HasGroupRotation(tUID group) const;
    virtual void GetGroupRotation(tUID group, rmt::Quaternion& rotation) const;

protected:
    
    virtual ~AnimationJointDriver();
    
private:
    
    Animation* m_Animation;
    AnimationFrame* m_AnimationFrame;
};


//---------------------------------------------------------------------------
// class AnimationJointDriver - implementation
//---------------------------------------------------------------------------

AnimationJointDriver::AnimationJointDriver(Animation* anim,
                                           AnimationFrame* animFrame)
{
    P3DASSERT(anim != 0);
    m_Animation = anim;
    m_Animation->AddRef();
    
    P3DASSERT(animFrame != 0);
    m_AnimationFrame = animFrame;
    m_AnimationFrame->AddRef();
}

AnimationJointDriver::~AnimationJointDriver()
{
    m_AnimationFrame->Release();
    m_Animation->Release();
}

void AnimationJointDriver::Reset()
{
    // do nothing
}

void AnimationJointDriver::Advance(float deltaTime)
{
    // do nothing
}

float AnimationJointDriver::GetBlendInTime() const
{
    return m_AnimationFrame->GetBlendInTime();
}

float AnimationJointDriver::GetBlendOutTime() const
{
    return m_AnimationFrame->GetBlendOutTime();
}

float AnimationJointDriver::GetJointWeight(int jointIndex) const
{
    return m_AnimationFrame->GetJointWeight(jointIndex);
}

int AnimationJointDriver::GetJointPriority(int jointIndex) const
{
    return m_AnimationFrame->GetJointPriority(jointIndex);
}

bool AnimationJointDriver::HasJointTranslation(int jointIndex) const
{
    return m_AnimationFrame->HasJointTranslation(jointIndex);
}

void AnimationJointDriver::GetJointTranslation(int jointIndex, rmt::Vector& translation) const
{
    m_Animation->GetJointTranslation(jointIndex, GetFrame(), translation);
}

bool AnimationJointDriver::HasJointRotation(int jointIndex) const
{
    return m_AnimationFrame->HasJointRotation(jointIndex);
}

void AnimationJointDriver::GetJointRotation(int jointIndex, rmt::Quaternion& rotation) const
{
    m_Animation->GetJointRotation(jointIndex, GetFrame(), rotation);

    // FIXME:  removed -- only needed for max?
    /*// check to see if we need to do character root fixup
    Rig* rig = m_Animation->GetRig();
    int cRootIndex = rig->GetJointIndex(RIG_JOINT_CHARACTER_ROOT);
    
    if (jointIndex == cRootIndex)
    {
        int motionRootIndex = rig->GetJointIndex(RIG_JOINT_MOTION_ROOT);
        int orientRootIndex = rig->GetJointIndex(RIG_JOINT_ORIENTATION_ROOT);
        
        if (motionRootIndex != orientRootIndex)
        {
            rmt::Quaternion orientRot;
            m_Animation->GetJointRotation(orientRootIndex, GetFrame(), orientRot);
            orientRot.Inverse();
            
            orientRot.Mult(rotation);
            rotation = orientRot;
        }
    }*/
}

float AnimationJointDriver::GetGroupWeight(tUID group) const
{
    return m_AnimationFrame->GetGroupWeight();
}

int AnimationJointDriver::GetGroupPriority(tUID group) const
{
    return m_AnimationFrame->GetGroupPriority();
}

bool AnimationJointDriver::HasGroupTranslation(tUID group) const
{
    return m_Animation->HasGroupTranslation(group);
}

void AnimationJointDriver::GetGroupTranslation(tUID group, rmt::Vector& translation) const
{
    m_Animation->GetGroupTranslation(group, GetFrame(), translation);
}

bool AnimationJointDriver::HasGroupRotation(tUID group) const
{
    return m_Animation->HasGroupRotation(group);
}

void AnimationJointDriver::GetGroupRotation(tUID group, rmt::Quaternion& rotation) const
{
    m_Animation->GetGroupRotation(group, GetFrame(), rotation);
}


//---------------------------------------------------------------------------
// class AnimationFootDriver - interface
//---------------------------------------------------------------------------

class AnimationFootDriver: public FootBlendDriver
{
public:
    
    AnimationFootDriver(RigLeg* rigLeg,
                        Animation* anim,
                        AnimationFrame* animFrame);
    
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
    
    virtual ~AnimationFootDriver();
    
private:
    
    RigLeg* m_RigLeg;
    Animation* m_Animation;
    AnimationFrame* m_AnimationFrame;
};


//---------------------------------------------------------------------------
// class AnimationFootDriver - implementation
//---------------------------------------------------------------------------

AnimationFootDriver::AnimationFootDriver(RigLeg* rigLeg,
                                         Animation* anim,
                                         AnimationFrame* animFrame)
{
    P3DASSERT(rigLeg != 0);
    m_RigLeg = rigLeg;
    m_RigLeg->AddRef();
    
    P3DASSERT(anim != 0);
    m_Animation = anim;
    m_Animation->AddRef();
    
    P3DASSERT(animFrame != 0);
    m_AnimationFrame = animFrame;
    m_AnimationFrame->AddRef();
}

AnimationFootDriver::~AnimationFootDriver()
{
    m_AnimationFrame->Release();
    m_Animation->Release();
    m_RigLeg->Release();
}

float AnimationFootDriver::GetFootBlendInTime() const
{
    return m_AnimationFrame->GetBlendInTime();
}

float AnimationFootDriver::GetFootBlendOutTime() const
{
    return m_AnimationFrame->GetBlendOutTime();
}

float AnimationFootDriver::GetFootWeight() const
{
    return m_AnimationFrame->GetJointWeight(m_RigLeg->GetAnkleJointIndex());
}

int AnimationFootDriver::GetFootPriority() const
{
    return m_AnimationFrame->GetJointPriority(m_RigLeg->GetAnkleJointIndex());
}

bool AnimationFootDriver::HasFootEffectorTranslation() const
{
    return m_AnimationFrame->HasJointTranslation(m_RigLeg->GetEffectorJointIndex());
}

void AnimationFootDriver::GetFootEffectorTranslation(rmt::Vector& effTranslate) const
{
    // get effector transform
    int effectorIndex = m_RigLeg->GetEffectorJointIndex();
    float frame = m_AnimationFrame->GetCurrentFrame();
    
    m_Animation->GetJointTranslation(effectorIndex, frame, effTranslate);
    
    // check if we have to do a motion root fixup
    Rig* rig = m_Animation->GetRig();
    int aiRootIndex = rig->GetJointIndex(RIG_JOINT_AI_ROOT);
    int motionRootIndex = rig->GetJointIndex(RIG_JOINT_MOTION_ROOT);
    
    if ((aiRootIndex < effectorIndex) &&
        (motionRootIndex > effectorIndex))
    {
        rmt::Vector mrootTranslate;
        m_Animation->GetJointTranslation(motionRootIndex, frame, mrootTranslate);
        effTranslate.Sub(mrootTranslate);
    }
}

bool AnimationFootDriver::HasFootEffectorRotation() const
{
    return m_AnimationFrame->HasJointRotation(m_RigLeg->GetEffectorJointIndex());
}

void AnimationFootDriver::GetFootEffectorRotation(rmt::Quaternion& effRotate) const
{
    // get effector transform
    int effectorIndex = m_RigLeg->GetEffectorJointIndex();
    float frame = m_AnimationFrame->GetCurrentFrame();
    
    m_Animation->GetJointRotation(effectorIndex, frame, effRotate);
    
    // check if we have to do a motion root fixup
    Rig* rig = m_Animation->GetRig();
    int aiRootIndex = rig->GetJointIndex(RIG_JOINT_AI_ROOT);
    int motionRootIndex = rig->GetJointIndex(RIG_JOINT_MOTION_ROOT);
    
    if ((aiRootIndex < effectorIndex) &&
        (motionRootIndex > effectorIndex))
    {
        rmt::Quaternion mrootRotate;
        m_Animation->GetJointRotation(motionRootIndex, frame, mrootRotate);
        mrootRotate.Inverse();
        effRotate.Mult(mrootRotate);
    }
}

bool AnimationFootDriver::HasFootPoleVectorTranslation() const
{
    return m_AnimationFrame->HasJointTranslation(m_RigLeg->GetPoleVectorJointIndex());
}

void AnimationFootDriver::GetFootPoleVectorTranslation(rmt::Vector& pvTranslate) const
{
    // get pole vector transform
    int poleVectorIndex = m_RigLeg->GetPoleVectorJointIndex();
    float frame = m_AnimationFrame->GetCurrentFrame();
    
    m_Animation->GetJointTranslation(poleVectorIndex, frame, pvTranslate);
}

bool AnimationFootDriver::HasFootPlanted() const
{
    return m_AnimationFrame->HasJointTranslation(m_RigLeg->GetFootPlantJointIndex());
}

bool AnimationFootDriver::IsFootPlanted() const
{
    rmt::Vector channels;
    m_Animation->GetJointTranslation(m_RigLeg->GetFootPlantJointIndex(),
                                     m_AnimationFrame->GetCurrentFrame(),
                                     channels);
    
    float footValue = ((float*)&channels)[m_RigLeg->GetFootPlantChannelIndex()];
    
    return (rmt::Epsilon(footValue, m_RigLeg->GetFootPlantValue()));
}

bool AnimationFootDriver::HasFootGroundFixup() const
{
    return m_AnimationFrame->HasJointRotation(m_RigLeg->GetEffectorJointIndex());
}

float AnimationFootDriver::GetFootGroundFixup() const
{
    return m_Animation->GetGroundFixup(m_AnimationFrame->GetCurrentFrame());
}


//---------------------------------------------------------------------------
// class AnimationDriver
//---------------------------------------------------------------------------

AnimationDriver::AnimationDriver(Animation* anim):
    m_Reset(true),
    m_HoldEndFrame(false),
    m_Puppet(0)
{
    P3DASSERT(anim != 0);
    CopyName(anim);
    
    m_Animation = anim;
    m_Animation->AddRef();
    
    m_AnimationFrame = new AnimationFrame(m_Animation);
    m_AnimationFrame->AddRef();
    
    m_AnimationRootDriver = new AnimationRootDriver(m_Animation,
                                                    m_AnimationFrame);
    m_AnimationRootDriver->AddRef();
    
    m_AnimationJointDriver = new AnimationJointDriver(m_Animation,
                                                      m_AnimationFrame);
    m_AnimationJointDriver->AddRef();
    
    Rig* rig = m_Animation->GetRig();
    m_AnimationFootDrivers = new AnimationFootDriver* [rig->GetLegCount()];
    
    for (int i = 0; i < rig->GetLegCount(); ++i)
    {
        m_AnimationFootDrivers[i] = new AnimationFootDriver(rig->GetLeg(i),
                                                            m_Animation,
                                                            m_AnimationFrame);
        m_AnimationFootDrivers[i]->AddRef();
    }
    
    m_IsCyclic = m_Animation->IsCyclic();

    SetPlaybackTimings(m_Animation->GetDuration(),
        m_Animation->GetSpeed(),
        m_Animation->GetStartFrame(),
        m_Animation->GetEndFrame());
}

AnimationDriver::~AnimationDriver()
{
    P3DASSERT(m_Puppet == 0);

    Rig* rig = m_Animation->GetRig();
    for (int i = rig->GetLegCount() - 1; i >= 0; --i)
    {
        m_AnimationFootDrivers[i]->Release();
    }
    delete[] m_AnimationFootDrivers;

    m_AnimationJointDriver->Release();
    m_AnimationRootDriver->Release();
    m_AnimationFrame->Release();
    m_Animation->Release();
}

PuppetDriver* AnimationDriver::Clone() const
{
    return new AnimationDriver(m_Animation);
}

Partition* AnimationDriver::GetPartition() const
{
    return m_AnimationFrame->GetPartition();
}

void AnimationDriver::SetPartition(Partition* partition)
{
    m_AnimationFrame->SetPartition(partition);
}

float AnimationDriver::GetWeight() const
{
    return m_AnimationFrame->GetWeight();
}

void AnimationDriver::SetWeight(float weight)
{
    m_AnimationFrame->SetWeight(weight);
}

int AnimationDriver::GetPriority() const
{
    return m_AnimationFrame->GetPriority();
}

void AnimationDriver::SetPriority(int priority)
{
    m_AnimationFrame->SetPriority(priority);
}

float AnimationDriver::GetRootBlendInTime() const
{
    return m_AnimationFrame->GetRootBlendInTime();
}

void AnimationDriver::SetRootBlendInTime(float rootBlendInTime)
{
    m_AnimationFrame->SetRootBlendInTime(rootBlendInTime);
}

float AnimationDriver::GetRootBlendOutTime() const
{
    return m_AnimationFrame->GetRootBlendOutTime();
}

void AnimationDriver::SetRootBlendOutTime(float rootBlendOutTime)
{
    m_AnimationFrame->SetRootBlendOutTime(rootBlendOutTime);
}

float AnimationDriver::GetBlendInTime() const
{
    return m_AnimationFrame->GetBlendInTime();
}

void AnimationDriver::SetBlendInTime(float blendInTime)
{
    m_AnimationFrame->SetBlendInTime(blendInTime);
}

float AnimationDriver::GetBlendOutTime() const
{
    return m_AnimationFrame->GetBlendOutTime();
}

void AnimationDriver::SetBlendOutTime(float blendOutTime)
{
    m_AnimationFrame->SetBlendOutTime(blendOutTime);
}

void AnimationDriver::SetSpeed(float speed)
{
    if (speed >= 0.0f)
    {
        m_Speed = speed;
    }
}

void AnimationDriver::SetStartFrame(float startFrame)
{
    m_StartFrame = Animation::MakeValidFrame(startFrame, 0.0f, GetFrameCount() - 1.0f, IsCyclic());
}

void AnimationDriver::SetEndFrame(float endFrame)
{
    float frameCountMinusOne = GetFrameCount() - 1.0f;
    if (endFrame >= frameCountMinusOne)
    {
        m_EndFrame = frameCountMinusOne;
        return;
    }

    m_EndFrame = Animation::MakeValidFrame(endFrame, 0.0f, frameCountMinusOne, IsCyclic());
}

void AnimationDriver::SetPlaybackTimings(float duration,
                                         float speed,
                                         float startFrame,
                                         float endFrame)
{
    SetStartFrame(startFrame);
    
    if (endFrame < startFrame)
    {
        SetEndFrame(GetFrameCount() - 1.0f);
    }
    else
    {
        SetEndFrame(endFrame);
    }
    
    if ((duration > 0.0f) && !IsCyclic())
    {
        // reset speed
        SetSpeed(1.0f);
        // recalculate desired speed
        speed = GetCycleDuration() / duration;
        SetSpeed(speed);
    }
    else
    {
        SetSpeed(speed);
    }
}

float AnimationDriver::GetCycleDuration() const
{
    if (rmt::Epsilon(m_Speed, 0.0f))
        return 0.0f;
    if (rmt::Epsilon(GetFramesPerSecond(), 0.0f))
        return 0.0f;
    return (m_EndFrame - m_StartFrame) / (GetFramesPerSecond() * m_Speed);
}

float AnimationDriver::GetDuration() const
{
    if (IsCyclic())
    {
        return -1.0f;
    }
    
    return GetCycleDuration();
}

float AnimationDriver::GetTimeRemaining() const
{
    if (rmt::Epsilon(m_Speed, 0.0f))
        return 0.0f;
    if (rmt::Epsilon(GetFramesPerSecond(), 0.0f))
        return 0.0f;
    return (m_EndFrame - m_AnimationFrame->GetCurrentFrame()) /
           (GetFramesPerSecond() * m_Speed);
}

float AnimationDriver::GetValidFrame(float frame) const
{
    return Animation::MakeValidFrame(frame, m_StartFrame, m_EndFrame, IsCyclic());
}

float AnimationDriver::GetNextFrame(float deltaTime) const
{
    float frameAdvance = deltaTime *
                         m_Animation->GetFramesPerSecond() *
                         GetSpeed();
    float newFrame = GetFrame() + frameAdvance;
    return GetValidFrame(newFrame);
}

float AnimationDriver::GetFrame() const
{
    return m_AnimationFrame->GetCurrentFrame();
}

void AnimationDriver::SetFrame(float frame)
{
    m_AnimationFrame->SetPreviousFrame(GetValidFrame(frame));
    m_AnimationFrame->SetCurrentFrame(GetValidFrame(frame));
}

void AnimationDriver::Reset()
{
    m_AnimationFrame->SetPreviousFrame(GetStartFrame());
    m_AnimationFrame->SetCurrentFrame(GetStartFrame());
}

bool AnimationDriver::HasRootTranslation() const
{
    return m_AnimationRootDriver->HasRootTranslation();
}

void AnimationDriver::SetHasRootTranslation(bool isEnabled)
{
    m_AnimationRootDriver->SetHasRootTranslation(isEnabled);
}

void AnimationDriver::Begin(Puppet* puppet)
{
    P3DASSERT(m_Puppet == 0);
    m_Puppet = puppet; // no reference counting, or we leak
    Reset();
}

void AnimationDriver::Advance(float deltaTime)
{
    m_AnimationFrame->SetPreviousFrame(m_AnimationFrame->GetCurrentFrame());
    m_AnimationFrame->SetCurrentFrame(GetNextFrame(deltaTime));
}

bool AnimationDriver::IsFinished() const
{
    if (IsCyclic())
        return false;
    if (m_HoldEndFrame)
        return false;
    return (GetFrame() >= m_EndFrame);
}

void AnimationDriver::End(Puppet* puppet)
{
    P3DASSERT(m_Puppet == puppet);
    m_Puppet = NULL;
}

void AnimationDriver::Apply(Engine* engine)
{
    engine->GetRootBlender()->Push(m_AnimationRootDriver);
    engine->GetJointBlender()->Push(m_AnimationJointDriver);
    
    int legCount = m_Animation->GetRig()->GetLegCount();
    for (int i = 0; i < legCount; ++i)
    {
        engine->GetFootBlender(i)->Push(m_AnimationFootDrivers[i]);
    }
}


} // namespace choreo


// End of file.
