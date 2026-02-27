/*
 * choreo/jointblender.hpp
 */

#ifndef CHOREO_JOINTBLENDER_HPP
#define CHOREO_JOINTBLENDER_HPP


#include <poser/posedriver.hpp>
#include <choreo/blendtemplate.hpp>
#include <p3d/anim/pose.hpp>


class tPose;

namespace poser
{
    class Transform;
    class Joint;
};


namespace choreo
{


class Partition;
class LODPartition;
class JointBlendDriver;


class JointBlender: public poser::PoseDriver
{
public:

    enum
    {
        DEFAULT_MAX_DRIVER_COUNT = 4,
        DEFAULT_MAX_BLENDED_PRIORITIES = 4
    };
    
    JointBlender(int maxBlendedDrivers = DEFAULT_MAX_DRIVER_COUNT,
                 int maxBlendedPriorities = DEFAULT_MAX_BLENDED_PRIORITIES,
                 int maxPropCount = 0);
    
    virtual int GetMinimumJointIndex() const
        { return m_MinimumJointIndex; }
    void SetMinimumJointIndex(int jointIndex);

    virtual int GetPriority() const
        { return 0; }
    
    // driver interface
    bool Push(JointBlendDriver* jointDriver);
    
    virtual void Advance(float dt);
    virtual void Update(poser::Pose* pose);

    bool UpdateTransform(tUID groupUID, poser::Transform& transform);
    bool UpdateTransform(const char* groupName, poser::Transform& transform)
        { return UpdateTransform(tEntity::MakeUID(groupName), transform); }
    
    // props
    int GetMaxAttachPropCount() const{ return 0; }
    int GetAttachPropCount() const { return 0; }
    bool AttachProp(int attachJoint, tPose* p3dPose) { return false;}
    bool RemoveAttachedProp(tPose* p3dPose) { return false;}
    bool HasAttachedProp(tPose* p3dPose) const { return false;}
    int GetAttachedPropJoint(tPose* p3dPose) const { return 0;}

protected:
    
    virtual ~JointBlender();
    
private:
    
    void UpdateDriverTranslation(int jointIndex,
                                 JointBlendDriver* jointDriver,
                                 float jointWeight,
                                 int jointPriority);
    void UpdateDriverRotation(int jointIndex,
                              JointBlendDriver* jointDriver,
                              float jointWeight,
                              int jointPriority);
    bool UpdateTranslation(poser::Joint* joint);
    bool UpdateRotation(poser::Joint* joint);
    
    void UpdateGroupDriverTranslation(tUID group,
                                      JointBlendDriver* jointDriver,
                                      float jointWeight,
                                      int jointPriority);
    void UpdateGroupDriverRotation(tUID group,
                                   JointBlendDriver* jointDriver,
                                   float jointWeight,
                                   int jointPriority);
    bool UpdateGroupTranslation(tPose::Joint* joint);
    bool UpdateGroupRotation(tPose::Joint* joint);

    int m_MinimumJointIndex;
    
    struct JointDriverStruct
    {
        JointBlendDriver* jointDriver;
        float weight;
        bool pushed;
    };
    
    int m_JointDriverCount;
    JointDriverStruct m_JointDrivers[DEFAULT_MAX_DRIVER_COUNT];
    
    int m_TranslatePriorityCount;
    BlendPriority<rmt::Vector> m_TranslatePriorities[DEFAULT_MAX_BLENDED_PRIORITIES];
    
    int m_RotatePriorityCount;
    BlendPriority<rmt::Quaternion> m_RotatePriorities[DEFAULT_MAX_BLENDED_PRIORITIES];
};


} // namespace choreo


#endif
