

#ifndef TrackerJointModifier_HPP_
#define TrackerJointModifier_HPP_

#include <radmath/radmath.hpp>
//#include "simcommon/tarray.hpp"
#include "poser/posedriver.hpp"

namespace poser
{
    class Joint;
}

namespace sim
{

class SimTarget;

class TrackerJointModifier
    : public poser::PoseDriver
{
public:
    enum LookAtControlEnum { LookAtEmpty = 0, LookAtDissipating, LookAtActivated };
    
    TrackerJointModifier(poser::Pose* inPose, int inIndex, SimTarget* t = 0);
    
    void LookAt(SimTarget* t, bool resetdef = true);
    SimTarget* GetLookAt()                          { return mLookAt; }
    void Reset() { LookAt(GetLookAt(), true); }
    
    void Update(poser::Pose* inPose);
    virtual void Advance(float dt) { mCurrentDt = dt; }
    
    void SetLookAtControl(LookAtControlEnum c);
    bool IsTracking()                               { return mLookAtControl == LookAtActivated; }
    bool SeeTarget()                          const { return mSeeTarget; }
    void SetLookAtDir0(const rmt::Vector& v)        { mLookAtDir0 = v; }
    void SetAxis0(const rmt::Vector& v)             { mAxis0 = v; }
    
    inline void SetSpeed(float fh, float fv)        { mSpeedH = fh; mSpeedV = fv; }
    inline void SetMaxAngleDef(float fh, float fv)  { mMaxangleH = fh; mMaxangleV = fv; }
    inline void SetGoodEnough(float f)              { mGoodEnough = f; }
    
    bool TrackVertical()                      const { return mTrackVertical; }
    void SetTrackVertical(bool b)                   { mTrackVertical = b; }
    
    void DebugDisplay();
    
    bool UseAnimJointLocalMat()               const { return mUseAnimJointLocalMat; }
    void SetAnimJointLocalMat();
    
    int GetPriority()                         const { return poser::TrackerJointPriority; }
    
protected:
    void AddDefToMatrix(rmt::Matrix& inMatrix) const;
    virtual ~TrackerJointModifier();
    
private:
    
    SimTarget*           mLookAt;                   // look at target
    
    float                mTrackerJointDef;          // current deformation around axis
    LookAtControlEnum    mLookAtControl;            // current look at control
    rmt::Vector          mLookAtDir0;               // look at direction in joint space
    rmt::Vector          mAxis0;                    // main axis of rotation
    
    bool                 mTrackVertical;            // indicates if vertical tracking is enable
    float                mTrackerJointDefVertical;  // current deformation around vertical axis
    
    float                mSpeedH;                   // trackerJoint angular speed in rad/sec
    float                mSpeedV;                   // trackerJoint angular speed in rad/sec
    float                mMaxangleH;                // max deformation
    float                mMaxangleV;                // max deformation
    float                mGoodEnough;               // acceptable deviation
    
    bool                 mSeeTarget;                // indocates if the target is seen 
    
    bool                 mUseAnimJointLocalMat;     // current matrix stored in mJointLocalMat
    rmt::Matrix          mJointLocalMat;            // current matrix for the joint
    rmt::Matrix          mStoredAnimMat;            // current frame matrix for that joint
    rmt::Matrix          mRestPose;                 // restpose matrix for the tracking joint (neutral position)
    poser::Joint*        mJoint;                    // the pose
    int                  mJointIndex;
    float                mCurrentDt;
};

} // sim

#endif // TrackerJointModifier_HPP_