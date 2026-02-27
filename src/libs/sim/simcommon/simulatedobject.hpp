
#ifndef _SIMULATEDOBJECT_HPP_
#define _SIMULATEDOBJECT_HPP_

#include "p3d/entity.hpp"

#include "simcommon/physicsproperties.hpp"
#include "simcommon/simconstraint.hpp"

namespace sim
{

enum SimulatedObjectTypeEnum { RigidObjectType, ArticulatedObjectType, FlexibleObjectType, MaxPhObjEnum };

class SymMatrix;


class SimulatedObject
    : public tEntity
{
public:
    SimulatedObject(PhysicsProperties* inParam);
    
    SimulatedObjectTypeEnum Type() const  { return mType; }
    int RefIndex() const { return mRefIndex; }
    
    virtual void Update(float inDt_sec)  {}
    
    virtual void SyncSimObj(bool inReset=true) = 0;
    
    tUID GetPhysicsPropertiesUID() const { return mPhysicsProperties->GetUID(); }
    tUID GetSimEnvironmentUID(void) { return mSimEnvironment->GetUID(); }
    
    virtual void UpdateForPhysicsProperties() {}
    virtual void UpdateForSimEnvironment() {}
    
    virtual void ResetRestingDetector(){};
    void UseRestingDetector(bool inUse) { mUseRestingDetector = inUse; }
    bool UseRestingDetector() const { return mUseRestingDetector; }
    
    virtual bool IsAtRest() const = 0;
    virtual bool TestResting(float dt) = 0;
    virtual void WakeUp() = 0;
    virtual float GetMinimumLinSpeed()
    {
        SimLength l(0.1f);
        SimTime   t(1.0f);
        return  l.Value() / t.Value();
    }
    virtual float GetMinimumAngSpeed()
    {
        SimLength l(10.0f);
        return GetMinimumLinSpeed() / l.Value();
    }
    
    virtual int GetNumSubObj() const { return 0; }
    
    //Read physical properties methods:
    float Density()         const { return mPhysicsProperties->Density(); }
    
    void SetSimEnvironment(SimEnvironment* inSimEnvironment);
    
    // Methods in the process of solving collisions
    // StartCollision clear the cache buffer
    // AddImpulse store in the cache buffer the effect caused by the collision, returns false if the impulse can be added in the buffer.
    // End Collision adds the cache buffer into the object's state
    virtual void  StartCollision(const rmt::Vector& inPos, const rmt::Vector& inDir, const SimulatedObject* inObj = NULL, int index = -1, int otherIndex = -1)=0;
    virtual bool  AddImpulse(const rmt::Vector& inPos, const rmt::Vector& inImp, const SymMatrix& inSymMatrix, SimulatedObject* otherObj = NULL, int index = -1, int otherIndex = -1)=0;
    virtual void  EndCollision(bool fdyn = true, int index = -1, int otherindex = -1)=0;
    virtual void  GetMassMatrix(const rmt::Vector& inPos, SymMatrix& oM, SimulatedObject* o = NULL, int index = -1, int otherIndex = -1)=0;
    virtual float InvMass() const { return 1.0f; }
    virtual void  ResolveInterpenetration( int in_index, float &io_dist, const rmt::Vector &inN, rmt::Vector &o_imp, bool addconstrain, const SimulatedObject &otherObj );
    virtual void  AddSolverConstraint( const rmt::Vector& inDir, int in_index ){};
    virtual bool  SpeedAdjustmentFromCache(const rmt::Vector& , rmt::Vector&, int inIndex = -1 ) { return false; }
    
    // access to collision cache data
    virtual bool GetCollisionCache(rmt::Vector& o_cacheV, rmt::Vector& o_cacheW, int inIndex=-1) const{return false; }//default simulated objects do not have caches;
    virtual bool IsCacheEmpty(int index = -1) const { return true; }
    virtual void ResetCache(int index = -1){}

    //Methods related to constraints.
    virtual bool  UpdateConstraintTarget( const LocalTarget &in_target, rmt::Vector *o_VelocityTarget, SimTarget &io_target) const { return false;}
    virtual bool  UpdateConstraint( const LocalTarget &in_LocalTarget, const SimTarget &in_Target, const rmt::Vector *in_VelocityTarget, float in_dt) { return false;}
    virtual void  AddConstraint( const LocalTarget &in_LocalTarget ){};
    SimState* GetSimState() const { return mSimState; }
    
protected:
    friend class PhysicsRestingDetector;
    friend class SimState;
    friend class SimStateArticulated;
    friend class SimulationLoader;
    friend class PhNIS;
    
    virtual ~SimulatedObject();
    
    virtual SimulatedObject* Clone( SimState *inSimState) = 0;
    virtual void SetSimState( SimState *inSimState) { mSimState = inSimState; }
    
    void SetPhysicsProperties(PhysicsProperties* inProperties);
    PhysicsProperties* GetPhysicsProperties() { return mPhysicsProperties; }
    
    SimEnvironment* GetSimEnvironment() const { return mSimEnvironment;}
    
    SimState *mSimState;
    
    float                   mProgressTime;
    PhysicsProperties       *mPhysicsProperties;
    SimEnvironment          *mSimEnvironment;
    bool                    mUseRestingDetector;
    
    SimulatedObjectTypeEnum mType;
    int  mRefIndex;
    
    SimulatedObject(); // for clone service
    void SetClone(SimulatedObject* inClone, SimState* inSimState); //const;
    
    static int sSimulatedObjectRefIndex;
    
    static float sMaxTimeStep; // default max time step set for 30 fps

private:
    SimulatedObject& operator=(SimulatedObject& inObj) { rAssert(false); return *this; }
    SimulatedObject(const SimulatedObject& inObject) { rAssert(false); }
};

} // sim

#endif // _SimulatedObject_HPP_
