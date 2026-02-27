/*
 * choreo/replay.cpp
 */

#include <choreo/replay.hpp>
#include <choreo/rig.hpp>
#include <choreo/partition.hpp>
#include <choreo/scriptreader.hpp>
#include <choreo/scriptwriter.hpp>
#include <choreo/basebank.hpp>
#include <choreo/replayblenddriver.hpp>
#include <choreo/engine.hpp>
#include <choreo/replayblender.hpp>
#include <choreo/load.hpp>

#include <poser/transform.hpp>

#include <p3d/error.hpp>

#include <string.h>


namespace choreo
{


//---------------------------------------------------------------------------
// class Replay
//---------------------------------------------------------------------------

Replay::Replay(Rig* rig,
               int recordFrameCount,
               Partition* partition):
    m_Weight(1.0f),
    m_Priority(0),

    m_BlendInTime(0.0f),
    m_BlendOutTime(0.0f),

    m_MinFrameTime(1.0f / 60.0f)   // defaults to 60fps record rate
{
    P3DASSERT(rig != 0);
    m_Rig = rig;
    m_Rig->AddRef();

    if (partition != 0)
    {
        m_Partition = partition;
        m_Partition->AddRef();
    }
    else
    {
        m_Partition = 0;
    }

    SetRecordFrameCount(recordFrameCount);
}

Replay::~Replay()
{
    if (m_Partition != 0)
    {
        m_Partition->Release();
    }

    m_Rig->Release();
}

void Replay::SetPartition(Partition* partition)
{
    tRefCounted::Assign(m_Partition, partition);
}

void Replay::SetRecordFrameCount(int recordFrameCount)
{
    m_RecordFrameCount = recordFrameCount;

    if (m_RecordFrameCount < 0)
    {
        m_RecordFrameCount = 0;
    }
}

PuppetDriver* Replay::NewDriver()
{
    return NewReplayDriver();
}

ReplayDriver* Replay::NewReplayDriver()
{
    return new ReplayDriver(this);
}

static const char* TOKEN_REPLAY           = "replay";
static const char* TOKEN_NAME             = "name";
static const char* TOKEN_PARTITION        = "partition";
static const char* TOKEN_WEIGHT           = "weight";
static const char* TOKEN_PRIORITY         = "priority";
static const char* TOKEN_BLENDINTIME      = "blendInTime";
static const char* TOKEN_BLENDOUTTIME     = "blendOutTime";
static const char* TOKEN_MINFRAMETIME     = "minFrameTime";
static const char* TOKEN_RECORDFRAMECOUNT = "recordFrameCount";

bool Replay::ReadFromScript(ScriptReader* script, BaseBank* bank)
{
    if (!script->IsMoreStructs())
        return false;
    if (strcmp(script->GetNextStructIdentifier(), TOKEN_REPLAY) != 0)
        return false;
    
    script->BeginStruct(TOKEN_REPLAY);
    
    // required params
    char replayName[256] = "";
    script->RequiredParamString(TOKEN_NAME, replayName, sizeof(replayName));
    
    // optional params
    char partitionName[256] = "";
    script->ParamString(TOKEN_PARTITION, partitionName, sizeof(partitionName));
    
    float weight = 1.0f;
    script->ParamFloat(TOKEN_WEIGHT, &weight);
    
    float priority = 0.0f;
    script->ParamFloat(TOKEN_PRIORITY, &priority);
    
    float blendInTime = 0.0f;
    script->ParamFloat(TOKEN_BLENDINTIME, &blendInTime);
    
    float blendOutTime = 0.0f;
    script->ParamFloat(TOKEN_BLENDOUTTIME, &blendOutTime);
    
    float minFrameTime = 0.0f;
    script->ParamFloat(TOKEN_MINFRAMETIME, &minFrameTime);
    
    float recordFrameCountf = 0.0f;
    script->ParamFloat(TOKEN_RECORDFRAMECOUNT, &recordFrameCountf);
    
    script->EndStruct();
    
    // resolve rig
    Rig* rig = bank->GetRig();
    if (rig == 0)
    {
        script->SetErrorString("Replay loaded outside of bank, cannot create without rig.");
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
    
    int recordFrameCount = (int)recordFrameCountf;
    Replay* replay = new Replay(rig, recordFrameCount, partition);
    replay->AddRef();
    replay->SetName(replayName);
    replay->SetWeight(weight);
    replay->SetPriority((int)priority);
    replay->SetBlendInTime(blendInTime);
    replay->SetBlendOutTime(blendOutTime);
    replay->SetMinFrameTime(minFrameTime);
    
    bank->Store(replay);
    
    replay->Release();
    return true;
}

#ifdef CHOREO_TOOL
bool Replay::WriteToScriptHandler(ScriptWriter* script, BaseBank* bank)
{
    CHOREO_SCRIPT_WRITE_HANDLER(Replay, script, bank);
}

bool Replay::WriteToScript(ScriptWriter* script) const
{
    script->BeginStruct(TOKEN_REPLAY);

    // required params
    script->RequiredParamString(TOKEN_NAME, GetName());
    
    // optional params
	if (m_Partition != 0)
	{
		script->ParamString(TOKEN_PARTITION, m_Partition->GetName());
	}

    script->ParamFloat(TOKEN_WEIGHT, m_Weight);
    script->ParamFloat(TOKEN_PRIORITY, (float)m_Priority);
    script->ParamFloat(TOKEN_BLENDINTIME, m_BlendInTime);
    script->ParamFloat(TOKEN_BLENDOUTTIME, m_BlendOutTime);
    script->ParamFloat(TOKEN_MINFRAMETIME, m_MinFrameTime);
    script->ParamFloat(TOKEN_RECORDFRAMECOUNT, (float)m_RecordFrameCount);

    script->EndStruct();
	return true;
}
#endif // CHOREO_TOOL


//---------------------------------------------------------------------------
// class ReplayJoint - interface
//---------------------------------------------------------------------------

class ReplayJoint
{
public:

    ReplayJoint()
        { }
    ReplayJoint(const poser::Transform& transform)
        { Compress(transform); }

    void Compress(const poser::Transform& transform);
    void Uncompress(poser::Transform& transform) const;

private:

    rmt::Vector m_Translate;
    rmt::Quaternion m_Rotate;
};


//---------------------------------------------------------------------------
// class ReplayJoint - implementation
//---------------------------------------------------------------------------

void ReplayJoint::Compress(const poser::Transform& transform)
{
    m_Translate = transform.GetTranslation();
    m_Rotate = transform.GetQuaternion();
}

void ReplayJoint::Uncompress(poser::Transform& transform) const
{
    transform.SetTranslation(m_Translate);
    transform.SetQuaternion(m_Rotate);
}


//---------------------------------------------------------------------------
// class ReplayBuffer - interface
//---------------------------------------------------------------------------

class ReplayBuffer: public tRefCounted
{
public:

    ReplayBuffer(Replay* replay);

    // partition
    Partition* GetPartition() const
        { return m_Partition; }
    void SetPartition(Partition* partition)
        { tRefCounted::Assign(m_Partition, partition); }
    
    // weight & priority
    float GetWeight() const
        { return m_Weight; }
    void SetWeight(float weight)
        { m_Weight = weight; }
    
    int GetPriority() const
        { return m_Priority; }
    void SetPriority(int priority)
        { m_Priority = priority; }

    // blend times
    float GetBlendInTime() const
        { return m_BlendInTime; }
    void SetBlendInTime(float blendInTime)
        { m_BlendInTime = blendInTime; }
    float GetBlendOutTime() const
        { return m_BlendOutTime; }
    void SetBlendOutTime(float blendOutTime)
        { m_BlendOutTime = blendOutTime; }

    float GetPlaybackTime() const
        { return m_PlaybackTime; }
    void SetPlaybackTime(float playbackTime);

    float GetRecordBufferTime() const
        { return m_TotalFrameTimes; }

    void Reset();
    void Advance(float deltaTime);
    void Update();
    void AdvanceReplay(float deltaTime);

    float GetJointWeight(int jointIndex) const;
    int GetJointPriority(int jointIndex) const;

    bool HasJoint(int jointIndex) const;

    void GetJointTransform(int jointIndex, poser::Transform& transform) const;
    void SetJointTransform(int jointIndex, const poser::Transform& transform);

protected:

    virtual ~ReplayBuffer();

private:

    Replay* m_Replay;

    Partition* m_Partition;
    float m_Weight;
    int m_Priority;

    float m_BlendInTime;
    float m_BlendOutTime;

    float m_DeltaTime;

    float m_TotalFrameTimes;
    float* m_FrameTimes;
    ReplayJoint* m_FrameJoints;

    int m_RecordFrameIndex;

    float m_PlaybackTime;
    int m_PlaybackFrameIndex;
    float m_PlaybackDeltaTime;
};


//---------------------------------------------------------------------------
// class ReplayBuffer - implementation
//---------------------------------------------------------------------------

ReplayBuffer::ReplayBuffer(Replay* replay):
    m_Weight(1.0f),
    m_Priority(0)
{
    P3DASSERT(replay != 0);
    m_Replay = replay;
    m_Replay->AddRef();

    m_Partition = m_Replay->GetPartition();
    if (m_Partition != 0)
    {
        m_Partition->AddRef();
    }

    m_BlendInTime = m_Replay->GetBlendInTime();
    m_BlendOutTime = m_Replay->GetBlendOutTime();

    m_FrameTimes = new float [m_Replay->GetRecordFrameCount()];
    m_FrameJoints = new ReplayJoint [m_Replay->GetRecordFrameCount() *
                                     m_Replay->GetRig()->GetJointCount()];

    Reset();
}

ReplayBuffer::~ReplayBuffer()
{
    delete[] m_FrameJoints;
    delete[] m_FrameTimes;

    if (m_Partition != 0)
    {
        m_Partition->Release();
    }

    m_Replay->Release();
}

void ReplayBuffer::Reset()
{
    m_DeltaTime = 0.0f;
    m_TotalFrameTimes = 0.0f;
    m_RecordFrameIndex = 0;

    m_PlaybackTime = 0.0f;
    m_PlaybackFrameIndex = 0;
    m_PlaybackDeltaTime = 0.0f;

    memset(m_FrameTimes, 0, sizeof(float) * m_Replay->GetRecordFrameCount());
}

void ReplayBuffer::SetPlaybackTime(float playbackTime)
{
    if (playbackTime >= 0.0f)
    {
        m_PlaybackTime = 0.0f;
        m_PlaybackFrameIndex = m_RecordFrameIndex;
        m_PlaybackDeltaTime = 0.0f;
        return;
    }

    m_PlaybackTime = playbackTime;
    m_PlaybackFrameIndex = m_RecordFrameIndex;

    while (-playbackTime > m_FrameTimes[m_PlaybackFrameIndex])
    {
        playbackTime += m_FrameTimes[m_PlaybackFrameIndex];

        --m_PlaybackFrameIndex;

        // check for buffer loop
        if (m_PlaybackFrameIndex < 0)
        {
            m_PlaybackFrameIndex = m_Replay->GetRecordFrameCount() - 1;
        }

        // check if we've completely wrapped
        if (m_PlaybackFrameIndex == m_RecordFrameIndex)
        {
            m_PlaybackFrameIndex = (m_PlaybackFrameIndex + 1) % m_Replay->GetRecordFrameCount();
            playbackTime = 0.0f;
        }
    }

    m_PlaybackDeltaTime = m_FrameTimes[m_PlaybackFrameIndex] + playbackTime;
}

void ReplayBuffer::Advance(float deltaTime)
{
    m_DeltaTime += deltaTime;
}

void ReplayBuffer::Update()
{
    m_TotalFrameTimes -= m_FrameTimes[m_RecordFrameIndex];
    m_FrameTimes[m_RecordFrameIndex] = m_DeltaTime;
    m_TotalFrameTimes += m_DeltaTime;

    if ((m_DeltaTime > 0.0f) &&
        (m_DeltaTime > m_Replay->GetMinFrameTime()))
    {
        int prevRecordFrame = m_RecordFrameIndex;
        m_RecordFrameIndex = (m_RecordFrameIndex + 1) % m_Replay->GetRecordFrameCount();

        // copy previous frame over
        memcpy(m_FrameJoints + (m_RecordFrameIndex * m_Replay->GetRig()->GetJointCount()),
               m_FrameJoints + (prevRecordFrame * m_Replay->GetRig()->GetJointCount()),
               m_Replay->GetRig()->GetJointCount() * sizeof(ReplayJoint));

        m_DeltaTime = 0.0f;
    }
}

void ReplayBuffer::AdvanceReplay(float deltaTime)
{
    if (m_PlaybackFrameIndex != m_RecordFrameIndex)
    {
        m_PlaybackDeltaTime += deltaTime;
        m_PlaybackTime += deltaTime;

        while (m_PlaybackDeltaTime > m_FrameTimes[m_PlaybackFrameIndex]) 
        {
            m_PlaybackDeltaTime -= m_FrameTimes[m_PlaybackFrameIndex];
            m_PlaybackFrameIndex = (m_PlaybackFrameIndex + 1) % m_Replay->GetRecordFrameCount();

            if (m_PlaybackFrameIndex == m_RecordFrameIndex)
                break;
        }
    }
    else
    {
        m_PlaybackDeltaTime = 0.0f;
        m_PlaybackTime = 0.0f;
    }
}

float ReplayBuffer::GetJointWeight(int jointIndex) const
{
    float weight = GetWeight();
    weight *= m_Replay->GetWeight();

    if (m_Partition != 0)
    {
        weight *= m_Partition->GetJointWeight(jointIndex);
    }

    return weight;
}

int ReplayBuffer::GetJointPriority(int jointIndex) const
{
    int priority = GetPriority();
    priority += m_Replay->GetPriority();

    if (m_Partition != 0)
    {
        priority += m_Partition->GetJointPriority(jointIndex);
    }

    return priority;
}

bool ReplayBuffer::HasJoint(int jointIndex) const
{
    if ((jointIndex < 0) || (jointIndex >= m_Replay->GetRig()->GetJointCount()))
        return false;
    if (m_Partition == 0)
        return true;
    return m_Partition->HasJoint(jointIndex);
}

void ReplayBuffer::GetJointTransform(int jointIndex, poser::Transform& transform) const
{
    int jointCount = m_Replay->GetRig()->GetJointCount();
    P3DASSERT((jointIndex >= 0) && (jointIndex < jointCount));

    m_FrameJoints[(m_PlaybackFrameIndex * jointCount) + jointIndex].Uncompress(transform);

    // blend to next frame
    if (m_PlaybackFrameIndex != m_RecordFrameIndex)
    {
        int nextFrameIndex = (m_PlaybackFrameIndex + 1) % m_Replay->GetRecordFrameCount();
        poser::Transform nextTransform;
        m_FrameJoints[(nextFrameIndex * jointCount) + jointIndex].Uncompress(nextTransform);

        float alpha = m_PlaybackDeltaTime / m_FrameTimes[m_PlaybackFrameIndex];
        transform.Blend(alpha, nextTransform);
    }
}

void ReplayBuffer::SetJointTransform(int jointIndex, const poser::Transform& transform)
{
    int jointCount = m_Replay->GetRig()->GetJointCount();
    P3DASSERT((jointIndex >= 0) && (jointIndex < jointCount));

    m_FrameJoints[(m_RecordFrameIndex * jointCount) + jointIndex].Compress(transform);
}


//---------------------------------------------------------------------------
// class ReplayDriverInternal - interface
//---------------------------------------------------------------------------

class ReplayDriverInternal: public ReplayBlendDriver
{
public:

    ReplayDriverInternal(ReplayBuffer* replayBuffer);

    virtual void Reset();
    virtual void Advance(float dt);
    virtual void Update();
    virtual void AdvanceReplay(float dt);
    
    // blend times
    virtual float GetBlendInTime() const;
    virtual float GetBlendOutTime() const;
    
    // weight & priority
    virtual float GetJointWeight(int jointIndex) const;
    virtual int GetJointPriority(int jointIndex) const;
    
    // joint world transform storage/retrieval
    virtual bool HasJoint(int jointIndex) const;
    virtual void GetJointTransform(int jointIndex, poser::Transform& transform) const;
    virtual void RecordJointTransform(int jointIndex, const poser::Transform& transform) const;

protected:

    virtual ~ReplayDriverInternal();

private:

    ReplayBuffer* m_ReplayBuffer;
};


//---------------------------------------------------------------------------
// class ReplayDriverInternal - implementation
//---------------------------------------------------------------------------

ReplayDriverInternal::ReplayDriverInternal(ReplayBuffer* replayBuffer)
{
    P3DASSERT(replayBuffer != 0);
    m_ReplayBuffer = replayBuffer;
    m_ReplayBuffer->AddRef();
}

ReplayDriverInternal::~ReplayDriverInternal()
{
    m_ReplayBuffer->Release();
}

void ReplayDriverInternal::Reset()
{
    m_ReplayBuffer->Reset();
}

void ReplayDriverInternal::Advance(float dt)
{
    m_ReplayBuffer->Advance(dt);
}

void ReplayDriverInternal::Update()
{
    m_ReplayBuffer->Update();
}
    
void ReplayDriverInternal::AdvanceReplay(float dt)
{
    m_ReplayBuffer->AdvanceReplay(dt);
}

float ReplayDriverInternal::GetBlendInTime() const
{
    return m_ReplayBuffer->GetBlendInTime();
}

float ReplayDriverInternal::GetBlendOutTime() const
{
    return m_ReplayBuffer->GetBlendOutTime();
}
    
float ReplayDriverInternal::GetJointWeight(int jointIndex) const
{
    return m_ReplayBuffer->GetJointWeight(jointIndex);
}

int ReplayDriverInternal::GetJointPriority(int jointIndex) const
{
    return m_ReplayBuffer->GetJointPriority(jointIndex);
}

bool ReplayDriverInternal::HasJoint(int jointIndex) const
{
    return m_ReplayBuffer->HasJoint(jointIndex);
}

void ReplayDriverInternal::GetJointTransform(int jointIndex,
                                             poser::Transform& transform) const
{
    m_ReplayBuffer->GetJointTransform(jointIndex, transform);
}

void ReplayDriverInternal::RecordJointTransform(int jointIndex,
                                                const poser::Transform& transform) const
{
    m_ReplayBuffer->SetJointTransform(jointIndex, transform);
}


//---------------------------------------------------------------------------
// class ReplayDriver
//---------------------------------------------------------------------------

ReplayDriver::ReplayDriver(Replay* replay)
{
    P3DASSERT(replay != 0);
    CopyName(replay);

    m_Replay = replay;
    m_Replay->AddRef();

    m_ReplayBuffer = new ReplayBuffer(m_Replay);
    m_ReplayBuffer->AddRef();

    m_ReplayDriverInternal = new ReplayDriverInternal(m_ReplayBuffer);
    m_ReplayDriverInternal->AddRef();
}

ReplayDriver::~ReplayDriver()
{
    m_ReplayDriverInternal->Release();
    m_ReplayBuffer->Release();
    m_Replay->Release();
}

PuppetDriver* ReplayDriver::Clone() const
{
    return new ReplayDriver(m_Replay);
}

Partition* ReplayDriver::GetPartition() const
{
    return m_ReplayBuffer->GetPartition();
}

void ReplayDriver::SetPartition(Partition* partition)
{
    m_ReplayBuffer->SetPartition(partition);
}

float ReplayDriver::GetWeight() const
{
    return m_ReplayBuffer->GetWeight();
}

void ReplayDriver::SetWeight(float weight)
{
    m_ReplayBuffer->SetWeight(weight);
}

int ReplayDriver::GetPriority() const
{
    return m_ReplayBuffer->GetPriority();
}

void ReplayDriver::SetPriority(int priority)
{
    m_ReplayBuffer->SetPriority(priority);
}

float ReplayDriver::GetBlendInTime() const
{
    return m_ReplayBuffer->GetBlendInTime();
}

void ReplayDriver::SetBlendInTime(float blendInTime)
{
    m_ReplayBuffer->SetBlendInTime(blendInTime);
}

float ReplayDriver::GetBlendOutTime() const
{
    return m_ReplayBuffer->GetBlendOutTime();
}

void ReplayDriver::SetBlendOutTime(float blendOutTime)
{
    m_ReplayBuffer->SetBlendOutTime(blendOutTime);
}

float ReplayDriver::GetPlaybackTime() const
{
    return m_ReplayBuffer->GetPlaybackTime();
}

void ReplayDriver::SetPlaybackTime(float playbackTime)
{
    m_ReplayBuffer->SetPlaybackTime(playbackTime);
}

float ReplayDriver::GetRecordBufferTime() const
{
    return m_ReplayBuffer->GetRecordBufferTime();
}

void ReplayDriver::Begin(Puppet* puppet)
{
    // do nothing
}

void ReplayDriver::Advance(float deltaTime)
{
    // do nothing
}

bool ReplayDriver::IsFinished() const
{
    // always recording or playing back
    return false;
}

void ReplayDriver::End(Puppet* puppet)
{
    // do nothing
}

void ReplayDriver::Apply(Engine* engine)
{
    ReplayBlender* replayBlender = engine->GetReplayBlender();

    if (replayBlender != 0)
    {
        replayBlender->Push(m_ReplayDriverInternal);
    }
}


} // namespace choreo


// End of file.
