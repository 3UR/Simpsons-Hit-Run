
#ifndef _SIMSTATEARTICULATED_HPP_
#define _SIMSTATEARTICULATED_HPP_

#include <simcommon/simstate.hpp>
#include <simcommon/simmath.hpp>
#include <simcommon/tlist.hpp>
#include <poser/joint.hpp>

namespace poser
{
class Pose;
}

namespace sim
{

class JointMatrixModifier;
class FlexJointModifier;
class FlexibleJointObject;
class ArticulatedPhysicsObject;


class SimStateArticulated
    : public SimState
{
public:
    static SimStateArticulated* CreateSimStateArticulated(tUID inUID        , poser::Pose* inPose, SimStateAttributes inAttrib = SimStateAttribute_Default, tEntityStore* inStore=NULL );
    static SimStateArticulated* CreateSimStateArticulated(const char* inName, poser::Pose* inPose, SimStateAttributes inAttrib = SimStateAttribute_Default, tEntityStore* inStore=NULL );
    static SimStateArticulated* CreateSimStateArticulated(poser::Pose* inPose, SimStateArticulated* inSimState, const char* inName=NULL);
    static SimStateArticulated* CreateSimStateArticulated(poser::Pose* inPose, CollisionObject* inColl, SimulatedObject* inSim );
    
    static bool sUseInventoryObject;

    poser::Pose* GetPose() { return mPose; }
    
    void GetVelocity(const rmt::Vector& inPosition, rmt::Vector& oVelocity, int inIndex=-1);
    
    //using SimState::GetSimulatedObject;  // so that it is not hidden by GetSimulatedObject(int inIndex)
    //using SimState::GetPosition;  // so that it is not hidden by GetPosition(int inIndex)
    //using SimState::GetTransform;  // so that it is not hidden by GetTransform(int inIndex)
    SimulatedObject* GetSimulatedObject() { return SimState::GetSimulatedObject(); }
    const rmt::Vector& GetPosition() const { return SimState::GetPosition(); }
    const rmt::Matrix& GetTransform() const { return SimState::GetTransform(); }
    


    SimulatedObject* GetSimulatedObject(int index);
    
    virtual void AddVirtualCM( int inIndex, VirtualCMMode inBits = VCM_DefaultMode );
    virtual void InitVirtualCM( int inIndex );
    virtual void InitAllVirtualCM();
    virtual sim::VirtualCM* GetVirtualCM(int inIndex);
    virtual void AddCacheToVirtualCM( const rmt::Vector& inV, const rmt::Vector& inW, int inIndex = -1 );

    virtual void SetTransform(const rmt::Matrix& inTransform, float dt = 0);

    virtual const rmt::Matrix& GetTransform(int inIndex) const;
    virtual const rmt::Vector& GetPosition(int inIndex)  const;
    
    void StoreJointState(float inDt);
    void UpdateJointState(float inDt); // this should be called after SetTransform has been called and poser::Pose has been updated
    virtual void ResetVelocities();
    
    void ConvertToRigidBody();
    bool ConvertedToRigidBody() const { return mConvertedToRigidBody; }

    void StartModifyingJointVisibility();
    void SetJointVisibility(int inJoint, bool inVisible);
    void EndModifyingJointVisibility();

    SimState* CreateJointSimState(int inJoint, const char* inName=NULL);

    class JointState
    {
        friend class SimStateArticulated;
        
    public:
        void StoreJointState(float dt);
        
        void ResetVelocities()                 { mVelocityUpdated = true; mVelocityState.Reset(); }
        SimVelocityState& VelocityState()      { UpdateVelocity(); return mVelocityState; }
        rmt::Vector& GetLinearVelocity ()      { UpdateVelocity(); return mVelocityState.mLinear; }
        rmt::Vector& GetAngularVelocity ()     { UpdateVelocity(); return mVelocityState.mAngular; }
        void GetVelocity(const rmt::Vector& inPosition, rmt::Vector& oVelocity);
        const rmt::Vector& GetPosition()  const { return mJoint->GetWorldTransform().GetMatrix().Row(3); }
        
        const rmt::Matrix& GetPreviousTransform() const { return mPreviousTransform; }
        
        void DebugDisplay();
        
        poser::Joint* Joint() { return mJoint; }
        
    protected:
        JointState();
        ~JointState();
        
        void SetJoint(poser::Joint* inJoint);
        void UpdateVelocity();
        
        poser::Joint* mJoint;
        SimVelocityState  mVelocityState;
        bool mVelocityUpdated;
        rmt::Matrix mPreviousTransform;
        float mPreviousDt;
    };
    
    JointVCMpArray mVirtualCMList;

    JointState* GetJointState(int i) { return &(mJointState[i]); }
    
    virtual void DebugDisplay(int debugIndex = 0);
    virtual bool RequiresPushTransforForDisplay() const { return false; }
    
    ArticulatedPhysicsObject* GetArtPhysObject() { return (ArticulatedPhysicsObject*)mSimulatedObject; }
    
    void AddFlexJointModifier(FlexJointModifier* inFlexJointModifier);
    FlexJointModifier* GetFlexJointModifier(tUID inUID);
    void RemoveFlexJointModifier(tUID inUID);
    void SynchronizeFlexJointModifier();

    // the MinJointCollisionIndex is the one used to make the collision time estimate
    void SetMinJointCollisionIndex(int inMinJointCollisionIndex);
    int GetMinJointCollisionIndex()  const { return mMinJointCollisionIndex; }
    
    // Michael Riegger - This is the optimization that Stan Jang did for the Hulk to 
    // destroy to poser::Pose object from the articulated sim state to save memory
    void ReleasePose();

protected:
    SimStateArticulated(poser::Pose* inPose, SimControlEnum inControl = simAICtrl);
    ~SimStateArticulated();

    virtual void SetCollisionObject(CollisionObject* inObject);
    void UpdateHasMoved();
    void SetPose(poser::Pose* inPose);
    
    poser::Pose* mPose;
    JointState* mJointState;
    int mMinJointCollisionIndex;
    
    bool mConvertedToRigidBody;

    class FlexibleJointCache
    {
    public:
        FlexibleJointCache() : mFlexibleJointObject(NULL) {}
        ~FlexibleJointCache() {}
        
        FlexibleJointObject* mFlexibleJointObject;
        short mParticleIndex;
    };
    void SetFlexibleJointCache();
    
private:
    SimStateArticulated& operator=(SimStateArticulated& inObj) { rAssert(false); return *this; }
    SimStateArticulated(const SimStateArticulated& inSSA) { rAssert(false); }
};

} // sim

#endif // _SimStateArticulated_HPP_
