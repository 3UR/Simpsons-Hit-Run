//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


/*
 * poser/joint.cpp
 */

#include <poser/joint.hpp>

#include <p3d/error.hpp>


namespace poser
{


Joint::Joint():
    m_Index(0),
    m_ParentJoint(0),
    m_MaxChildJointCount(0),
    m_ChildJointCount(0),
    m_ChildJoints(0),
    m_Dirty(DIRTY_NONE)
{
}

Joint::Joint(const Joint& joint):
    m_Index(0),
    m_ParentJoint(0),
    m_MaxChildJointCount(0),
    m_ChildJointCount(0),
    m_ChildJoints(0)
{
    *this = joint;
}

Joint::~Joint()
{
}

Joint& Joint::operator = (const Joint& joint)
{
    m_Object = joint.m_Object;
    m_World = joint.m_World;
    m_Dirty = joint.m_Dirty;
    
    return *this;
}

void Joint::SetChildJointCount(int count)
{
    P3DASSERT(m_ChildJoints == 0);
    m_ChildJointCount = count;
}

void Joint::SetChildJointMem(Joint** mem)
{
    m_ChildJoints = mem;
    m_ChildJointCount = 0;
}

Joint* Joint::GetChildJoint(int i)
{
    P3DASSERT((i >= 0) && (i < m_ChildJointCount));
    if ((i < 0) || (i >= m_ChildJointCount))
        return 0;
    
    return m_ChildJoints[i];
}

const Joint* Joint::GetChildJoint(int i) const
{
    P3DASSERT((i >= 0) && (i < m_ChildJointCount));
    if ((i < 0) || (i >= m_ChildJointCount))
        return 0;
    
    return m_ChildJoints[i];
}

bool Joint::AddChildJoint(Joint* child)
{
    /*
    // allocate array, if necessary
    if (m_ChildJoints == 0)
    {
        P3DASSERT(m_ChildJointCount > 0);
        m_ChildJoints = new Joint* [m_ChildJointCount];
        m_ChildJointCount = 0;
    }
    */

    // append child joint pointer
    m_ChildJoints[m_ChildJointCount] = child;
    ++m_ChildJointCount;
    
    return true;
}

const Transform& Joint::GetObjectTransform() const
{
    if (m_Dirty == DIRTY_OBJECT)
    {
        Joint* cthis = const_cast<Joint*>(this);
        
        if (m_ParentJoint != 0)
        {
            Transform invPWorld = m_ParentJoint->GetWorldTransform();
            invPWorld.Invert();
            
            cthis->m_Object = m_World;
            cthis->m_Object.Mult(invPWorld);
        }
        
        cthis->m_Dirty = DIRTY_NONE;
    }
    
    return m_Object;
}

void Joint::SetObjectTransform(const Transform& xForm)
{
    m_Object = xForm;
    m_Dirty = DIRTY_WORLD;
    NotifyWorldModified();
}

void Joint::SetObjectMatrix(const rmt::Matrix& matrix)
{
    GetObjectTransform();
    m_Object.SetMatrix(matrix);
    m_Dirty = DIRTY_WORLD;
    NotifyWorldModified();
}

void Joint::SetObjectMatrixRotation(const rmt::Matrix& matrix)
{
    GetObjectTransform();
    m_Object.SetMatrixRotation(matrix);
    m_Dirty = DIRTY_WORLD;
    NotifyWorldModified();
}

void Joint::SetObjectTranslation(const rmt::Vector& trans)
{
    GetObjectTransform();
    m_Object.SetTranslation(trans);
    m_Dirty = DIRTY_WORLD;
    NotifyWorldModified();
}

void Joint::SetObjectQuaternion(const rmt::Quaternion& quat)
{
    GetObjectTransform();
    m_Object.SetQuaternion(quat);
    m_Dirty = DIRTY_WORLD;
    NotifyWorldModified();
}

const Transform& Joint::GetWorldTransform() const
{
    if (m_Dirty == DIRTY_WORLD)
    {
        Joint* cthis = const_cast<Joint*>(this);
        
        if (m_ParentJoint != 0)
        {
            cthis->m_World = m_Object;
            cthis->m_World.Mult(m_ParentJoint->GetWorldTransform());
            
            NotifyWorldModified();
        }
        
        cthis->m_Dirty = DIRTY_NONE;
    }
    
    return m_World;
}

void Joint::SetWorldTransform(const Transform& xForm)
{
    m_World = xForm;
    m_Dirty = DIRTY_OBJECT;
    NotifyWorldModified();
}

void Joint::NotifyWorldModified() const
{
    for (int i = 0; i < m_ChildJointCount; ++i)
    {
        m_ChildJoints[i]->ParentWorldModified();
    }
}

void Joint::ParentWorldModified()
{
#if (defined(RAD_DEBUG) || defined(RAD_TUNE))
    // It is a driver ordering error if an object matrix
    // is dirty when its parent joint's world matrix has
    // been modified!
    if (m_Dirty == DIRTY_OBJECT)
    {
        P3DASSERT("poseEngine::Joint error!  Child object transform dirty "
                  "during parent world transform modification.  Your "
                  "PoseDrivers are probably sorted incorrectly." == 0);
        return;
    }
#endif
    
    if (m_Dirty != DIRTY_NONE)
        return;
    
    m_Dirty = DIRTY_WORLD;
    
    for (int i = 0; i < m_ChildJointCount; ++i)
    {
        m_ChildJoints[i]->ParentWorldModified();
    }
}

void Joint::SetWorldMatrix(const rmt::Matrix& matrix)
{
    GetWorldTransform();
    m_World.SetMatrix(matrix);
    m_Dirty = DIRTY_OBJECT;
    NotifyWorldModified();
}

void Joint::SetWorldMatrixRotation(const rmt::Matrix& matrix)
{
    GetWorldTransform();
    m_World.SetMatrixRotation(matrix);
    m_Dirty = DIRTY_OBJECT;
    NotifyWorldModified();
}

void Joint::SetWorldTranslation(const rmt::Vector& trans)
{
    GetWorldTransform();
    m_World.SetTranslation(trans);
    m_Dirty = DIRTY_OBJECT;
    NotifyWorldModified();
}

void Joint::SetWorldQuaternion(const rmt::Quaternion& quat)
{
    GetWorldTransform();
    m_World.SetQuaternion(quat);
    m_Dirty = DIRTY_OBJECT;
    NotifyWorldModified();
}

void Joint::Clean()
{
    m_Dirty = DIRTY_NONE;
}


} // namespace poser


// End of file.
