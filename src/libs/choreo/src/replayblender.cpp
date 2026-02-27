/*
 * choreo/replayblender.cpp
 */

#include <choreo/replayblender.hpp>
#include <choreo/replayblenddriver.hpp>
#include <choreo/utility.hpp>

#include <poser/pose.hpp>
#include <poser/joint.hpp>
#include <poser/transform.hpp>

#include <string.h>

#include <p3d/error.hpp>


namespace choreo
{


ReplayBlender::ReplayBlender(int maxBlendedDrivers,
                             int maxBlendedPriorities):
    m_MinimumJointIndex(0),
    m_MaxReplayDrivers(maxBlendedDrivers),
    m_ReplayDriverCount(0),
    m_MaxBlendedPriorities(maxBlendedPriorities)
{
    m_ReplayDrivers = new ReplayDriverStruct [m_MaxReplayDrivers];
    m_Priorities = new BlendPriority<poser::Transform> [m_MaxBlendedPriorities];
}

ReplayBlender::~ReplayBlender()
{
    for (int i = m_ReplayDriverCount - 1; i >= 0; --i)
    {
        m_ReplayDrivers[i].replayDriver->Release();
    }
    
    delete[] m_Priorities;
    delete[] m_ReplayDrivers;
}

void ReplayBlender::SetMinimumJointIndex(int jointIndex)
{
    m_MinimumJointIndex = jointIndex;
    
    if (m_MinimumJointIndex < 0)
    {
        m_MinimumJointIndex = 0;
    }
}

bool ReplayBlender::Push(ReplayBlendDriver* replayDriver)
{
    // check to see if replayDriver is in current list
    // of previously pushed replay drivers
    
    for (int i = 0; i < m_ReplayDriverCount; ++i)
    {
        ReplayDriverStruct* rDriver = (m_ReplayDrivers + i);
        
        if (rDriver->replayDriver == replayDriver)
        {
            // already in list, so set the "pushed" flag
            // for this frame and return
            rDriver->pushed = true;
            return true;
        }
    }
    
    // check to make sure we haven't maxed out the number of pushed drivers
    P3DASSERT(m_ReplayDriverCount < m_MaxReplayDrivers);
    if (m_ReplayDriverCount >= m_MaxReplayDrivers)
        return false;
    
    // append replayDriver to list of currently pushed drivers
    ReplayDriverStruct* rDriver = (m_ReplayDrivers + m_ReplayDriverCount);
    
    rDriver->replayDriver = replayDriver;
    rDriver->replayDriver->AddRef();
    
    rDriver->weight = 0.0f;
    rDriver->pushed = true;
    
    ++m_ReplayDriverCount;
    
    // inform replayDriver that it has been newly pushed
    replayDriver->Reset();
    
    return true;
}

void ReplayBlender::Advance(float dt)
{
    for (int i = 0; i < m_ReplayDriverCount; ++i)
    {
        ReplayDriverStruct* rDriver = (m_ReplayDrivers + i);
        ReplayBlendDriver* replayDriver = rDriver->replayDriver;
        
        // advance currently pushed drivers
        // (whether easing in or out)
        replayDriver->Advance(dt);
    }
}

void ReplayBlender::Update(poser::Pose* pose)
{
    // exit early if we're not doing anything
    if (m_ReplayDriverCount <= 0)
        return;

    int jointCount = pose->GetJointCount();
    
    // iterate over each joint
    for (int j = m_MinimumJointIndex; j < jointCount; ++j)
    {
        poser::Joint* joint = pose->GetJoint(j);

        // iterate over each recording driver
        for (int i = 0; i < m_ReplayDriverCount; ++i)
        {
            ReplayDriverStruct* rDriver = (m_ReplayDrivers + i);
            ReplayBlendDriver* replayDriver = rDriver->replayDriver;

            if (replayDriver->HasJoint(j))
            {
                replayDriver->RecordJointTransform(j, joint->GetWorldTransform());
            }
        }
    }

    // update recording drivers
    for (int i = 0; i < m_ReplayDriverCount; ++i)
    {
        ReplayDriverStruct* rDriver = (m_ReplayDrivers + i);
        ReplayBlendDriver* replayDriver = rDriver->replayDriver;

        replayDriver->Update();
    }
}

void ReplayBlender::AdvanceReplay(float dt)
{
    int i = 0;
    
    while (i < m_ReplayDriverCount)
    {
        ReplayDriverStruct* rDriver = (m_ReplayDrivers + i);
        ReplayBlendDriver* replayDriver = rDriver->replayDriver;
        
        // advance currently pushed drivers
        // (whether easing in or out)
        replayDriver->AdvanceReplay(dt);
        
        if (rDriver->pushed)
        {
            // driver is easing in, or is maxed out
            float blendIn = replayDriver->GetBlendInTime();
            
            if (blendIn <= 0.0f)
            {
                // blendIn == 0.0f means a pop
                rDriver->weight = 1.0f;
            }
            else
            {
                // ease in
                rDriver->weight += (dt / blendIn);
                
                // clamp weight at 1.0f
                if (rDriver->weight > 1.0f)
                {
                    rDriver->weight = 1.0f;
                }
            }
            
            // clear pushed flag for next frame
            rDriver->pushed = false;
            
            ++i;
        }
        else
        {
            // driver is easing out
            float blendOut = replayDriver->GetBlendOutTime();
            
            if (blendOut <= 0.0f)
            {
                // blendOut == 0.0f means a pop
                rDriver->weight = 0.0f;
            }
            else
            {
                // ease out
                rDriver->weight -= (dt / blendOut);
            }
            
            if (rDriver->weight <= 0.0f)
            {
                // driver weight has dropped to zero; dump it!
                replayDriver->Release();
                
                // dec driver count
                --m_ReplayDriverCount;
                
                // shift driver list over
                memmove(m_ReplayDrivers + i,
                        m_ReplayDrivers + i + 1,
                        (m_ReplayDriverCount - i) * sizeof(ReplayDriverStruct));
                
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

void ReplayBlender::UpdateDriver(int jointIndex,
                                 ReplayBlendDriver* replayDriver,
                                 float jointWeight,
                                 int jointPriority)
{
    BlendDriverWithContext
        <poser::Transform, int, ReplayBlendDriver>
        (jointIndex, replayDriver, &ReplayBlendDriver::GetJointTransform, jointWeight, jointPriority,
         m_Priorities, m_MaxBlendedPriorities, m_PriorityCount);
}

void ReplayBlender::UpdateJoint(poser::Joint* joint)
{
    poser::Transform transform;

    if (BlendPriorities<poser::Transform>(m_Priorities,
                                          m_PriorityCount,
                                          transform))
    {
        // stuff transform value into joint
        joint->SetWorldTransform(transform);
    }
}

void ReplayBlender::UpdateReplay(poser::Pose* pose)
{
    // exit early if we're not doing anything
    if (m_ReplayDriverCount <= 0)
        return;

    int jointCount = pose->GetJointCount();
    
    // iterate over each joint
    for (int j = m_MinimumJointIndex; j < jointCount; ++j)
    {
        poser::Joint* joint = pose->GetJoint(j);
        
        // initialize priority slots
        m_PriorityCount = 0;
        
        // iterate over each non-recording driver
        int i;
        for (i = 0; i < m_ReplayDriverCount; ++i)
        {
            ReplayDriverStruct* rDriver = (m_ReplayDrivers + i);
            ReplayBlendDriver* replayDriver = rDriver->replayDriver;

            if (replayDriver->HasJoint(j))
            {
                float weight = (rDriver->weight * replayDriver->GetJointWeight(j));
        
                if (weight > 0.0f)
                {
                    int jointPriority = replayDriver->GetJointPriority(j);
            
                    // fill in and blend within priority slots
                    UpdateDriver(j, replayDriver, weight, jointPriority);
                }
            }
        }
        
        // blend across priority slots, and stuff joint values
        UpdateJoint(joint);
    }
}


} // namespace choreo


// End of file.
