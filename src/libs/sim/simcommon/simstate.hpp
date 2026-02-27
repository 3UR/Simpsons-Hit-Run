
#ifndef _SIMSTATE_HPP_
#define _SIMSTATE_HPP_


#include <p3d/refcounted.hpp>
#include <raddebug.hpp>
#include <radmath/radmath.hpp>
#include "simcommon/symmatrix.hpp"
#include "simphysics/virtualcm.hpp"

class tEntityStore;

namespace sim
{

class SimulatedObject;
class CollisionObject;
class PhysicsProperties;
class CollisionVolume;

enum SimControlEnum { simAICtrl=0, simSimulationCtrl };

typedef unsigned SimStateAttributes;
const SimStateAttributes SimStateAttribute_Collision  = 1<<0;
const SimStateAttributes SimStateAttribute_Simulated  = 1<<1;
const SimStateAttributes SimStateAttribute_Default    = 1<<2;
const SimStateAttributes SimStateAttribute_LastBit    = 1<<3;

//
//
//

class SimVelocityState
{
public:
    SimVelocityState():
      mLinear(0.0f,0.0f,0.0f),
          mAngular(0.0f,0.0f,0.0f) {}
      
      void Reset() { mLinear.Clear(); mAngular.Clear(); }
      rmt::Vector mLinear;
      rmt::Vector mAngular;
};

void ExtractVelocityFromMatrix(const rmt::Matrix& oldMatrix, const rmt::Matrix& newMatrix, float scale, float dt, SimVelocityState& velocity);

//
//
//

class ManualSimState;

class SimState
    : public tRefCounted
{
public:
    
    static SimState* CreateSimState(tUID inUID,         SimStateAttributes inAttrib = SimStateAttribute_Default, tEntityStore* inStore=NULL );
    static SimState* CreateSimState(const char* inName, SimStateAttributes inAttrib = SimStateAttribute_Default, tEntityStore* inStore=NULL );
    static SimState* CreateSimState(SimState* inSimState, const char* inName=NULL);

    static SimState* CreateSimState(CollisionObject* inColl, SimulatedObject* inSim);

    // this is for creation of simple invisible object like tiles that represent collision with the terrain
    static SimState* CreateSimState(CollisionVolume* inCollisionVolume, const char* inName = NULL, tEntityStore* inStore = NULL);
    
    // this simstate cannot be updated
    static SimState* CreateManualSimState(CollisionVolume* inCollisionVolume, const char* inName=NULL);

    // this is manual creation of static collision object
    static SimState* CreateStaticSimState(CollisionObject* inCollisionObject);
    
    void* mAIRefPointer;
    int   mAIRefIndex;            // Greg's index
    
    //void  SetScale(float scale);
    float GetScale() const { return mScale; }
    bool  HasScale() const { return (mScale == 1.0f ? false : true ); }
    void  SetScaleFromTransform();
    
    SimControlEnum GetControl() const { return mControl; }
    void SetControl(SimControlEnum inControl);
    virtual void SetHasMoved(bool in_hasMoved);
    
    virtual void AddVirtualCM( int inIndex, VirtualCMMode inBits = VCM_DefaultMode );
    virtual void InitVirtualCM( int inIndex=-1 );
    virtual void InitAllVirtualCM();
    sim::VirtualCM* GetVirtualCM() {return mVirtualCM;}
    virtual sim::VirtualCM* GetVirtualCM(int inIndex) {return mVirtualCM;}
    virtual void AddCacheToVirtualCM( const rmt::Vector& inV, const rmt::Vector& inW, int inIndex = -1 );

    const rmt::Matrix& GetTransform() const { return mTransform; }
    const rmt::Vector& GetPosition()  const { return mTransform.Row(3); }
    virtual const rmt::Matrix& GetTransform(int inIndex) const { return mTransform; }
    virtual const rmt::Vector& GetPosition(int inIndex)  const { return mTransform.Row(3); }
    virtual void SetTransform(const rmt::Matrix& inTransform, float dt = 0);
    
    virtual void ResetVelocities()         { mVelocityState.Reset(); mApproxSpeedMagnitude = 0; }
    SimVelocityState& VelocityState()      { return mVelocityState; }
    rmt::Vector& GetLinearVelocity ()      { return mVelocityState.mLinear; }
    rmt::Vector& GetAngularVelocity ()     { return mVelocityState.mAngular; }
    
    // return the velocity of the object at the given world position
    virtual void GetVelocity(const rmt::Vector& inPosition, rmt::Vector& oVelocity, int inIndex = -1);
    
    virtual SimulatedObject* GetSimulatedObject(int index) { return mSimulatedObject; }
    SimulatedObject* GetSimulatedObject() { return mSimulatedObject; }
    CollisionObject* GetCollisionObject() { return mCollisionObject; }
    
    void SetPhysicsProperties(PhysicsProperties* inProperties);
    PhysicsProperties* GetPhysicsProperties() const;
    
    float GetSphereRadius()             const;
    bool IsObjectMoving()               const { return mObjectMoving; }

    /* nb: simstate SafeTimeBeforeCollision is no longer needed by the 
       library. it could still be usefull and the code remains there but is
       diseabled by default. Enable it by calling: CollisionManager->SetUpdateSimStateSTBC(true);
    */
    float& SafeTimeBeforeCollision()          { return mSafeTimeBeforeCollision; }
    
    float UpApproxSpeedMagnitude()      const { return mApproxSpeedMagnitude*sApproxSpeedMagnitudeFactor; }
    float DownApproxSpeedMagnitude()    const { return mApproxSpeedMagnitude/sApproxSpeedMagnitudeFactor; }
    static void SetApproxSpeedMagnitudeFactor(float inNewFactor) { if (inNewFactor > 0) sApproxSpeedMagnitudeFactor = inNewFactor; }
    
    virtual void DebugDisplay(int debugIndex = 0);
    virtual bool RequiresPushTransforForDisplay() const { return true; }

    virtual void DynamicPositionAdjustment(const rmt::Vector& inDeltaPosition, float inDt, bool adjustSpeed);

    bool IsArticulated() const { return mArticulated; }

    void SwapCollisionVolume(CollisionVolume* inCollisionVolume);

protected:
    
    SimState(SimControlEnum inControl = simAICtrl);
    virtual ~SimState();
    
    // push the current transform in the simulated object. There is no need for that
    // method to be public. The user who wants to move the object manually has to make sure 
    // that the control is to ai. Then whenever the control is set back to simulation, 
    // SyncSimObj is called from SetControl to make sure that the simulated object is in synch.
    void SyncSimObj();
    
    virtual void SetSimulatedObject(SimulatedObject* inObject);
    virtual void SetCollisionObject(CollisionObject* inObject);
    
    void MoveCollisionObject(const rmt::Matrix& previousTransform, const rmt::Matrix& newTransform);
    
    rmt::Matrix       mTransform;
    SimVelocityState  mVelocityState;
    float             mScale;
    SimControlEnum    mControl;
    SimulatedObject*  mSimulatedObject;
    CollisionObject*  mCollisionObject;
    VirtualCM      *  mVirtualCM; 
    bool              mObjectMoving;
    float             mSafeTimeBeforeCollision;
    float             mApproxSpeedMagnitude;

    bool              mArticulated;
    
    static float      sApproxSpeedMagnitudeFactor;
    
private:
    SimState& operator=(SimState& inObj) { rAssert(false); return *this; }
    SimState(const SimState& inSimState) { rAssert(false); }
};

class ManualSimState 
    : public SimState
{
public:
    virtual const rmt::Matrix& GetTransform(int inIndex) const { return sIdentity; }
    virtual const rmt::Vector& GetPosition(int inIndex)  const { return *((rmt::Vector*)sIdentity.m[3]); }
    virtual void SetTransform(const rmt::Matrix& inTransform, float dt = 0) {}

    static rmt::Matrix sIdentity;
};

} // sim

#endif // _SimState_HPP_
