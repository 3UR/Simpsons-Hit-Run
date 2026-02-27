/*
 * poser/poseengine.cpp
 */

#include <poser/poseengine.hpp>
#include <poser/joint.hpp>
#include <poser/posedriver.hpp>

#include <string.h>

#include <p3d/error.hpp>
#include <p3d/anim/pose.hpp>
#include <p3d/anim/skeleton.hpp>



namespace poser
{


PoseEngine::PoseEngine(tPose* p3dPose,
                       int passCount,
                       int maxPoseDriversPerPass):
    m_PassCount(passCount),
    m_MaxPoseDriversPerPass(maxPoseDriversPerPass)
{
    m_Pose = new Pose(p3dPose);
    m_Pose->AddRef();
    
    m_P3DPose = p3dPose;
    m_P3DPose->AddRef();
    
    m_PoseDriverCounts = new int [m_PassCount];
    m_PoseDrivers = new PoseDriver** [m_PassCount];
    
    for (int pass = 0; pass < m_PassCount; ++pass)
    {
        m_PoseDriverCounts[pass] = 0;
        m_PoseDrivers[pass] = new PoseDriver* [m_MaxPoseDriversPerPass];
    }
}

PoseEngine::~PoseEngine()
{
    for (int pass = m_PassCount - 1; pass >= 0; --pass)
    {
        for (int i = m_PoseDriverCounts[pass] - 1; i >= 0; --i)
        {
            m_PoseDrivers[pass][i]->Release();
        }
        
        delete[] m_PoseDrivers[pass];
    }
    
    delete[] m_PoseDrivers;
    delete[] m_PoseDriverCounts;
    
    m_P3DPose->Release();
    m_Pose->Release();
}

void PoseEngine::SetP3DPose(tPose* p) 
{
    tRefCounted::Assign(m_P3DPose, p); 
    tRefCounted::Assign(m_Pose, new Pose(m_P3DPose));
}

int PoseEngine::GetPoseDriverCount(int pass) const
{
    P3DASSERT((pass >= 0) && (pass < m_PassCount));
    if ((pass < 0) || (pass >= m_PassCount))
        return 0;
    
    return m_PoseDriverCounts[pass];
}

PoseDriver* PoseEngine::GetPoseDriver(int pass, int i) const
{
    P3DASSERT((pass >= 0) && (pass < m_PassCount));
    if ((pass < 0) || (pass >= m_PassCount))
        return 0;
    
    P3DASSERT((i >= 0) && (i < m_PoseDriverCounts[pass]));
    if ((i < 0) || (i >= m_PoseDriverCounts[pass]))
        return 0;
    
    return m_PoseDrivers[pass][i];
}

int PoseEngine::GetPoseDriverIndex(int pass, PoseDriver* poseDriver) const
{
    // verify pass index
    P3DASSERT((pass >= 0) && (pass < m_PassCount));
    if ((pass < 0) || (pass >= m_PassCount))
        return 0;
    
    // search for driver
    for (int i = 0; i < m_PoseDriverCounts[pass]; ++i)
    {
        if (m_PoseDrivers[pass][i] == poseDriver)
            return i;
    }
    
    // didn't find driver
    return -1;
}

bool PoseEngine::AddPoseDriver(int pass, PoseDriver* poseDriver)
{
    // make sure poseDriver is valid
    P3DASSERT(poseDriver != 0);
    if (poseDriver == 0)
        return false;
    
    // make sure pass is valid
    P3DASSERT((pass >= 0) && (pass < m_PassCount));
    if ((pass < 0) || (pass >= m_PassCount))
        return false;
    
    // make sure we have space for another driver
    P3DASSERT(m_PoseDriverCounts[pass] < m_MaxPoseDriversPerPass);
    if (m_PoseDriverCounts[pass] >= m_MaxPoseDriversPerPass)
        return false;
    
    int driverMinJoint = poseDriver->GetMinimumJointIndex();
    int driverPriority = poseDriver->GetPriority();
    
    // search for correct spot for driver
    int i;
    for (i = 0; i < m_PoseDriverCounts[pass]; ++i)
    {
        PoseDriver* curDriver = m_PoseDrivers[pass][i];
        int curDriverMinJoint = curDriver->GetMinimumJointIndex();
        
        if (driverMinJoint < curDriverMinJoint)
        {
            break;
        }
        else if (driverMinJoint == curDriverMinJoint)
        {
            int curDriverPriority = curDriver->GetPriority();
            
            if (driverPriority < curDriverPriority)
            {
                break;
            }
        }
    }
    
    // shift array contents
    memmove(m_PoseDrivers[pass] + i + 1,
            m_PoseDrivers[pass] + i,
            (m_PoseDriverCounts[pass] - i) * sizeof(PoseDriver*));
    
    // increment pose driver count
    ++(m_PoseDriverCounts[pass]);
    
    // insert pose driver
    m_PoseDrivers[pass][i] = poseDriver;
    m_PoseDrivers[pass][i]->AddRef();
    
    return true;
}

bool PoseEngine::RemovePoseDriver(int pass, int i)
{
    // verify pass index
    P3DASSERT((pass >= 0) && (pass < m_PassCount));
    if ((pass < 0) || (pass >= m_PassCount))
        return false;
    
    // verify driver index
    P3DASSERT((i >= 0) && (i < m_PoseDriverCounts[pass]));
    if ((i < 0) || (i >= m_PoseDriverCounts[pass]))
        return false;
    
    // release driver
    m_PoseDrivers[pass][i]->Release();
    
    // dec driver count
    --(m_PoseDriverCounts[pass]);
    
    // shift array contents
    memmove(m_PoseDrivers[pass] + i,
            m_PoseDrivers[pass] + i + 1,
            (m_PoseDriverCounts[pass] - i) * sizeof(PoseDriver*));
    
    return true;
}

bool PoseEngine::RemovePoseDriver(int pass, PoseDriver* poseDriver)
{
    int driverIndex = GetPoseDriverIndex(pass, poseDriver);
    if (driverIndex < 0)
        return false;
    
    return RemovePoseDriver(pass, driverIndex);
}

void PoseEngine::Begin(bool resetToRestPose)
{
    if (resetToRestPose)
    {
        m_Pose->SetRestPose();
    }
    else
    {
        m_Pose->Assign(m_P3DPose);
    }
}

void PoseEngine::Advance(int pass, float dt)
{
    P3DASSERT((pass >= 0) && (pass < m_PassCount));
    if ((pass < 0) || (pass >= m_PassCount))
        return;
    
    // perform driver advances
    for (int i = 0;  i < m_PoseDriverCounts[pass]; ++i)
    {
        if (m_PoseDrivers[pass][i]->IsEnabled())
        {
            m_PoseDrivers[pass][i]->Advance(dt);
        }
    }
}

void PoseEngine::Update(int pass)
{
    P3DASSERT((pass >= 0) && (pass < m_PassCount));
    if ((pass < 0) || (pass >= m_PassCount))
        return;
    
    // perform driver updates
    for (int i = 0; i < m_PoseDriverCounts[pass]; ++i)
    {
        if (m_PoseDrivers[pass][i]->IsEnabled())
        {
            m_PoseDrivers[pass][i]->Update(m_Pose);
        }
    }
}

void PoseEngine::End()
{
    m_Pose->FillP3DPose(m_P3DPose, false);
}


} // namespace poser


// End of file.
