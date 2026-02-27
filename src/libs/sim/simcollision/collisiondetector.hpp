
#ifndef _COLLISIONDETECTOR_HPP_
#define _COLLISIONDETECTOR_HPP_

#include "simcollision/subcollisiondetector.hpp"


namespace sim
{


class WallVolume;
class SphereVolume;


class CollisionDetector
    : protected SubCollisionDetector
{
public:
    CollisionDetector();
    virtual ~CollisionDetector();
    
    // collide two volumes
    void DetectCollision(CollisionVolume* volA, CollisionVolume* volB, TArray<Collision>* inCList);

    // collide many volumes 
    void DetectCollision(TArray<CollisionObjectPair>* inPList, TArray<Collision>* inCList, float inDt, const bool printOut);
   
    // collision time prediction for an object pair moving at same constant speed 
    // (if everything is well tuned, this value could be as big as you want)
    static void SetMaxSafeTimeBeforeCollision(float inTime);
    static float SafeTimeBeforeCollision() { return sSafeTimeBeforeCollision; }

    void SetCollisionDistance(float f)     { rAssert(f >= 0); mMinCollisionDistance = f; }

#ifdef COLLECT_PAIR_COLLISION_HISTORY
    void SetPairCollisionInfoHolder(CollisionObject* inObjA, CollisionObject* inObjB)
    {
        mPairCollisionInfoHolder.mObjectA = inObjA;
        mPairCollisionInfoHolder.mObjectB = inObjB;
    }
    PairCollisionInfoHolder& GetPairCollisionInfoHolder() { return mPairCollisionInfoHolder; }

    void ResetPairCollisionInfoHolder()
    {
        mPairCollisionInfoHolder.Reset();

    }
#endif

protected:
    void DetectCollision(CollisionVolume* volA, CollisionVolume* volB);
    void DetectOnSubVolume(CollisionVolume* volA, CollisionVolume* volB);
    void DetectColl(CollisionVolume*  volA, CollisionVolume* volB);
    
    void DetectColl(WallVolume*        wall, CollisionVolume* volB); // CollisionVolume vs WallVolume
    void DetectColl(OBBoxVolume*       volA, OBBoxVolume*     volB); // OBBoxVolume vs OBBoxVolume
    void DetectColl(OBBoxVolume*       volA, CylinderVolume*  volB); // OBBoxVolume vs CylinderVolume
    void DetectBSColl(OBBoxVolume*     volA, CollisionVolume* volB, float radius); // OBBoxVolume vs SphereVolume
    void DetectColl(CylinderVolume*    volA, CylinderVolume*  volB); // CylinderVolume vs CylinderVolume
    void DetectCSColl(CylinderVolume*  volA, CollisionVolume* volB); // CylinderVolume vs SphereVolume
    void DetectSSColl(CollisionVolume* volA, CollisionVolume* volB); // SphereVolume vs SphereVolume
    void DetectWallBBox(WallVolume*    wall, CollisionVolume* volA);
    
    bool CollTestInit(CollisionVolume* volA, CollisionVolume* volB);
    static float sSafeTimeBeforeCollision;
    float mDt;
};

} // sim

#endif // _PHCOLLDETECT_H_
