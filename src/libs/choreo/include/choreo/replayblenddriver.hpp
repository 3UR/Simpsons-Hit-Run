/*
 * choreo/replayblenddriver.hpp
 */

#ifndef CHOREO_REPLAYBLENDDRIVER_HPP
#define CHOREO_REPLAYBLENDDRIVER_HPP


#include <p3d/refcounted.hpp>
#include <poser/transform.hpp>


namespace choreo
{


class ReplayBlendDriver: public tRefCounted
{
public:

    virtual void Reset() = 0;
    virtual void Advance(float dt) = 0;
    virtual void Update() = 0;

    virtual void AdvanceReplay(float dt) = 0;
    
    // blend times
    virtual float GetBlendInTime() const = 0;
    virtual float GetBlendOutTime() const = 0;
    
    // weight & priority
    virtual float GetJointWeight(int jointIndex) const = 0;
    virtual int GetJointPriority(int jointIndex) const = 0;
    
    // joint world transform storage/retrieval
    virtual bool HasJoint(int jointIndex) const = 0;
    virtual void GetJointTransform(int jointIndex, poser::Transform& transform) const = 0;
    virtual void RecordJointTransform(int jointIndex, const poser::Transform& transform) const = 0;
};


} // namespace choreo


#endif
