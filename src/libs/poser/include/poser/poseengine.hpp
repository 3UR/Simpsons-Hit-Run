//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


/*
 * poser/poseengine.hpp
 */

#ifndef POSER_POSEENGINE_HPP
#define POSER_POSEENGINE_HPP


#include <poser/pose.hpp>


// Pure3D classes
class tPose;


namespace poser
{


class PoseDriver;


class PoseEngine: public tRefCounted
{
public:

    enum
    {
        DEFAULT_PASS_COUNT = 4,
        DEFAULT_MAX_POSE_DRIVERS_PER_PASS = 16
    };
    
    PoseEngine(tPose* p3dPose,
               int passCount = DEFAULT_PASS_COUNT,
               int maxPoseDriversPerPass = DEFAULT_MAX_POSE_DRIVERS_PER_PASS);
    
    tPose* GetP3DPose()
        { return m_P3DPose; }
    
    void SetP3DPose(tPose* p);
    
    Pose* GetPose()
        { return m_Pose; }
    const Pose* GetPose() const
        { return m_Pose; }
    
    int GetPassCount() const
        { return m_PassCount; }
    
    int GetPoseDriverCount(int pass) const;
    PoseDriver* GetPoseDriver(int pass, int i) const;
    int GetPoseDriverIndex(int pass, PoseDriver* poseDriver) const;
    
    bool AddPoseDriver(int pass, PoseDriver* poseDriver);
    bool RemovePoseDriver(int pass, int i);
    bool RemovePoseDriver(int pass, PoseDriver* poseDriver);
    
    void Begin(bool resetToRestPose = true);
    void Advance(int pass, float deltaTime);  // deltaTime in seconds
    void Update(int pass);
    void End();
    
protected:
    
    virtual ~PoseEngine();
    
private:
    
    tPose* m_P3DPose;
    Pose* m_Pose;
    
    int m_PassCount;
    int m_MaxPoseDriversPerPass;
    
    int* m_PoseDriverCounts;
    PoseDriver*** m_PoseDrivers;
};


} // namespace poser


#endif
