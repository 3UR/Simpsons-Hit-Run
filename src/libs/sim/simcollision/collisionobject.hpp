
#ifndef _COLLISIONOBJECT_HPP_
#define _COLLISIONOBJECT_HPP_

#include <p3d/entity.hpp>
#include <p3d/loadmanager.hpp>
#include <radmath/radmath.hpp>
#include "simcommon/tarray.hpp"
#include "simcommon/simstate.hpp"
#include "simcommon/physicsproperties.hpp"
#include "simcollision/collisionvolume.hpp"
#include "simcollision/collisionanalyserdata.hpp"


class tCompositeDrawable;

namespace sim
{

class SimState;
class CollisionVolume;

class CollisionObject
    : public tEntity
{
public:
    CollisionObject(CollisionVolume* inCollisionVolume);
    
    virtual CollisionObject* Clone(SimState* inSimState);
    
    SimState* GetSimState()                            { return mSimState; }
    
    const rmt::Matrix& GetTransform(int objref)  const { return mSimState->GetTransform(objref); };
    const rmt::Vector& GetPosition(int objref)   const { return mSimState->GetPosition(objref); };
    
    CollisionVolume* GetCollisionVolume()              { return mCollisionVolume; }
    
    void Moved(const rmt::Vector& dp)
    {
        rAssert(mManualUpdate == false && mIsStatic == false); 
        mHasMoved = true;
        mCollisionVolume->mPosition.Add(dp);
        mCollisionVolume->mUpdated = false;
    }
    void SetHasMoved(bool in_val)
    {
        rAssert(mManualUpdate == false && (in_val == false || mIsStatic == false)); 

        mHasMoved = in_val; 
        if (in_val)
            mCollisionVolume->mUpdated = false;
    }
    void SetHasRelocated(bool in_val)
    {
        rAssert(mManualUpdate == false && (in_val == false || mIsStatic == false)); 

        mHasRelocated = in_val; 
        if (in_val) 
            mCollisionVolume->mUpdated = false; 
    }
    void Relocated()
    {
        rAssert(mManualUpdate == false && mIsStatic == false);

        mCollisionVolume->mUpdated = false;
        mHasMoved = true;
        mHasRelocated = true;
    }
    void EndCollision()
    {
        mHasMoved = mHasRelocated = false;
    }
    void Update()
    {
        rAssert(mManualUpdate == false);

        if (mHasMoved || mHasRelocated)
        {
            mCollisionVolume->UpdateAll();
        }
    }
    void PostManualUpdate()                     // for manual collision object only
    {
        mHasMoved = true;
        mHasRelocated = true;
        mCollisionVolume->UpdateBBox();
        mCollisionVolume->UpdateBSphere();
    }
    bool HasMoved()                              const { return mHasMoved; }
    bool HasRelocated()                          const { return mHasRelocated; }
    void SetManualUpdate(bool b)                       { mManualUpdate = b; }
    
    // This has been added for Greg M
    bool IsManualUpdate() {return mManualUpdate;}


    
    CollisionAnalyserPossibleEvent PossibleCollisionEvents()     { return mPossibleCollisionEvents; }
    void SetPossibleCollisionEvents(CollisionAnalyserPossibleEvent in_val) { mPossibleCollisionEvents=in_val; }
    
    int  GetNumJoint()                           const { return mNumJoint; }
    
    tName mStringData;                           // public string/uid for the user
    
    float RestCoeff()       const { rAssert(mPhysicsProperties); return mPhysicsProperties->RestCoeff(); }
    float TangRestCoeff()   const { rAssert(mPhysicsProperties); return mPhysicsProperties->TangRestCoeff(); }
    float FrictCoeff()      const { rAssert(mPhysicsProperties); return mPhysicsProperties->FrictCoeff(); }
    
    class CollisionVolumeOwner
    {
    public:
        CollisionVolumeOwner() : mNumOwner(0), mOwnerList(NULL) {}
        ~CollisionVolumeOwner() { delete[] mOwnerList; }
        
        void SetNumOwnerList( int inNum );
        CollisionVolumeOwner &operator= ( const CollisionVolumeOwner &inCollisionVolumeOwner );
        
        int mNumOwner;
        TArray<tUID>* mOwnerList;
        TArray<bool> mVisible;
    };
    
    class SelfCollision
    {
    public:
        SelfCollision()
            : mIndex1(0), mIndex2(0), mSelf1(0), mSelf2(0), 
            mCollisionVolume1(NULL), mCollisionVolume2(NULL)
        {
        }
        void Set(int inIndex1, int inIndex2, bool inSelf1, bool inSelf2)
        {
            mIndex1 = inIndex1;
            mIndex2 = inIndex2;
            mSelf1 = inSelf1;
            mSelf2 = inSelf2;
            rAssert(mIndex1 != mIndex2);
        }
        
        ~SelfCollision() {}
        
        int mIndex1;
        int mIndex2;
        bool mSelf1;
        bool mSelf2;
        
        CollisionVolume* mCollisionVolume1;
        CollisionVolume* mCollisionVolume2;
    };
    
    void AddSelfCollision(int inIndex1, int inIndex2, bool inSelf1, bool inSelf2);
    
    TArray<SelfCollision>& GetSelfCollisionList() { return mSelfCollisionList; }
    
    virtual void SetVisibility(tCompositeDrawable* inCompositeDrawable);
    virtual void SetVisibility(int inJoint, bool inVisible);
    
    void SetDefaultArea(int area)                { mDefaultArea = area; }
    int GetDefaultArea()                   const { return mDefaultArea; }
    
    void SetIsStatic(bool inIsStatic);
    bool IsStatic()                        const { return mIsStatic; }
    
    void SetCollideWithStatic(bool b);
    bool CollideWithStatic()               const { return mCollideWithStatic; }

    void SetAutoPair(bool inAutoPair)            { mAutoPair = inAutoPair; }
    bool GetAutoPair()                     const { return mAutoPair; }
    
    void SetSelfCollisionEnabled(bool b)         { mSelfCollisionEnabled = b; }
    bool GetSelfCollisionEnabled()         const { return mSelfCollisionEnabled; }
    
    void SetCollisionEnabled(bool b)             { mCollisionEnabled = b; }
    bool GetCollisionEnabled()             const { return mCollisionEnabled; }
    
    void SetRayCastingEnabled(bool b)            { mRayCastingEnabled = b; }
    bool GetRayCastingEnabled()            const { return mRayCastingEnabled; }
    
    PhysicsProperties* GetPhysicsProperties() const { return mPhysicsProperties; }

protected:
    friend class SimState;
    friend class SimStateArticulated;
    friend class SimulationLoader;
    friend class SimStateFlexible;
    friend class PSystGeomLoader;
    friend class CollisionObjectLoader;
    
    CollisionObject()
    : mSimState(NULL),
      mPhysicsProperties(NULL),
      mCollisionVolume(NULL),
      mNumJoint(0),
      mIsStatic(false),
      mCollideWithStatic(true),
      mDefaultArea(0),
      mHasMoved(true),
      mHasRelocated(true),
      mAutoPair(true),
      mManualUpdate(false),
      mCollisionEnabled(true),
      mSelfCollisionEnabled(false),
      mRayCastingEnabled(true),
      mPossibleCollisionEvents(cCollisionAnalyserPossibleEventAll)
      {}
    virtual ~CollisionObject();
    
    void SetPhysicsProperties(PhysicsProperties* inProperties);
    void SetCollisionVolume(CollisionVolume* inCollisionVolume);

    void ConvertToRigidBody();
    
    void SetSimState( SimState *inSimState)         { mSimState = inSimState; }
    void SetNumJoint(int inNumJoint)                { mNumJoint = inNumJoint; }
    
    void SetSelfCollision(SelfCollision& inSelfColl);
    
    SimState*            mSimState;           // world space position
    PhysicsProperties*   mPhysicsProperties;  // for friction etc.
    CollisionVolume*     mCollisionVolume;    // the collision volume hierarchy
    int                  mNumJoint;           // the number of joint
    
    TArray<SelfCollision> mSelfCollisionList;
    
    CollisionVolumeOwner mCollisionVolumeOwner;
    
    bool                 mIsStatic : 1;           // true for static world object
    bool                 mCollideWithStatic : 1;  // if false, object not static won't collide with static objects
    int                  mDefaultArea;            // can be use to direct object in specific area
    
private:
    
    // an object is moved when using simstate->Settransform(newtransform, dt > 0)
    // a speed is extracted from the displacment.
    bool                 mHasMoved      : 1;       // indicate the object has moved
   
    // an object is relocated when using simstate->Settransform(newtransform)
    // NO speed is extracted from the displacment.
    bool mHasRelocated         : 1;       // indicated teh object has been relocated
    bool mAutoPair             : 1;       // indicates to the collison manager if we want this object to be automatically paired with other object in the area
    bool mManualUpdate         : 1;       // indicates the collision volume are update by the user (like tiles etc.)
    bool mCollisionEnabled     : 1;       // enable/diseable temp. coll detect without removing the object from the collision manager
    bool mSelfCollisionEnabled : 1;       // enable/diseable self collision
    bool mRayCastingEnabled    : 1;       // enable/diseable ray casting hit on an object

    // the collision detector tries to estimated a number of frame within which two
    // moving object should not collide. If an object is relocated, this estimate is
    // reset.

    // two different method for setting collisionvolume visibility. Note that user
    // should not use the two method on a same object since the results from the two
    // methods might intere with each other.
    void UpdateVisibilityByOwner(CollisionVolume* inVol, TArray<bool>& inVisible);
    void UpdateVisibilityByJoint(CollisionVolume* inVol, int inJoint, bool inVisible);
    
    CollisionAnalyserPossibleEvent mPossibleCollisionEvents;    // for collision analyzer
    
    CollisionObject& operator=(CollisionObject& inVolume) { rAssert(false); return *this; }
    CollisionObject(const CollisionObject& inCollisionObject) { rAssert(false); }
};

//
//
//

class CollisionObjectLoader
: public tSimpleChunkHandler
{
public:
CollisionObjectLoader();
~CollisionObjectLoader() {}

tEntity* LoadObject(tChunkFile* f, tEntityStore* store);
CollisionVolume* LoadCollisionVolume(tChunkFile* f);
};


} // sim

#endif // _CollisionObject_HPP_
