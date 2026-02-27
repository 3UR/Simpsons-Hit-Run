
#ifndef _ARTICULATEDPHYSICSOBJECT_HPP_
#define _ARTICULATEDPHYSICSOBJECT_HPP_


#include "simphysics/physicsobject.hpp"
#include "simphysics/physicsjoint.hpp"
#include "simcommon/skeletoninfo.hpp"
#include "simcommon/simstatearticulated.hpp"

//#include "poser/pose.hpp"

namespace sim
{

class PhysicsProperties;

class ArticulatedPhysicsObject
    : public PhysicsObject
{
public:
    ArticulatedPhysicsObject(PhysicsProperties* inProperties, pPhysicsJoint* inJoints, int inNumJoints);
    
    void SetPose(poser::Pose* inPose);
    void UpdatePoseMatrix();
    
    virtual void StartCollision(const rmt::Vector& inPos, const rmt::Vector& inDir, const SimulatedObject* inObj = NULL, int index = -1, int otherIndex = -1);
    virtual bool AddImpulse(const rmt::Vector& inPos, const rmt::Vector& inImp, const SymMatrix& inSymMatrix, SimulatedObject* otherObj = NULL, int index = -1, int otherIndex = -1);
    virtual void EndCollision(bool fdyn = true, int index = -1, int otherIndex = -1);
    virtual void GetMassMatrix(const rmt::Vector& inPos, SymMatrix& oM, SimulatedObject* o = NULL, int index = -1, int otherIndex = -1);
    virtual bool SpeedAdjustmentFromCache(const rmt::Vector& pos, rmt::Vector & speed, int inIndex = -1);
    
    // access to collision cache data
    virtual bool GetCollisionCache(rmt::Vector& o_cacheV, rmt::Vector& o_cacheW, int inIndex=-1) const;
    virtual bool IsCacheEmpty(int index = -1) const;
    virtual void ResetCache(int index = -1);

    virtual void UpdateForPhysicsProperties(); // Need to update after changed of dymamic parameters. 
    //virtual void UpdateForSimEnvironment();
    virtual void UpdateScale() { rAssert(false); }

    PhysicsJoint* CollidingJoint() { return mCollidingJoint; }
    PhysicsJoint* SelfCollisionCenter() { return mSelfCollisionCenter; }
    bool SelfCollision() const { return mSelfCollision; }
    
    void SetSkeletonInfo(SkeletonInfo* inSkeletonInfo);
    const SkeletonInfo::JointInfo& JointInfo(int index) const { return mSkeletonInfo->mJointInfo[index]; }
    SkeletonInfo* GetSkeletonInfo() const { return mSkeletonInfo; }
    void JointAxis(int inIndex, rmt::Vector& axis, bool recompute = true) const;
    const rmt::Matrix& JointTransform(int inIndex) const { return mPose->GetJoint(inIndex)->GetWorldTransform().GetMatrix(); }
    
    float TimeStep() const { return mTimeStep; }
    
    rmt::Vector& GetJointLinearVelocity(int index) { return ((SimStateArticulated*)mSimState)->GetJointState(index)->GetLinearVelocity(); }
    rmt::Vector& GetJointAngularVelocity(int index) { return ((SimStateArticulated*)mSimState)->GetJointState(index)->GetAngularVelocity(); }
    
    int NumSimJoints() const { return mNumSimJoints; }
    PhysicsJoint* GetSimJoint(int simJointIndex) { return mSimJoints[simJointIndex]; }
    PhysicsJoint* GetJoint(int jointIndex) { return mJoints[jointIndex]; }
    
    void ApplyImpulse(int inJoint, float inIntensity, const rmt::Vector& inDir);

    float GetDissipationInternalRate()  const { return mDissipationInternalRate; }
    void SetTimeComputeInertiaMatrix(float inTimeComputeInertiaMatrix)      { mTimeComputeInertiaMatrix = inTimeComputeInertiaMatrix; }
    void SetDissipationInternalRate(float inDissipationInternalRate)        { mDissipationInternalRate = inDissipationInternalRate; }
    void SetDissipationDeformationRate(float inDissipationDeformationRate)  { mDissipationDeformationRate = inDissipationDeformationRate; }
    void SetDissipationDeformationSpeedRate(float inDissipationDeformationSpeedRate) { mDissipationDeformationSpeedRate = inDissipationDeformationSpeedRate; }
    
    bool ConvertedToRigidBody()                                       const { return ((SimStateArticulated*)mSimState)->ConvertedToRigidBody(); }

    // this is used to remove joint masses from the articulated physics object's property
    void StartModifyingJointVisibility() { mMass = mInvMass = 0; }
    bool SetJointVisibility(int inJoint, bool inVisible);
    void EndModifyingJointVisibility();
    
    virtual void Update(float inDt);

    void SetImpulseReactionEnabled(bool inImpulseReactionEnabled)           { mImpulseReactionEnabled = inImpulseReactionEnabled; }
    bool GetImpulseReactionEnabled()                                  const { return mImpulseReactionEnabled; }
    
protected:
    friend class SimStateArticulated;
    
    virtual ~ArticulatedPhysicsObject();
    
    void ComputeInertiaMatrix();
    void ComputeJointMasses();
    
    void ConvertToRigidBody();

    virtual SimulatedObject* Clone(SimState* inSimState);
    void SetClone(ArticulatedPhysicsObject* clone, SimState* inSimState); //const;
    
    void StartHandlingCollision(const SimulatedObject* otherObj, int index, int otherindex);
    void EndHandlingCollision()
    {
        mCollidingJoint = mSelfCollisionCenter = NULL;
        mSelfCollision = false;
    }
    
    void InitFromPose();
    
    pPhysicsJoint* mSimJoints;                      // simjoint array
    pPhysicsJoint* mJoints;                         // joint array
    int            mNumSimJoints;                   // num joint part of the simulation
    int            mNumJoints;                      // num joints in the pose
    poser::Pose*   mPose;                           // ref on the pose
    
    float          mTimeSinceComputeInertiaMatrix;
    float          mTimeComputeInertiaMatrix;
    
    PhysicsJoint*  mCollidingJoint;                 // the current joint in collision
    PhysicsJoint*  mSelfCollisionCenter;            // indicates the common joint in case of self-collision
    bool           mSelfCollision;                  // indicates self-collision
    bool           mAllCacheEmpty;                  // used in self-collision
    
    SkeletonInfo*  mSkeletonInfo;                   // get axis, rotaxis etc from there
    float          mTimeStep;                       // stores last time step
    float          mDissipationDeformationRate;     // rate of dissipation for ...
    float          mDissipationDeformationSpeedRate;
    float          mDissipationInternalRate;

    bool           mImpulseReactionEnabled;         // enabled secondary reaction from impulse, default=false
    
    ArticulatedPhysicsObject() : mJoints(NULL) { mType = ArticulatedObjectType; } // for clone service
    
private:
    ArticulatedPhysicsObject& operator=(ArticulatedPhysicsObject& inObj) { rAssert(false); return *this; }
    ArticulatedPhysicsObject(const ArticulatedPhysicsObject& inObject) { rAssert(false); }
};

} // sim

#endif // _ARTICULATEDPHYSICSOBJECT_HPP_
