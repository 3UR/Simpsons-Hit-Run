#ifndef _COLLISION_HPP_
#define _COLLISION_HPP_

#include "simcollision/collisionobject.hpp"

namespace sim
{

typedef unsigned CollisionStatus;
const CollisionStatus Collision_Unsolved       = 0;
const CollisionStatus Collision_PhysicsSolved  = 1<<0;
const CollisionStatus Collision_FlexibleSolved = 1<<1;
const CollisionStatus Collision_AllSolved      = 1<<0 | 1<<1;

class SimulatedObject;

class Collision
{
    friend class SubCollisionDetector;
public:
    Collision();
    Collision(CollisionVolume* a, const rmt::Vector& pa, CollisionVolume* b, const rmt::Vector& pb, float dist, const rmt::Vector& n);
    ~Collision() {}
    
    void Set(CollisionVolume* a, const rmt::Vector& pa, CollisionVolume* b, const rmt::Vector& pb, float dist, const rmt::Vector& n);
    //bool Contains(CollisionObject* vol) const { return mCollisionObjectA == vol || mCollisionObjectB == vol; }
    
    CollisionVolume* mCollisionVolumeA;
    CollisionVolume* mCollisionVolumeB;
    CollisionObject* mCollisionObjectA;
    CollisionObject* mCollisionObjectB;
    int mIndexA;                  // indicated the colliding articulation, -1 if root
    int mIndexB;                  // indicated the colliding articulation, -1 if root
    
    // the next values are compute such that
    // mPositionA = mPositionB + mNormal * mDistance
    
    rmt::Vector mNormal;          // collision normal
    float mDistance;              // negative means inter-penetration
    
    CollisionStatus mStatus;      // indicate if collision has been solved or not
    
    // return contact point on the two volumes in world space. nb. 
    const rmt::Vector& GetPositionA() const { return mPositionA; }
    const rmt::Vector& GetPositionB() const { return mPositionB; }
    
    SimulatedObject* GetSimulatedObjectA() { return mCollisionObjectA->GetSimState()->GetSimulatedObject(mIndexA); }
    SimulatedObject* GetSimulatedObjectB() { return mCollisionObjectB->GetSimState()->GetSimulatedObject(mIndexB); }
    
protected:
    rmt::Vector mPositionA;       // contact point on mCollisionObjectA
    rmt::Vector mPositionB;       // contact point on mCollisionObjectB
};


//
//
//


class CollisionObjectPair
{
    
public:
    CollisionObjectPair();
    CollisionObjectPair(CollisionObject *a, CollisionObject *b);
    ~CollisionObjectPair() {}
    
    bool Contains(CollisionObject* obj) const { return mObjectA == obj || mObjectB == obj ? true : false; }
    
    CollisionVolume* VolumeA() { return mObjectA->GetCollisionVolume(); }
    CollisionVolume* VolumeB() { return mObjectB->GetCollisionVolume(); }
    
    CollisionObject* mObjectA;
    CollisionObject* mObjectB;
    
    float UpApproxSpeedMagnitude() const
    {
        return mObjectA->GetSimState()->UpApproxSpeedMagnitude() + mObjectB->GetSimState()->UpApproxSpeedMagnitude();
    }

    float SafeCollisionDistance(float dt);
    void  UpdateSafeTimeBeforeCollision(float dt)
    {
        mSafeTimeBeforeCollision -= dt;

        /* 
        // we could decrease from the time the contribution caused by the increase of the safe collision distance
        // but it doesn't seem necessary.
        float d = mSavedSafeCollisionDistance;
        (void)SafeCollisionDistance(dt);

        if (d < mSavedSafeCollisionDistance)
        {
            mSafeTimeBeforeCollision -= (mSavedSafeCollisionDistance-d)/UpApproxSpeedMagnitude();
        }
        */
    }

    
    int      mCollide;
    float    mSafeTimeBeforeCollision;
    bool     mNeedsUpdate;
    
    void Set(CollisionObject *a, CollisionObject *b);
    void PrepareForCollisionDetection();
    
private:
};

} // sim

#endif // _Collision_HPP_
