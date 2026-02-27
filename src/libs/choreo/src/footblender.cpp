/*
 * choreo/footblender.cpp
 */

#include <choreo/footblender.hpp>
#include <choreo/footblenddriver.hpp>
#include <choreo/utility.hpp>
#include <choreo/rig.hpp>
#include <choreo/partition.hpp>

#include <poser/pose.hpp>
#include <poser/joint.hpp>

#include <p3d/anim/skeleton.hpp>
#include <p3d/error.hpp>

#include <string.h>


namespace choreo
{


FootBlender::FootBlender(Rig* rig,
                         RigLeg* rigLeg,
                         int maxBlendedDrivers,
                         int maxBlendedPriorities):
    m_IsIKEnabled(true),
    m_IsFootPlanted(false),
    m_PoleVector(0.0f, 0.0f, 0.0f),
    m_GroundFixupWeight(0.0f),
    m_MaxFootDrivers(maxBlendedDrivers),
    m_FootDriverCount(0),
    m_MaxBlendedPriorities(maxBlendedPriorities),
    m_EffectorTranslatePriorityCount(0),
    m_PoleVectorTranslatePriorityCount(0),
    m_EffectorRotatePriorityCount(0),
    m_FootPlantPriorityCount(0),
    m_GroundFixupPriorityCount(0)
{
    P3DASSERT(rig != 0);

    P3DASSERT(rigLeg != 0);
    m_RigLeg = rigLeg;
    m_RigLeg->AddRef();
    
    m_MinJointIndex = (m_RigLeg->GetEffectorJointIndex() <
                       m_RigLeg->GetPoleVectorJointIndex()) ?
                       m_RigLeg->GetEffectorJointIndex() :
                       m_RigLeg->GetPoleVectorJointIndex();

    m_FootDrivers = new FootDriverStruct [m_MaxFootDrivers];
    
    m_EffectorTranslatePriorities = new BlendPriority<rmt::Vector> [m_MaxBlendedPriorities];
    m_PoleVectorTranslatePriorities = new BlendPriority<rmt::Vector> [m_MaxBlendedPriorities];
    m_EffectorRotatePriorities = new BlendPriority<rmt::Quaternion> [m_MaxBlendedPriorities];
    m_FootPlantPriorities = new BlendPriority<float> [m_MaxBlendedPriorities];
    m_GroundFixupPriorities = new BlendPriority<float> [m_MaxBlendedPriorities];

    int orientOffsetIndex = m_RigLeg->GetOrientOffsetJointIndex();
    if (orientOffsetIndex >= 0)
    {
        tSkeleton* skel = rig->GetSkeleton();

        tSkeleton::Joint* orientOffsetJoint = skel->GetJoint(orientOffsetIndex);
        m_OrientOffset.BuildFromMatrix(orientOffsetJoint->restPose);
        m_ApplyOrientOffsetEffector = true;
        m_OrientOffsetInverse = orientOffsetJoint->restPose;
        m_OrientOffsetInverse.InvertOrtho();

        int pvIndex = m_RigLeg->GetPoleVectorJointIndex();
        if (pvIndex >= 0)
        {
            tSkeleton::Joint* pvJoint = skel->GetJoint(pvIndex);
            m_ApplyOrientOffsetPoleVector = (pvJoint->parentIndex == m_RigLeg->GetEffectorJointIndex());
        }
        else
        {
            m_ApplyOrientOffsetPoleVector = false;
        }
    }
    else
    {
        m_OrientOffset.Identity();
        m_ApplyOrientOffsetEffector = false;
        m_OrientOffsetInverse.Identity();
        m_ApplyOrientOffsetPoleVector = false;
    }
}

FootBlender::~FootBlender()
{
    for (int i = m_FootDriverCount - 1; i >= 0; --i)
    {
        m_FootDrivers[i].footDriver->Release();
    }
    
    delete[] m_GroundFixupPriorities;
    delete[] m_FootPlantPriorities;
    delete[] m_EffectorRotatePriorities;
    delete[] m_PoleVectorTranslatePriorities;
    delete[] m_EffectorTranslatePriorities;
    
    delete[] m_FootDrivers;

    m_RigLeg->Release();
}

bool FootBlender::Push(FootBlendDriver* footDriver)
{
    // check to see if footDriver is in current list
    // of previously pushed root drivers
    
    for (int i = 0; i < m_FootDriverCount; ++i)
    {
        FootDriverStruct* fDriver = (m_FootDrivers + i);
        
        if (fDriver->footDriver == footDriver)
        {
            // already in list, so set the "pushed" flag
            // for this frame and return
            fDriver->pushed = true;
            return true;
        }
    }
    
    // check to make sure we haven't maxed out the number of pushed drivers
    P3DASSERT(m_FootDriverCount < m_MaxFootDrivers);
    if (m_FootDriverCount >= m_MaxFootDrivers)
        return false;
    
    // append footDriver to list of currently pushed drivers
    FootDriverStruct* fDriver = (m_FootDrivers + m_FootDriverCount);
    
    fDriver->footDriver = footDriver;
    fDriver->footDriver->AddRef();
    
    fDriver->weight = 0.0f;
    fDriver->pushed = true;
    
    ++m_FootDriverCount;
    
    return true;
}

void FootBlender::Advance(float deltaTime)
{
    int i = 0;
    
    while (i < m_FootDriverCount)
    {
        FootDriverStruct* fDriver = (m_FootDrivers + i);
        FootBlendDriver* footDriver = fDriver->footDriver;
        
        if (fDriver->pushed)
        {
            // driver is easing in, or is maxed out
            float blendIn = footDriver->GetFootBlendInTime();
            
            if (blendIn <= 0.0f)
            {
                // blendIn == 0.0f means a pop
                fDriver->weight = 1.0f;
            }
            else
            {
                // ease in
                fDriver->weight += (deltaTime / blendIn);
                
                // clamp weight at 1.0f
                if (fDriver->weight > 1.0f)
                {
                    fDriver->weight = 1.0f;
                }
            }
            
            // clear pushed flag for next frame
            fDriver->pushed = false;
            
            ++i;
        }
        else
        {
            // driver is easing out
            float blendOut = footDriver->GetFootBlendOutTime();
            
            if (blendOut <= 0.0f)
            {
                // blendOut == 0.0f means a pop
                fDriver->weight = 0.0f;
            }
            else
            {
                // ease out
                fDriver->weight -= (deltaTime / blendOut);
            }
            
            if (fDriver->weight <= 0.0f)
            {
                // driver weight has dropped to zero; dump it!
                footDriver->Release();
                
                // dec driver count
                --m_FootDriverCount;
                
                // shift driver list over
                memmove(m_FootDrivers + i,
                        m_FootDrivers + i + 1,
                        (m_FootDriverCount - i) * sizeof(FootDriverStruct));
                
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

void FootBlender::UpdateEffectorTranslateDriver(FootBlendDriver* footDriver,
                                                float footWeight,
                                                int footPriority)
{
    BlendDriverNoContext
        <rmt::Vector, FootBlendDriver>
        (footDriver, &FootBlendDriver::GetFootEffectorTranslation, footWeight, footPriority,
         m_EffectorTranslatePriorities, m_MaxBlendedPriorities, m_EffectorTranslatePriorityCount);
}

void FootBlender::UpdateEffectorTranslate()
{
    rmt::Vector effTranslate;
    effTranslate.Clear();

    BlendPriorities<rmt::Vector>(m_EffectorTranslatePriorities,
                                 m_EffectorTranslatePriorityCount,
                                 effTranslate);

    m_Effector.SetTranslation(effTranslate);
}

void FootBlender::UpdatePoleVectorTranslateDriver(FootBlendDriver* footDriver,
                                                  float footWeight,
                                                  int footPriority)
{
    BlendDriverNoContext
        <rmt::Vector, FootBlendDriver>
        (footDriver, &FootBlendDriver::GetFootPoleVectorTranslation, footWeight, footPriority,
         m_PoleVectorTranslatePriorities, m_MaxBlendedPriorities, m_PoleVectorTranslatePriorityCount);
}

void FootBlender::UpdatePoleVectorTranslate()
{
    BlendPriorities<rmt::Vector>(m_PoleVectorTranslatePriorities,
                                 m_PoleVectorTranslatePriorityCount,
                                 m_PoleVector);

    // apply orient offset, if necessary
    if (m_ApplyOrientOffsetPoleVector)
    {
        rmt::Vector tmp;
        m_OrientOffsetInverse.Transform(m_PoleVector, &tmp);
        m_Effector.GetMatrix().Transform(tmp, &m_PoleVector);
    }
}

void FootBlender::UpdateEffectorRotateDriver(FootBlendDriver* footDriver,
                                             float footWeight,
                                             int footPriority)
{
    BlendDriverNoContext
        <rmt::Quaternion, FootBlendDriver>
        (footDriver, &FootBlendDriver::GetFootEffectorRotation, footWeight, footPriority,
         m_EffectorRotatePriorities, m_MaxBlendedPriorities, m_EffectorRotatePriorityCount);
}

void FootBlender::UpdateEffectorRotate()
{
    rmt::Quaternion effRotate;
    effRotate.Identity();

    BlendPriorities<rmt::Quaternion>(m_EffectorRotatePriorities,
                                     m_EffectorRotatePriorityCount,
                                     effRotate);

    // apply orient offset, if necessary
    if (m_ApplyOrientOffsetEffector)
    {
        effRotate.Mult(m_OrientOffset);
    }

    m_Effector.SetQuaternion(effRotate);
}

void FootBlender::UpdateFootPlantDriver(FootBlendDriver* footDriver,
                                        float footWeight,
                                        int footPriority)
{
    BlendDriverNoContext
        <float, FootBlendDriver>
        (footDriver, &FootBlendDriver::GetFootPlanted, footWeight, footPriority,
         m_FootPlantPriorities, m_MaxBlendedPriorities, m_FootPlantPriorityCount);
}

void FootBlender::UpdateFootPlant()
{
    float isFootPlanted = 0.0f;

    BlendPriorities<float>(m_FootPlantPriorities,
                           m_FootPlantPriorityCount,
                           isFootPlanted);

    m_IsFootPlanted = (isFootPlanted >= 0.5f);
}

void FootBlender::UpdateGroundFixupDriver(FootBlendDriver* footDriver,
                                          float footWeight,
                                          int footPriority)
{
    BlendDriverNoContext
        <float, FootBlendDriver>
        (footDriver, &FootBlendDriver::GetFootGroundFixup, footWeight, footPriority,
         m_GroundFixupPriorities, m_MaxBlendedPriorities, m_GroundFixupPriorityCount);
}

void FootBlender::UpdateGroundFixup()
{
    BlendPriorities<float>(m_GroundFixupPriorities,
                           m_GroundFixupPriorityCount,
                           m_GroundFixupWeight);
}

void FootBlender::Update(poser::Pose* pose)
{
    // initialize priority slots
    m_EffectorTranslatePriorityCount = 0;
    m_PoleVectorTranslatePriorityCount = 0;
    m_EffectorRotatePriorityCount = 0;
    m_FootPlantPriorityCount = 0;
    m_GroundFixupPriorityCount = 0;
    
    if (m_RigLeg->IsIKRigged() && m_IsIKEnabled)
    {
        // iterate over each driver
        for (int i = 0; i < m_FootDriverCount; ++i)
        {
            FootDriverStruct* fDriver = (m_FootDrivers + i);
            FootBlendDriver* footDriver = fDriver->footDriver;

            bool hasEffectorTranslate = footDriver->HasFootEffectorTranslation();
            bool hasEffectorRotate = footDriver->HasFootEffectorRotation();
            bool hasPVTranslate = footDriver->HasFootPoleVectorTranslation();
            bool hasPlanted = footDriver->HasFootPlanted();
            bool hasGroundFixup = footDriver->HasFootGroundFixup();

            if (hasEffectorTranslate ||
                hasEffectorRotate ||
                hasPVTranslate ||
                hasPlanted ||
                hasGroundFixup)
            {
                float footWeight = (fDriver->weight * footDriver->GetFootWeight());
            
                if (footWeight > 0.0f)
                {
                    int footPriority = footDriver->GetFootPriority();
                
                    // fill in and blend within priority slots
                    if (hasEffectorTranslate)
                    {
                        UpdateEffectorTranslateDriver(footDriver, footWeight, footPriority);
                    }
                    if (hasEffectorRotate)
                    {
                        UpdateEffectorRotateDriver(footDriver, footWeight, footPriority);
                    }
                    if (hasPVTranslate)
                    {
                        UpdatePoleVectorTranslateDriver(footDriver, footWeight, footPriority);
                    }
                    if (hasPlanted)
                    {
                        UpdateFootPlantDriver(footDriver, footWeight, footPriority);
                    }
                    if (hasGroundFixup)
                    {
                        UpdateGroundFixupDriver(footDriver, footWeight, footPriority);
                    }
                }
            }
        }
        
        // blend across priority slots
        UpdateEffectorTranslate();
        UpdatePoleVectorTranslate();
        UpdateEffectorRotate();
        UpdateFootPlant();
        UpdateGroundFixup();

        // stuff final transforms into joints
        if (m_RigLeg->GetEffectorJointIndex() >= 0)
        {
            poser::Joint* joint = pose->GetJoint(m_RigLeg->GetEffectorJointIndex());
            joint->SetObjectTransform(m_Effector);
        }
        if (m_RigLeg->GetPoleVectorJointIndex() >= 0)
        {
            poser::Joint* joint = pose->GetJoint(m_RigLeg->GetPoleVectorJointIndex());

            if (m_ApplyOrientOffsetPoleVector)
            {
                rmt::Vector tmp;
                rmt::Matrix m = m_Effector.GetMatrix();
                m.InvertOrtho();
                m.Transform(m_PoleVector, &tmp);
                joint->SetObjectTranslation(tmp);
            }
            else
            {
                joint->SetObjectTranslation(m_PoleVector);
            }
        }
    }
    else
    {
        // iterate over each driver
        for (int i = 0; i < m_FootDriverCount; ++i)
        {
            FootDriverStruct* fDriver = (m_FootDrivers + i);
            FootBlendDriver* footDriver = fDriver->footDriver;

            bool hasPlant = footDriver->HasFootPlanted();
            
            if (hasPlant)
            {
                float footWeight = (fDriver->weight * footDriver->GetFootWeight());
            
                if (footWeight > 0.0f)
                {
                    int footPriority = footDriver->GetFootPriority();
                
                    // fill in and blend within priority slots
                    UpdateFootPlantDriver(footDriver, footWeight, footPriority);
                }
            }
        }
        
        // blend across priority slots
        UpdateFootPlant();
        
        m_GroundFixupWeight = 0.0f;
    }
}


} // namespace choreo


// End of file.
