#ifndef _IKDriver_HPP_
#define _IKDriver_HPP_

#include "radmath/radmath.hpp"
#include "poser/posedriver.hpp"
#include "raddebug.hpp"

class tSkeleton;

namespace sim
{
    class Pose;
}

namespace sim
{


class SRS;
class SimTarget;


// IKLimbType is used to calculate the restPose poleVector if the limb is hyper-extended
enum IKLimbType {  IKLimbElbow = 0, IKLimbKnee  };

// GoalType determines which Solve method is used
enum GoalType { GPosition = 0, GOrientation, GAim };

// ConstraintType determines how the PoleVector is calculated 
enum ConstraintType { CPosition, CPoleVector, COrientation, CAim }; 

enum ModifierType { MParented = 0, MPoleFromPose, MPoleAndGoalFromPose, MPoleAndGoalFromMaya };

class IKDriver : public poser::PoseDriver
{
public:
    // constructor to be called when calculating twist matrices (called from toolib)
    
    IKDriver(
        poser::Pose* inPose,
        int inStartjoint, 
        int inMidjoint, 
        int inEndjoint, 
        IKLimbType inIKLimbType,
        SimTarget* inEffectorTarget,
        SimTarget* inConstraintTarget = NULL
        );
    
    
    
    // virtual base class methods
    int GetMinimumJointIndex() const { return mStartIndex; }
    int GetPriority() const { return poser::IKDriverPriority; }
    virtual void Advance(float dt);
    virtual void Update(poser::Pose* pose) { UpdateJoints(); }
    
    
    // Modifier member functions
    void SetSwivelAngle(float a) { mSwivelAngle = a; }
    void SetGoalType(GoalType type) {mGoalType = type; }
    void SetLocalGoal(const rmt::Vector& newGoal );   
    void SetLocalGoal(const rmt::Matrix& newGoal );   
    
    void SetShiftedParentSkel();
    void SetShiftedParentPose();
    void SetRotateAxis();
    void SetOptimize(bool val) { mOptimize = val; }
    
    void ReplacePose(poser::Pose* inPose);
    void ReplaceEffectorTarget(SimTarget* inTarget);
    void ProgressiveGoal(rmt::Matrix& goal);
    
    bool ComputeTwistMatrices();
    // Access member functions
    
    // returned vectors are in CharacterRoot coordinates
    rmt::Vector GetMidJointPose();   
    rmt::Vector GetMidJointSkel();
    rmt::Vector GetGoalFromPose();
    
    // returned vectors are in ShiftedParent coordinates
    rmt::Vector GetProjFromPose();
    rmt::Vector GetProjFromSkel();   
    
    void SetProjDefault();
    void SetProjParented();
    
    bool IsSolveNeeded(const rmt::Vector& newGoal);
    bool IsSolveNeeded(const rmt::Matrix& newGoal);
    
    void UpdateJoints();
    void UpdateJointMatrix(float dt = 0);
    void DebugDisplay(); 
    //void PrintData();
    
    // *********** modifier member functions which communicate with SRS solver class **********
    void SetSolverAxes();   // set SRS::proj_axis, SRS::pos_axis
    void SetPoleVector(rmt::Vector v);    // v in character-root coords
    void SetPointConstraint(const rmt::Vector& v);
    void SetProjAxis(const rmt::Vector& v);      // v in shifted-parent coords
    
    void SetEndEffectorOffset(const rmt::Vector& v);
    void SetEndEffectorOffset(const rmt::Matrix& m);
    
    // Types of solvers
    bool SolveForGoal(const rmt::Vector& goal); // solve shoulder, and elbow or equivalent for legs.
    bool SolveForGoal(const rmt::Matrix& goal); // solve shoulder, elbow and wrist
    void SolveAim(const rmt::Vector& goal);
    
    /**************************************************************************************/
    
protected:
    virtual ~IKDriver();
    
    void Init();
    
private:
    bool mOptimize, mOldVersion;   // used to switch between old x-aligned version and new version
    
    int            mStartIndex, mMidIndex, mEndIndex, mParentIndex;
    poser::Pose*   mPose;
    tSkeleton*     mSkeleton;
    SimTarget*     mEffectorTarget;
    SimTarget*     mConstraintTarget;
    SRS*           mIKSolver;
    
    float mSwivelAngle;
    
    
    rmt::Vector    mPointConstraint;  // CharacterRoot coordinates
    rmt::Vector    mLocalGoal, mProjAxis,  mPosAxis, mRotateAxis;      // ShiftedParent coordinates
    rmt::Vector    mLocalGoal0, mProjAxis0, mPosAxis0, mLastGoal, mRotateAxis0;    // used in parenting calculations
    // ProjAxis = PoleVector in ShiftedParent coordinates
    // PosAxis is only significant if swivelAngle is non-zero.  Determines positive direction.
    
    // matrices used to transform points and vectors from character-root to shiftedParent space
    rmt::Matrix    mShiftedParent, mInvShiftedParent; 
    // matrices used by mIKSolver.  Last three are the result matrices. 
    rmt::Matrix    mLocalGoalMat, mT, mTa, mS, mE, mR1, mRmid, mR2 ;
    // matrices used to untwist skin on non-standard skeletons
    rmt::Matrix    mTw1, mTwmid, mTw2, mInvTw1, mInvTwmid;
    
    
    GoalType       mGoalType;
    ConstraintType mConstraintType;
    ModifierType   mModifierType;
    IKLimbType     mIKLimbType;
    
    IKDriver& operator=(IKDriver& inObj) { rAssert(false); return *this; }
    IKDriver(const IKDriver& inObject) { rAssert(false); }
    
}; // ikdriver

}  // sim

#endif // _IKDriver_HPP_