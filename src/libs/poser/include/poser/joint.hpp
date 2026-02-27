/*
 * poser/joint.hpp
 */

#ifndef POSER_JOINT_HPP
#define POSER_JOINT_HPP


#include <poser/constants.hpp>
#include <poser/transform.hpp>


namespace poser
{


class Joint
{
public:

    Joint();
    Joint(const Joint& joint);
    ~Joint();
    
    Joint& operator = (const Joint& joint);

    int GetIndex() const
        { return m_Index; }
    void SetIndex(int index)
        { m_Index = index; }

    Joint* GetParentJoint()
        { return m_ParentJoint; }
    const Joint* GetParentJoint() const
        { return m_ParentJoint; }
    void SetParentJoint(Joint* parent)
        { m_ParentJoint = parent; }
    
    int GetChildJointCount() const
        { return m_ChildJointCount; }
    void SetChildJointCount(int count);
    void SetChildJointMem(Joint** mem);
    
    Joint* GetChildJoint(int i);
    const Joint* GetChildJoint(int i) const;
    bool AddChildJoint(Joint* child);
    
    // object-space accessors
    const Transform& GetObjectTransform() const;
    void SetObjectTransform(const Transform& xForm);
    
    const rmt::Matrix& GetObjectMatrix() const
        { return GetObjectTransform().GetMatrix(); }
    void SetObjectMatrix(const rmt::Matrix& matrix);
    void SetObjectMatrixRotation(const rmt::Matrix& matrix);
    
    const rmt::Vector& GetObjectTranslation() const
        { return GetObjectTransform().GetTranslation(); }
    void SetObjectTranslation(const rmt::Vector& trans);
    
    const rmt::Quaternion& GetObjectQuaternion() const
        { return GetObjectTransform().GetQuaternion(); }
    void SetObjectQuaternion(const rmt::Quaternion& quat);

    // world-space accessors
    const Transform& GetWorldTransform() const;
    void SetWorldTransform(const Transform& xForm);
    
    const rmt::Matrix& GetWorldMatrix() const
        { return GetWorldTransform().GetMatrix(); }
    void SetWorldMatrix(const rmt::Matrix& matrix);
    void SetWorldMatrixRotation(const rmt::Matrix& matrix);
    
    const rmt::Vector& GetWorldTranslation() const
        { return GetWorldTransform().GetTranslation(); }
    void SetWorldTranslation(const rmt::Vector& trans);
    
    const rmt::Quaternion& GetWorldQuaternion() const
        { return GetWorldTransform().GetQuaternion(); }
    void SetWorldQuaternion(const rmt::Quaternion& quat);
    
    // convenience accessors using the Space enum
    const Transform& GetTransform(Space space) const
        { return ((space == SPACE_OBJECT) ? GetObjectTransform() : GetWorldTransform()); }
    void SetTransform(Space space, const Transform& xForm)
        { (space == SPACE_OBJECT) ? SetObjectTransform(xForm) : SetWorldTransform(xForm); }
    
    const rmt::Matrix& GetMatrix(Space space) const
        { return ((space == SPACE_OBJECT) ? GetObjectMatrix() : GetWorldMatrix()); }
    void SetMatrix(Space space, const rmt::Matrix& matrix)
        { ((space == SPACE_OBJECT) ? SetObjectMatrix(matrix) : SetWorldMatrix(matrix)); }
    
    const rmt::Vector& GetTranslation(Space space) const
        { return ((space == SPACE_OBJECT) ? GetObjectTranslation() : GetWorldTranslation()); }
    void SetTranslation(Space space, const rmt::Vector& trans)
        { ((space == SPACE_OBJECT) ? SetObjectTranslation(trans) : SetWorldTranslation(trans)); }
    
    const rmt::Quaternion& GetQuaternion(Space space) const
        { return ((space == SPACE_OBJECT) ? GetObjectQuaternion() : GetWorldQuaternion()); }
    void SetQuaternion(Space space, const rmt::Quaternion& quat)
        { ((space == SPACE_OBJECT) ? SetObjectQuaternion(quat) : SetWorldQuaternion(quat)); }

    // removes dirty flags without changes contents
    // - use wisely!
    void Clean();
    
private:
    friend class Pose;
    
    void NotifyWorldModified() const;
    void ParentWorldModified();
    
    int m_Index;
    Joint* m_ParentJoint;
    
    int m_MaxChildJointCount;
    int m_ChildJointCount;
    Joint** m_ChildJoints;
    
    Transform m_Object;
    Transform m_World;
    
    enum Dirty
    {
        DIRTY_NONE,
        DIRTY_OBJECT,
        DIRTY_WORLD
    };
    
    Dirty m_Dirty;
};


} // namespace poser


#endif
