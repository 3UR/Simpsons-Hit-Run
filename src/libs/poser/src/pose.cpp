/*
 * poser/pose.cpp
 */

#include <poser/pose.hpp>
#include <poser/joint.hpp>

#include <p3d/error.hpp>
#include <p3d/anim/skeleton.hpp>
#include <p3d/anim/pose.hpp>


namespace poser
{


Pose::Pose():
    m_Skeleton(0),
    m_Joints(0),
    m_Children(0)
{
}

Pose::Pose(tSkeleton* skeleton)
{
    P3DASSERT(skeleton != 0);
    
    if (skeleton != 0)
    {
        m_Skeleton = skeleton;
        m_Skeleton->AddRef();
        
        int jointCount = m_Skeleton->GetNumJoint();
        m_Joints = new Joint [jointCount];
        m_Children = new Joint* [jointCount];
        
        int j;
        // count child joints
        for (j = 1; j < jointCount; ++j)
        {
            m_Joints[j].SetIndex(j);

            int parentIndex = m_Skeleton->GetJoint(j)->parentIndex;
            P3DASSERT((parentIndex >= 0) && (parentIndex < j));
            Joint* parentJoint = (m_Joints + parentIndex);
            m_Joints[j].SetParentJoint(parentJoint);
            parentJoint->SetChildJointCount(parentJoint->GetChildJointCount() + 1);
        }
        
        int useChild = 0;
        for (j = 0; j < jointCount; ++j)
        {
            if(m_Joints[j].GetChildJointCount())
            {
                int old = m_Joints[j].GetChildJointCount();
                m_Joints[j].SetChildJointMem(&m_Children[useChild]);
                useChild += old ;
            }
        }

        // update parent child joint list
        for (j = 1; j < jointCount; ++j)
        {
            int parentIndex = m_Skeleton->GetJoint(j)->parentIndex;
            P3DASSERT((parentIndex >= 0) && (parentIndex < j));
            Joint* parentJoint = (m_Joints + parentIndex);
            parentJoint->AddChildJoint(m_Joints + j);
        }
    }
    else
    {
        m_Skeleton = 0;
        m_Joints = 0;
    }
}

Pose::Pose(Pose* pose):
    m_Skeleton(0),
    m_Joints(0)
{
    Assign(pose);
}

Pose::Pose(tPose* p3dPose):
    m_Skeleton(0),
    m_Joints(0)
{
    Assign(p3dPose);
}

Pose::~Pose()
{
    delete[] m_Joints;
    delete[] m_Children;
    
    if (m_Skeleton != 0)
    {
        m_Skeleton->Release();
    }
}

void Pose::Assign(Pose* pose)
{
    tSkeleton *skel = pose->m_Skeleton;
    
    if (skel != m_Skeleton)
    {
        tRefCounted::Assign(m_Skeleton, skel);
        
        delete[] m_Joints;
        int jointCount = m_Skeleton->GetNumJoint();
        
        if (jointCount > 0)
        {
            m_Joints = new Joint [jointCount];
            m_Children = new Joint* [jointCount];
            
            m_Joints[0] = pose->m_Joints[0];
            
            int j;
            
            // count child joints & assign
            for (j = 1; j < jointCount; ++j)
            {
                m_Joints[j].SetIndex(j);

                int parentIndex = m_Skeleton->GetJoint(j)->parentIndex;
                P3DASSERT((parentIndex >= 0) && (parentIndex < j));
                Joint* parentJoint = (m_Joints + parentIndex);
                m_Joints[j].SetParentJoint(parentJoint);
                parentJoint->SetChildJointCount(parentJoint->GetChildJointCount() + 1);
                m_Joints[j] = pose->m_Joints[j];
            }
            
            int useChild = 0;
            for (j = 0; j < jointCount; ++j)
            {
                if(m_Joints[j].GetChildJointCount())
                {
                    int old = m_Joints[j].GetChildJointCount();
                    m_Joints[j].SetChildJointMem(&m_Children[useChild]);
                    useChild += old ;
                }
            }

            // update parent child joint list
            for (j = 1; j < jointCount; ++j)
            {
                int parentIndex = m_Skeleton->GetJoint(j)->parentIndex;
                P3DASSERT((parentIndex >= 0) && (parentIndex < j));
                Joint* parentJoint = (m_Joints + parentIndex);
                parentJoint->AddChildJoint(m_Joints + j);
            }
        }
        else
        {
            m_Joints = 0;
        }
    }
    else
    {
        int jointCount = GetJointCount();
        
        for (int j = 0; j < jointCount; ++j)
        {
            m_Joints[j] = pose->m_Joints[j];
        }
    }
}

void Pose::Assign(tPose* p3dPose)
{
    tSkeleton *skel = p3dPose->GetSkeleton();
    
    if (skel != m_Skeleton)
    {
        tRefCounted::Assign(m_Skeleton, skel);
        
        delete[] m_Joints;
        int jointCount = m_Skeleton->GetNumJoint();
        
        if (jointCount > 0)
        {
            m_Joints = new Joint [jointCount];
            m_Children = new Joint* [jointCount];
            
            m_Joints[0].SetObjectTransform(Transform(p3dPose->GetJoint(0)->objectMatrix));
            
            int j;
            
            // count parent child joints
            for (j = 1; j < jointCount; ++j)
            {
                m_Joints[j].SetIndex(j);

                int parentIndex = m_Skeleton->GetJoint(j)->parentIndex;
                P3DASSERT((parentIndex >= 0) && (parentIndex < j));
                Joint* parentJoint = (m_Joints + parentIndex);
                m_Joints[j].SetParentJoint(parentJoint);
                parentJoint->SetChildJointCount(parentJoint->GetChildJointCount() + 1);
                
                m_Joints[j].SetObjectTransform(Transform(p3dPose->GetJoint(j)->objectMatrix));
            }
            
            int useChild = 0;
            for (j = 0; j < jointCount; ++j)
            {
                if(m_Joints[j].GetChildJointCount())
                {
                    int old = m_Joints[j].GetChildJointCount();
                    m_Joints[j].SetChildJointMem(&m_Children[useChild]);
                    useChild += old ;
                }
            }

            // update parent child joints
            for (j = 1; j < jointCount; ++j)
            {
                int parentIndex = m_Skeleton->GetJoint(j)->parentIndex;
                P3DASSERT((parentIndex >= 0) && (parentIndex < j));
                Joint* parentJoint = (m_Joints + parentIndex);
                parentJoint->AddChildJoint(m_Joints + j);
            }
        }
        else
        {
            m_Joints = 0;
        }
    }
    else
    {
        int jointCount = GetJointCount();
    
        // initialize pose
        for (int i = 0; i < jointCount; ++i)
        {
            m_Joints[i].m_Object.SetMatrix(p3dPose->GetJoint(i)->objectMatrix);
            m_Joints[i].m_Dirty = Joint::DIRTY_WORLD;
        }
    }
}

int Pose::GetJointCount() const
{
    if (m_Skeleton == 0)
        return 0;
    return m_Skeleton->GetNumJoint();
}

void Pose::SetRestPose()
{
    int jointCount = GetJointCount();
    
    // initialize pose
    for (int i = 0; i < jointCount; ++i)
    {
        m_Joints[i].m_Object.SetMatrix(m_Skeleton->GetJoint(i)->restPose);
        m_Joints[i].m_Dirty = Joint::DIRTY_WORLD;
    }

}

void Pose::FillP3DPose(tPose* p3dPose, bool fillObjectMatrices) const
{
    // make sure these are compatible poses
    // Not no more we dont (nbrooke 29/1/03)
//    P3DASSERT(p3dPose->GetSkeleton() == m_Skeleton);
//    if (p3dPose->GetSkeleton() != m_Skeleton)
//        return;
    
    int jointCount = GetJointCount();
    
    if (fillObjectMatrices)
    {
        // update output pose
        for (int i = 0; i < jointCount; ++i)
        {
            // update pose joint
            const Joint* joint = (m_Joints + i);
            tPose::Joint* p3dJoint = p3dPose->GetJoint(i);
        
            p3dJoint->objectMatrix = joint->GetObjectTransform().GetMatrix();
            p3dJoint->worldMatrix = joint->GetWorldTransform().GetMatrix();
        }
    }
    else
    {
        // update output pose
        for (int i = 0; i < rmt::Min(jointCount, p3dPose->GetNumJoint());  ++i)
        {
            // update pose joint
            const Joint* joint = (m_Joints + i);
            tPose::Joint* p3dJoint = p3dPose->GetJoint(i);
        
            p3dJoint->worldMatrix = joint->GetWorldTransform().GetMatrix();
        }
    }

    p3dPose->SetPoseReady(true);
}

void Pose::Clean()
{
    int jointCount = GetJointCount();
    for (int i = 0; i < jointCount; ++i)
    {
        m_Joints[i].Clean();
    }
}


} // namespace poser


// End of file.
