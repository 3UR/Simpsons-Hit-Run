/*
 * choreo/rootblenddriver.hpp
 */

#ifndef CHOREO_ROOTBLENDDRIVER_HPP
#define CHOREO_ROOTBLENDDRIVER_HPP


#include <p3d/refcounted.hpp>
#include <radmath/radmath.hpp>


namespace poser
{
    class Transform;
}


namespace choreo
{


class RootBlender;


class RootBlendDriver: public tRefCounted
{
public:

    virtual void Reset(RootBlender* rootBlender) = 0;
    
    // blend times
    virtual float GetRootBlendInTime() const = 0;
    virtual float GetRootBlendOutTime() const = 0;
    
    // weight & priority
    virtual float GetRootWeight() const = 0;
    virtual int GetRootPriority() const = 0;
    
    // translation
    virtual bool HasRootTranslation() const = 0;
    virtual void GetRootTranslation(const poser::Transform& oldTransform,
                                    rmt::Vector& newTranslation) const = 0;
    
    // rotation
    virtual bool HasRootRotation() const = 0;
    virtual void GetRootRotation(const poser::Transform& oldTransform,
                                 rmt::Quaternion& newRotate) const = 0;
    
    // ground fixup
    virtual bool HasRootGroundFixup() const = 0;
    virtual float GetRootGroundFixup() const = 0;
};


} // namespace choreo


#endif
