/*
 * choreo/replay.hpp
 */

#ifndef CHOREO_REPLAY_HPP
#define CHOREO_REPLAY_HPP


#include <choreo/buildconfig.hpp>
#include <choreo/driver.hpp>


namespace choreo
{


class Rig;
class Partition;
class ReplayDriver;
class ReplayBuffer;
class ReplayDriverInternal;
class ScriptReader;
class ScriptWriter;
class BaseBank;


//---------------------------------------------------------------------------
// class Replay
//---------------------------------------------------------------------------

class Replay: public PuppetDriverFactory
{
public:

    enum
    {
        DEFAULT_RECORD_FRAME_COUNT = 300
    };

    Replay(Rig* rig,
           int recordFrameCount = DEFAULT_RECORD_FRAME_COUNT,
           Partition* partition = 0);

    Rig* GetRig() const
        { return m_Rig; }

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

    // blend times
    float GetBlendInTime() const
        { return m_BlendInTime; }
    void SetBlendInTime(float blendInTime)
        { m_BlendInTime = blendInTime; }
    float GetBlendOutTime() const
        { return m_BlendOutTime; }
    void SetBlendOutTime(float blendOutTime)
        { m_BlendOutTime = blendOutTime; }

    // minimum delta time
    float GetMinFrameTime() const
        { return m_MinFrameTime; }
    void SetMinFrameTime(float minFrameTime)
        { m_MinFrameTime = minFrameTime; }

    // frame counts
    int GetRecordFrameCount() const
        { return m_RecordFrameCount; }
    void SetRecordFrameCount(int recordFrameCount);

    // driver creation
    virtual PuppetDriver* NewDriver();
    ReplayDriver* NewReplayDriver();

	// script loading
    static bool ReadFromScript(ScriptReader* script, BaseBank* bank);

#ifdef CHOREO_TOOL
    static bool WriteToScriptHandler(ScriptWriter* script, BaseBank* bank);
    bool WriteToScript(ScriptWriter* script) const;
#endif // CHOREO_TOOL

protected:

    virtual ~Replay();

private:

    Rig* m_Rig;

    Partition* m_Partition;
    float m_Weight;
    int m_Priority;

    float m_BlendInTime;
    float m_BlendOutTime;

    float m_MinFrameTime;

    int m_RecordFrameCount;
};


//---------------------------------------------------------------------------
// class ReplayDriver
//---------------------------------------------------------------------------

class ReplayDriver: public PuppetDriver
{
public:

    ReplayDriver(Replay* replay);

    virtual PuppetDriver* Clone() const;

    // partition
    Partition* GetPartition() const;
    void SetPartition(Partition* partition);
    
    // weight & priority
    float GetWeight() const;
    void SetWeight(float weight);
    
    int GetPriority() const;
    void SetPriority(int priority);

    // blend times
    float GetBlendInTime() const;
    void SetBlendInTime(float blendInTime);
    float GetBlendOutTime() const;
    void SetBlendOutTime(float blendOutTime);

    // should be < 0
    float GetPlaybackTime() const;
    void SetPlaybackTime(float playbackTime);

    float GetRecordBufferTime() const;

    virtual void Begin(Puppet* puppet);
    virtual void Advance(float deltaTime);
    virtual bool IsFinished() const;
    virtual void End(Puppet* puppet);
    
    virtual void Apply(Engine* engine);

protected:

    virtual ~ReplayDriver();

private:

    Replay* m_Replay;
    ReplayBuffer* m_ReplayBuffer;
    ReplayDriverInternal* m_ReplayDriverInternal;
};


} // namespace choreo


#endif
