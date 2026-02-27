#ifndef _PHYSICSOBJECT_HPP_
#define _PHYSICSOBJECT_HPP_

#include "simphysics/restingdetector.hpp"
#include "simcommon/simmath.hpp"
#include "simcommon/simstate.hpp"
#include "simcommon/symmatrix.hpp"
#include "simcommon/simulatedobject.hpp"

#include "p3d/loadmanager.hpp"

namespace sim
{

//  Next enum describes if an object can roll and if it can about which axes.
typedef enum {   
                 AXIS_OF_REVOLUTION_ABOUT_X = 0,    //  Axis of revolution is about X axis, mGeomRotAxis is not used, 
                 AXIS_OF_REVOLUTION_ABOUT_Y = 1,    //  Axis of revoultion is about Y axis, mGeomRotAxis is not used,
                 AXIS_OF_REVOLUTION_ABOUT_Z = 2,    //  Axis of revoultion is about Z axis, mGeomRotAxis is not used,
                 ONE_AXIS_OF_REVOLUTION     = 3,    //  Can roll about one specific axis, mGeomRotAxis must set, 
                 THREE_AXES_OF_REVOLUTION,          //  Can't roll about any axes,
                 NO_AXIS_OF_REVOLUTION              //  Can't roll, cause it doesn't have an axis of revolution
             } AXIS_OF_REVOLUTION;

class PhysicsObject
    : public SimulatedObject
{
public:
    PhysicsObject(PhysicsProperties* inProperties, const Vector& inPos, const SymMatrix& inInertia, float inVolume);
    static void ResetUnitsRelatedValues();
    
    virtual bool UpdateConstraint( const LocalTarget &in_LocalTarget, const SimTarget &in_Target, const Vector *in_VelocityTarget, float in_dt);
    virtual void AddConstraint( const LocalTarget &in_LocalTarget );
    
    
    virtual void UpdateForPhysicsProperties(); // Need to update after changed of dymamic parameters. 
    virtual void UpdateForSimEnvironment();
    virtual void UpdateScale(); // uses the SimState scale factor
    
    //Synchronisation service between the SimState and the pose.
    virtual void SyncSimObj(bool inReset);
    
    //Center of Mass offset services.
    void SetExternalCMOffset(const Vector& off);
    const rmt::Vector& GetExternalCMOffset() const { return mExternalCMOffset0; }
    void GetCMOffset(rmt::Vector& off) const { off.Add(mExternalCMOffset0, mCMOffset0); }
    void CMPosition(rmt::Vector& cmPosition) const { cmPosition = mCMPosition; }
    bool GetCMOffsetEmpty() const { return mCMOffsetEmpty; }
    
    //Read and write kinematic state methods:
    void SetCMVelocity(const rmt::Vector& inVelocity);
    const rmt::Vector& GetCMVelocity(rmt::Vector& oVelocity) const 
    { 
        mSimState->GetVelocity(mCMPosition, oVelocity, -1);
        return oVelocity;
    }
    virtual float GetKineticEnergy() const;
    
    //Resting services
    
    virtual void WakeUp();
    virtual bool IsAtRest() const { return mRestingDetector.IsAtRest(); }
    virtual void ResetRestingDetector()
    {
        mRestingDetector.SetNeedReset();
    }
    void SetRestingSensitivity( float in_PeriodSensitivity, float inLinSpeedSensitivity=1.0f, float inAngSpeedSensitivity=1.0f ) 
    { 
        mRestingDetector.SetRestingSensitivity(in_PeriodSensitivity);
        mRestingDetector.SetLinSpeedSensitivity(inLinSpeedSensitivity);
        mRestingDetector.SetAngSpeedSensitivity(inAngSpeedSensitivity);
        mRestingDetector.SetNeedReset();
    }
    //Use to test against impulse effect.
    float GetMinimumLinSpeed()
    {
        if (mRestingDetector.mNeedToComputeTypicalValues)
            return SimulatedObject::GetMinimumLinSpeed();
        else
            return mRestingDetector.GetTypicalLinSpeed();
    }
    float GetMinimumAngSpeed()
    {
        if (mRestingDetector.mNeedToComputeTypicalValues)
            return SimulatedObject::GetMinimumAngSpeed();
        else
            return mRestingDetector.GetTypicalAngSpeed();}
    
    //Add force, torque and impuse methods:
    virtual void AddForce(const rmt::Vector& inForce, const rmt::Vector* inPos = NULL);
    virtual void AddTorque(const rmt::Vector& inTorque);
    
    // Methods in the process of solving collisions
    // StartCollision clear the cache buffer
    // AddCollision store in the cache buffer the effect caused by the collision, returns false if the impulse can be added in the buffer.
    // End Collision adds the cache buffer into the object's state
    virtual void StartCollision(const rmt::Vector& inPos, const rmt::Vector& inDir, const SimulatedObject* inObj = NULL, int index = -1, int otherIndex = -1) { mCacheEmpty = true; }
    virtual bool AddImpulse(const rmt::Vector& inPos, const rmt::Vector& inImp, const SymMatrix& inSymMatrix, SimulatedObject* otherObj = NULL, int index = -1, int otherIndex = -1);
    virtual void EndCollision(bool fdyn = true, int index = -1, int otherIndex = -1);
    
    // access to collision cache data
    virtual bool GetCollisionCache(rmt::Vector& o_cacheV, rmt::Vector& o_cacheW, int inIndex=-1) const;
    virtual bool IsCacheEmpty(int index = -1) const { return mCacheEmpty; }
    virtual bool ValidCollision(const PhysicsObject *inObj=NULL)      const { return true; }
    virtual void ResetCache(int index = -1) { mCacheV.Clear(); mCacheL.Clear(); mCacheEmpty = true; }
    
    virtual float Mass()    const { return mMass; }
    virtual float InvMass() const { return mInvMass; }
    virtual void  GetMassMatrix(const rmt::Vector& inPos, SymMatrix& oM, SimulatedObject* o = NULL, int index = -1, int otherIndex = -1);
    
    float GetVolume() const { return mVolume; }

    // this sets rate of dissipation of angular momentum in a rigid body;
    void SetInvTWDissip(float inInvTWDissip) { mInvTWDissip = inInvTWDissip; }
    
    tName mStringData;
    
    static float SmallSpeed()        { return sSmall_Speed; }
    bool DidCollide()          const { return mTimeSinceCollide == 0; }

    const rmt::Vector& Gravity() { return mSimEnvironment->Gravity(); }
    
    void Update(float inDt);

    void SetAxisOfRevolution(AXIS_OF_REVOLUTION inAxisOfRevolution) { mAxisOfRevolution = inAxisOfRevolution; }
    
protected:
    friend class SimState;
    friend class PhysicsRestingDetector;
    
    virtual ~PhysicsObject();
    
    virtual bool TestResting(float dt);
    
    //Cloning services:
    virtual SimulatedObject* Clone(SimState* inSimState);
    void SetClone(PhysicsObject* oClone, SimState* inSimState); //const;
    
    virtual void ResetAppliedForces();
    virtual void SolveAppliedForces(float dt);
    virtual void BodyI();
    
    float TimeSinceCollide()   const { return mTimeSinceCollide; }
    
    float mVolumei;               // Initial volume.
    float mVolume;                // Current volume.
    float mMass;                  // mass
    float mInvMass;               // 1 / mass
    
    AXIS_OF_REVOLUTION mAxisOfRevolution; //Indicate if an objec has some axes of revolution.
    rmt::Vector mGeomRotAxis;     // If we know that an object can roll, we can set this vector to its
                                  // axis of revolution. For exemple, if the local geometrical axis of
                                  // revolution of a cylinder is the y axis, mGeomRotAxis would be (0,1,0).
    
    // SimState is shared with collisionObject and application, contains the object position, orientation and scale
    rmt::Matrix mRotMat;          // orientation 
    rmt::Vector mCMPosition;      // pos of center of mass in world. 
    
    bool mCMOffsetEmpty;          // true if the geometry center equal the center of mass position in the rest frame. In that case mCMOffset==(0,0,0)
    rmt::Vector mCMOffset0;       // vector from geometry origin to pos of center of mass. (Object space)
    rmt::Vector mExternalCMOffset0;// used defined addition to the cm position (Object space)

    rmt::Vector mAngularMomentum; // angular momentum
    rmt::Quaternion mQ;           // use to integrate the orientation
    
    SymMatrix mInertiaMatrix0;    // initial inertia matrix
    SymMatrix mInertiaMatrix;     // inertia matrix at the current orientation
    SymMatrix mInvInertiaMatrix;  // inverse of I
    
    rmt::Vector mForce;           // current applied force.
    rmt::Vector mTorque;          // current applied torque, angular acceleration = fIinv * fT
    
    
    bool        mCacheEmpty;
    rmt::Vector mCacheV;
    rmt::Vector mCacheL;

    float       mInvTWDissip;
    
    
    PhysicsRestingDetector mRestingDetector;
    
    static float   sSmall_Speed;
    static int     sPhysicsObjectRefIndex;
    static float   sThresholdTimeForRestingTest;
    static float   sThresholdTimeForContactFriction;
    
    float          mTimeSinceCollide;
    bool           mTorqueApplied;  // To save fiew vector multiplication.
    
    PhysicsObject() { mType = RigidObjectType; } // for clone service
    PhysicsObject(PhysicsProperties* inProperties); // for articulated object

    
private:
    PhysicsObject& operator=(PhysicsObject& inObj) { rAssert(false); return *this; }
    PhysicsObject(const PhysicsObject& inObject) { rAssert(false); }
};

//
//
//

class PhysicsObjectLoader
: public tSimpleChunkHandler
{
public:
    PhysicsObjectLoader();
    ~PhysicsObjectLoader() {}
    
    tEntity* LoadObject(tChunkFile* f, tEntityStore* store);
};

} // sim

#endif // _PHYSICSOBJECT_HPP_
