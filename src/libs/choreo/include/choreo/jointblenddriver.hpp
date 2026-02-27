/*
 * choreo/jointblenddriver.hpp
 */

#ifndef CHOREO_JOINTBLENDDRIVER_HPP
#define CHOREO_JOINTBLENDDRIVER_HPP


#include <choreo/buildconfig.hpp>

#include <radmath/radmath.hpp>
#include <p3d/refcounted.hpp>
#include <p3d/p3dtypes.hpp>


namespace choreo
{


class JointBlendDriver: public tRefCounted
{
public:

    virtual void Reset() = 0;
    virtual void Advance(float dt) = 0;
    
    // blend times
    virtual float GetBlendInTime() const = 0;
    virtual float GetBlendOutTime() const = 0;
    
    // weight & priority
    virtual float GetJointWeight(int jointIndex) const = 0;
    virtual int GetJointPriority(int jointIndex) const = 0;
    
    // translation
    virtual bool HasJointTranslation(int jointIndex) const = 0;
    virtual void GetJointTranslation(int jointIndex, rmt::Vector& translation) const = 0;
    
    // rotation
    virtual bool HasJointRotation(int jointIndex) const = 0;
    virtual void GetJointRotation(int jointIndex, rmt::Quaternion& rotation) const = 0;
    
    // weight & priority
    virtual float GetGroupWeight(tUID group) const = 0;
    virtual int GetGroupPriority(tUID group) const = 0;
    
    // translation
    virtual bool HasGroupTranslation(tUID group) const = 0;
    virtual void GetGroupTranslation(tUID group, rmt::Vector& translation) const = 0;
    
    // rotation
    virtual bool HasGroupRotation(tUID group) const = 0;
    virtual void GetGroupRotation(tUID group, rmt::Quaternion& rotation) const = 0;
};


} // namespace choreo


#endif
