/*
 * choreo/replayblender.hpp
 */

#ifndef CHOREO_REPLAYBLENDER_HPP
#define CHOREO_REPLAYBLENDER_HPP


#include <poser/posedriver.hpp>
#include <poser/transform.hpp>
#include <choreo/blendtemplate.hpp>


namespace poser
{
    class Joint;
};


namespace choreo
{


class ReplayBlendDriver;


class ReplayBlender: public poser::PoseDriver
{
public:

    enum
    {
        DEFAULT_MAX_DRIVER_COUNT = 32,
        DEFAULT_MAX_BLENDED_PRIORITIES = 4
    };
    
    ReplayBlender(int maxBlendedDrivers = DEFAULT_MAX_DRIVER_COUNT,
                  int maxBlendedPriorities = DEFAULT_MAX_BLENDED_PRIORITIES);
    
    virtual int GetMinimumJointIndex() const
        { return m_MinimumJointIndex; }
    void SetMinimumJointIndex(int jointIndex);
    
    virtual int GetPriority() const
        { return 0; }

    bool Push(ReplayBlendDriver* jointDriver);
    
    // recording
    virtual void Advance(float dt);
    virtual void Update(poser::Pose* pose);

    // replaying
    void AdvanceReplay(float dt);
    void UpdateReplay(poser::Pose* pose);
    
protected:
    
    virtual ~ReplayBlender();
    
private:

    void UpdateDriver(int jointIndex,
                      ReplayBlendDriver* replayDriver,
                      float jointWeight,
                      int jointPriority);
    void UpdateJoint(poser::Joint* joint);

    int m_MinimumJointIndex;
    int m_MaxReplayDrivers;

    struct ReplayDriverStruct
    {
        ReplayBlendDriver* replayDriver;
        float weight;
        bool pushed;
    };

    int m_ReplayDriverCount;
    ReplayDriverStruct* m_ReplayDrivers;

    int m_MaxBlendedPriorities;

    int m_PriorityCount;
    BlendPriority<poser::Transform>* m_Priorities;
};


} // namespace choreo


#endif
