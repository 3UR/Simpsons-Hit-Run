/*
 * choreo/footblenddriver.hpp
 */

#ifndef CHOREO_FOOTBLENDDRIVER_HPP
#define CHOREO_FOOTBLENDDRIVER_HPP


#include <radmath/radmath.hpp>
#include <p3d/refcounted.hpp>


namespace choreo
{


class FootBlendDriver: public tRefCounted
{
public:

    // blending
    virtual float GetFootBlendInTime() const = 0;
    virtual float GetFootBlendOutTime() const = 0;
    
    // weight & priority
    virtual float GetFootWeight() const = 0;
    virtual int GetFootPriority() const = 0;
    
    // effector
    virtual bool HasFootEffectorTranslation() const = 0;
    virtual void GetFootEffectorTranslation(rmt::Vector& effTranslate) const = 0;
    
    virtual bool HasFootEffectorRotation() const = 0;
    virtual void GetFootEffectorRotation(rmt::Quaternion& effRotate) const = 0;
    
    // pole vector
    virtual bool HasFootPoleVectorTranslation() const = 0;
    virtual void GetFootPoleVectorTranslation(rmt::Vector& pvTranslate) const = 0;
    
    // plant
    virtual bool HasFootPlanted() const = 0;
    virtual bool IsFootPlanted() const = 0;
    float GetFootPlanted() const
        { return IsFootPlanted(); }
    
    // ground fixup
    virtual bool HasFootGroundFixup() const = 0;
    virtual float GetFootGroundFixup() const = 0;
};


} // namespace choreo


#endif
