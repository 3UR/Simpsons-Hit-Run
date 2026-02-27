/*
 * poser/pose.hpp
 */

#ifndef POSER_POSE_HPP
#define POSER_POSE_HPP


#include <p3d/refcounted.hpp>
#include <raddebug.hpp>
#include <poser/joint.hpp>


class tSkeleton;
class tPose;


namespace poser
{


class Joint;


class Pose: public tRefCounted
{
public:
    Pose();
    Pose(tSkeleton* skeleton);
    Pose(Pose* pose);
    Pose(tPose* p3dPose);
    
    void Assign(Pose* pose);
    void Assign(tPose* p3dPose);
    
    tSkeleton* GetSkeleton() const
        { return m_Skeleton; }
    
    int GetJointCount() const;
    
    const Joint* GetJoint(int j) const
    {
        rAssert((j >= 0) && (j < GetJointCount()));
        return (m_Joints + j);
    }

    Joint* GetJoint(int j)
    {
        rAssert((j >= 0) && (j < GetJointCount()));
        return (m_Joints + j);
    }
    
    // fills this pose with the skeleton's rest pose
    void SetRestPose();
    
    // fills a tPose with contents of this pose
    void FillP3DPose(tPose* pose, bool fillObjectMatrices = false) const;

    // removes all dirty flags, but does not change contents of pose
    // - use wisely!
    void Clean();

protected:
    
    virtual ~Pose();
    
private:
    
    tSkeleton* m_Skeleton;
    Joint* m_Joints;
    Joint** m_Children;
};


} // namespace poser


#endif
