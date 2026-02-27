/*
 * choreo/jointblender.cpp
 */

#include <choreo/jointblender.hpp>
#include <choreo/jointblenddriver.hpp>
#include <choreo/utility.hpp>
#include <choreo/partition.hpp>

#include <poser/pose.hpp>
#include <poser/joint.hpp>
#include <poser/transform.hpp>

#include <string.h>

#include <p3d/anim/skeleton.hpp>
#include <p3d/error.hpp>


namespace choreo
{


JointBlender::JointBlender(int maxBlendedDrivers,
                           int maxBlendedPriorities,
                           int maxPropCount):
    m_MinimumJointIndex(0),
    m_JointDriverCount(0)
{
}

JointBlender::~JointBlender()
{
    for (int i = m_JointDriverCount - 1; i >= 0; --i)
    {
        m_JointDrivers[i].jointDriver->Release();
    }
}

void JointBlender::SetMinimumJointIndex(int jointIndex)
{
    m_MinimumJointIndex = jointIndex;
    
    if (m_MinimumJointIndex < 0)
    {
        m_MinimumJointIndex = 0;
    }
}

bool JointBlender::Push(JointBlendDriver* jointDriver)
{
    // check to see if jointDriver is in current list
    // of previously pushed joint drivers
    
    for (int i = 0; i < m_JointDriverCount; ++i)
    {
        JointDriverStruct* jDriver = (m_JointDrivers + i);
        
        if (jDriver->jointDriver == jointDriver)
        {
            // already in list, so set the "pushed" flag
            // for this frame and return
            jDriver->pushed = true;
            return true;
        }
    }
    
    // check to make sure we haven't maxed out the number of pushed drivers
//    P3DASSERT(m_JointDriverCount < DEFAULT_MAX_DRIVER_COUNT);
    if (m_JointDriverCount >= DEFAULT_MAX_DRIVER_COUNT)
        return false;
    
    // append jointDriver to list of currently pushed drivers
    JointDriverStruct* jDriver = (m_JointDrivers + m_JointDriverCount);
    
    jDriver->jointDriver = jointDriver;
    jDriver->jointDriver->AddRef();
    
    jDriver->weight = 0.0f;
    jDriver->pushed = true;
    
    ++m_JointDriverCount;
    
    // inform jointDriver that it has been newly pushed
    jointDriver->Reset();
    
    return true;
}

void JointBlender::Advance(float dt)
{
    int i = 0;
    
    while (i < m_JointDriverCount)
    {
        JointDriverStruct* jDriver = (m_JointDrivers + i);
        JointBlendDriver* jointDriver = jDriver->jointDriver;
        
        // advance currently pushed drivers
        // (whether easing in or out)
        jointDriver->Advance(dt);
        
        if (jDriver->pushed)
        {
            // driver is easing in, or is maxed out
            float blendIn = jointDriver->GetBlendInTime();
            
            if (blendIn <= 0.0f)
            {
                // blendIn == 0.0f means a pop
                jDriver->weight = 1.0f;
            }
            else
            {
                // ease in
                jDriver->weight += (dt / blendIn);
                
                // clamp weight at 1.0f
                if (jDriver->weight > 1.0f)
                {
                    jDriver->weight = 1.0f;
                }
            }
            
            // clear pushed flag for next frame
            jDriver->pushed = false;
            
            ++i;
        }
        else
        {
            // driver is easing out
            float blendOut = jointDriver->GetBlendOutTime();
            
            if (blendOut <= 0.0f)
            {
                // blendOut == 0.0f means a pop
                jDriver->weight = 0.0f;
            }
            else
            {
                // ease out
                jDriver->weight -= (dt / blendOut);
            }
            
            if (jDriver->weight <= 0.0f)
            {
                // driver weight has dropped to zero; dump it!
                jointDriver->Release();
                
                // dec driver count
                --m_JointDriverCount;
                
                // shift driver list over
                memmove(m_JointDrivers + i,
                        m_JointDrivers + i + 1,
                        (m_JointDriverCount - i) * sizeof(JointDriverStruct));
                
                // note:  don't inc i!
            }
            else
            {
                // driver weight still above zero, so
                // we're still ramping down...
                ++i;
            }
        }
    }
}

void JointBlender::UpdateDriverTranslation(int jointIndex,
                                           JointBlendDriver* jointDriver,
                                           float jointWeight,
                                           int jointPriority)
{
    BlendDriverWithContext
        <rmt::Vector, int, JointBlendDriver>
        (jointIndex, jointDriver, &JointBlendDriver::GetJointTranslation, jointWeight, jointPriority,
         m_TranslatePriorities, DEFAULT_MAX_BLENDED_PRIORITIES, m_TranslatePriorityCount);
}

void JointBlender::UpdateDriverRotation(int jointIndex,
                                        JointBlendDriver* jointDriver,
                                        float jointWeight,
                                        int jointPriority)
{
    BlendDriverWithContext
        <rmt::Quaternion, int, JointBlendDriver>
        (jointIndex, jointDriver, &JointBlendDriver::GetJointRotation, jointWeight, jointPriority,
         m_RotatePriorities, DEFAULT_MAX_BLENDED_PRIORITIES, m_RotatePriorityCount);
}

bool JointBlender::UpdateTranslation(poser::Joint* joint)
{
    rmt::Vector translate;

    if (BlendPriorities<rmt::Vector>(m_TranslatePriorities,
                                     m_TranslatePriorityCount,
                                     translate))
    {
        // stuff translate value into joint
        joint->SetObjectTranslation(translate);
        return true;
    }

    return false;
}

bool JointBlender::UpdateRotation(poser::Joint* joint)
{
    rmt::Quaternion rotate;
    
    if (BlendPriorities<rmt::Quaternion>(m_RotatePriorities,
                                         m_RotatePriorityCount,
                                         rotate))
    {
        // stuff rotate value into joint
        joint->SetObjectQuaternion(rotate);
        return true;
    }

    return false;
}

void JointBlender::Update(poser::Pose* pose)
{
    int jointCount = pose->GetJointCount();
    tSkeleton* skeleton = pose->GetSkeleton();
    poser::Transform prevTransform;
    poser::Transform transform;
    
    // iterate over each joint
    for (int j = m_MinimumJointIndex; j < jointCount; ++j)
    {
        poser::Joint* joint = pose->GetJoint(j);
        tSkeleton::Joint* skelJoint = skeleton->GetJoint(j);

        bool preserveBoneLength = j != 3;//skelJoint->preserveBoneLengths;

        // initialize priority slots
        m_TranslatePriorityCount = 0;
        m_RotatePriorityCount = 0;

        if (preserveBoneLength)
        {
            // iterate over each driver
            for (int i = 0; i < m_JointDriverCount; ++i)
            {
                JointDriverStruct* jDriver = (m_JointDrivers + i);
                JointBlendDriver* jointDriver = jDriver->jointDriver;

                if (jointDriver->HasJointRotation(j))
                {
                    float weight = (jDriver->weight * jointDriver->GetJointWeight(j));

                    if (weight > 0.0f)
                    {
                        int jointPriority = jointDriver->GetJointPriority(j);

                        UpdateDriverRotation(j, jointDriver, weight, jointPriority);
                    }
                }
            }

            // override animations with rest pose bone length
            joint->SetObjectTranslation(skelJoint->restPose.Row(3));
        }
        else
        {
            // iterate over each driver
            for (int i = 0; i < m_JointDriverCount; ++i)
            {
                JointDriverStruct* jDriver = (m_JointDrivers + i);
                JointBlendDriver* jointDriver = jDriver->jointDriver;

                bool hasTranslate = jointDriver->HasJointTranslation(j);
                bool hasRotate = jointDriver->HasJointRotation(j);

                if (hasTranslate || hasRotate)
                {
                    float weight = (jDriver->weight * jointDriver->GetJointWeight(j));

                    if (weight > 0.0f)
                    {
                        int jointPriority = jointDriver->GetJointPriority(j);
                
                        // fill in and blend within priority slots
                        if (hasTranslate)
                        {
                            UpdateDriverTranslation(j, jointDriver, weight, jointPriority);
                        }

                        if (hasRotate)
                        {
                            UpdateDriverRotation(j, jointDriver, weight, jointPriority);
                        }
                    }
                }
            }

            // blend across priority slots, and stuff joint values
            if (!UpdateTranslation(joint))
            {
                // fallback to rest pose
                joint->SetObjectTranslation(skelJoint->restPose.Row(3));
            }
        }

        // blend across priority slots, and stuff joint values
        if (!UpdateRotation(joint))
        {
            joint->SetObjectMatrixRotation(skelJoint->restPose);
        }
    }
}

void JointBlender::UpdateGroupDriverTranslation(tUID group,
                                                JointBlendDriver* jointDriver,
                                                float jointWeight,
                                                int jointPriority)
{
    BlendDriverWithContext
        <rmt::Vector, tUID, JointBlendDriver>
        (group, jointDriver, &JointBlendDriver::GetGroupTranslation, jointWeight, jointPriority,
         m_TranslatePriorities, DEFAULT_MAX_BLENDED_PRIORITIES, m_TranslatePriorityCount);
}

void JointBlender::UpdateGroupDriverRotation(tUID group,
                                             JointBlendDriver* jointDriver,
                                             float jointWeight,
                                             int jointPriority)
{
    BlendDriverWithContext
        <rmt::Quaternion, tUID, JointBlendDriver>
        (group, jointDriver, &JointBlendDriver::GetGroupRotation, jointWeight, jointPriority,
         m_RotatePriorities, DEFAULT_MAX_BLENDED_PRIORITIES, m_RotatePriorityCount);
}

bool JointBlender::UpdateGroupTranslation(tPose::Joint* joint)
{
    rmt::Vector translate;

    if (BlendPriorities<rmt::Vector>(m_TranslatePriorities,
                                     m_TranslatePriorityCount,
                                     translate))
    {
        // stuff translate value into joint
        joint->objectMatrix.Row(3) = translate;
        return true;
    }

    return false;
}

bool JointBlender::UpdateGroupRotation(tPose::Joint* joint)
{
    rmt::Quaternion rotate;
    
    if (BlendPriorities<rmt::Quaternion>(m_RotatePriorities,
                                         m_RotatePriorityCount,
                                         rotate))
    {
        // stuff rotate value into joint
        rotate.ConvertToMatrix(&(joint->objectMatrix));
        return true;
    }

    return false;
}

bool JointBlender::UpdateTransform(tUID groupUID, poser::Transform& transform)
{
    // initialize priority slots
    m_TranslatePriorityCount = 0;
    m_RotatePriorityCount = 0;

    // iterate over each driver
    for (int i = 0; i < m_JointDriverCount; ++i)
    {
        JointDriverStruct* jDriver = (m_JointDrivers + i);
        JointBlendDriver* jointDriver = jDriver->jointDriver;

        bool hasTranslate = jointDriver->HasGroupTranslation(groupUID);
        bool hasRotate = jointDriver->HasGroupRotation(groupUID);

        if (hasTranslate || hasRotate)
        {
            float weight = (jDriver->weight * jointDriver->GetGroupWeight(groupUID));

            if (weight > 0.0f)
            {
                int groupPriority = jointDriver->GetGroupPriority(groupUID);
    
                // fill in and blend within priority slots
                if (hasTranslate)
                {
                    UpdateGroupDriverTranslation(groupUID, jointDriver, weight, groupPriority);
                }

                if (hasRotate)
                {
                    UpdateGroupDriverRotation(groupUID, jointDriver, weight, groupPriority);
                }
            }
        }
    }

    // blend across priority slots, and stuff joint values
    bool rc = false;

    rmt::Vector translate;
    if (BlendPriorities<rmt::Vector>(m_TranslatePriorities,
            m_TranslatePriorityCount,
            translate))
    {
        transform.SetTranslation(translate);
        rc = true;
    }

    rmt::Quaternion rotate;
    if (BlendPriorities<rmt::Quaternion>(m_RotatePriorities,
            m_RotatePriorityCount,
            rotate))
    {
        transform.SetQuaternion(rotate);
        rc = true;
    }

    return rc;
}


} // namespace choreo


// End of file.
