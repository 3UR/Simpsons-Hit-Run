#ifndef _SUBCOLLISIONDETECTOR_HPP_
#define _SUBCOLLISIONDETECTOR_HPP_

#include "radmath/radmath.hpp"
#include "simcommon/tarray.hpp"

// defined to output debug info on collision detected 
// on the specified collision object pair.
//#define COLLECT_PAIR_COLLISION_HISTORY

#ifdef COLLECT_PAIR_COLLISION_HISTORY
#include "simcollision/collision.hpp"
#include <string.h>
#include <stdio.h>
#endif

namespace sim
{

class CollisionVolume;
class CylinderVolume;
class OBBoxVolume;
class Collision;
class CollisionObjectPair;



#ifdef COLLECT_PAIR_COLLISION_HISTORY

class PairCollisionInfo
{
public:
    void Set(const rmt::Vector& inPosA, const rmt::Vector& inPosB, const Collision& inColl)
    {
        mPositionA = inPosA;
        mPositionB = inPosB;
        mCollision = inColl;
    }

    void Dump(char* str)
    {
        sprintf(str, "sim: posA = %.4f %.4f %.4f posB = %.4f %.4f %.4f n = %.4f %.4f %.4f d = %.4f\n", 
            mPositionA.x, mPositionA.y, mPositionA.z,
            mPositionB.x, mPositionB.y, mPositionB.z,
            mCollision.mNormal.x, mCollision.mNormal.y, mCollision.mNormal.z,
            mCollision.mDistance);
        /*
        sprintf(str, "simcoll: posA = %.4f %.4f %.4f posB = %.4f %.4f %.4f n = %.4f %.4f %.4f d = %.4f\n", 
            mCollision.GetPositionA().x, mCollision.GetPositionA().y, mCollision.GetPositionA().z, 
            mCollision.GetPositionB().x, mCollision.GetPositionB().y, mCollision.GetPositionB().z, 
            mCollision.mNormal.x, mCollision.mNormal.y, mCollision.mNormal.z,
            mCollision.mDistance);
            */
    }

    rmt::Vector mPositionA;
    rmt::Vector mPositionB;
    Collision mCollision;
};

class PairCollisionInfoHolder
{
public:
    PairCollisionInfoHolder() : mObjectA(NULL), mObjectB(NULL) {}

    void AddCollision(const Collision& inCollision)
    {
        if ( (inCollision.mCollisionObjectA == mObjectA || inCollision.mCollisionObjectA == mObjectB) &&
             (inCollision.mCollisionObjectB == mObjectA || inCollision.mCollisionObjectB == mObjectB)
           )
        {
            mPairCollisionInfoArray.NewElem().Set(
                mObjectA->GetPosition(inCollision.mIndexA), 
                mObjectB->GetPosition(inCollision.mIndexB), 
                inCollision);
        }
    }
    void Dump(char* str, int index)
    {
        if (index < mPairCollisionInfoArray.GetSize())
            mPairCollisionInfoArray[index].Dump(str);
    }
    void Reset()
    {
        mObjectA = mObjectB = NULL;
        mPairCollisionInfoArray.Clear();
    }

    CollisionObject* mObjectA;
    CollisionObject* mObjectB;
    TArray<PairCollisionInfo> mPairCollisionInfoArray;
};

#endif




typedef enum { NO_INTERSECTION=-1, ALL_INSIDE=0, ONE_POINT=1, TWO_POINTS=2, THREE_POINTS=3  } GEOM_CODE;

typedef unsigned int CollisionFilterAttribute;
const CollisionFilterAttribute FILTER_NO_FILTERING = 0;
const CollisionFilterAttribute FILTER_BY_DEPTH     = 1<<0;
const CollisionFilterAttribute FILTER_BY_POSITION  = 1<<1;
    

class SubCollisionDetector
{
public:
    SubCollisionDetector();
    virtual ~SubCollisionDetector();

    void SetCollisionFilter(unsigned int inFilter) { mCollisionFilter = inFilter; }
    
#ifdef COLLECT_PAIR_COLLISION_HISTORY
    PairCollisionInfoHolder mPairCollisionInfoHolder;
#endif

protected:
    friend class CollisionManager;
    
    bool OBBoxSepPlane(OBBoxVolume* volA, OBBoxVolume* volB); // return true if a sperarating plane exists
    void OBBoxV(OBBoxVolume* volA, OBBoxVolume* volB);        // coll between box a and vertices of b
    void OBBoxEE(OBBoxVolume* volA, OBBoxVolume* volB);       // coll between edges of bax a and b
    bool EdgeEdge(bool& hasCollided, rmt::Vector& pa, rmt::Vector& oa, float la, rmt::Vector& pb, rmt::Vector& ob, float lb, rmt::Vector& N, float nn, float dist = 0, CollisionVolume* volA = NULL, CollisionVolume* volB = NULL);
    bool PointBoxColl(OBBoxVolume *El, rmt::Vector& pp, rmt::Vector* pa, rmt::Vector* n, float& dist, float radius = 0, bool getpos = false);
    void FindFaceIndexOrder(int* i, float d0, float d1, float d2) const;
    void FindSizeOrder4( int i[], float d[] ) const;
    void OBBoxBorderCollMove(OBBoxVolume* El, rmt::Vector& pos, float dist2, float dist3, int i);
    
    bool OBBoxCylSepPlane(OBBoxVolume* volA, CylinderVolume* volB);
    void OBBoxCylF(OBBoxVolume* volA, CylinderVolume* volB);
    void OBBoxCylV(OBBoxVolume* volA, CylinderVolume* volB);
    void OBBoxCylEE(OBBoxVolume* volA, CylinderVolume* volB);
    void OBBoxCylEC(OBBoxVolume* volA, CylinderVolume* volB);
    void CircleEdge(CollisionVolume* volA, rmt::Vector& pa, rmt::Vector& oa, float la, CylinderVolume* volB, float nft, float no, bool use_all_inside);
    void BoxSphereColl(OBBoxVolume* volA, CollisionVolume* volB, rmt::Vector& Pb, float rad);
    
    bool CylCylSepPlane(CylinderVolume* volA, CylinderVolume* volB);
    bool CylCylSphereEndSpecialFilter(float newdist);
    void CylCylSphereEnd(CylinderVolume* volA, CylinderVolume* volB);
    void CyliderFaceEnd(CylinderVolume* volA, CylinderVolume* volB)
    {
        CircleCyl(volA, volB);//All flat end will be test with side of the other cylinder.
        CylCylSphereEnd(volA, volB);//Test all sphere ends with the other cylinder side
    }
    void CircleCyl(CylinderVolume *volA, CylinderVolume *volB);
    bool CylCylEnd(CylinderVolume* volA, CylinderVolume* volB, bool testSphereA = true, bool addColl = true);
    bool CylSphereColl(CylinderVolume *volA, const rmt::Vector& Pb, float rb, rmt::Vector& N, float& dist, rmt::Vector *ppa = NULL, rmt::Vector *ppb = NULL, bool testSphereA = true);
    bool PlanePlaneIntersection(  rmt::Vector &in_c1, rmt::Vector &in_c2, rmt::Vector &in_n1,
        rmt::Vector &in_n2, rmt::Vector *o_pt, rmt::Vector *io_ns, bool ); 
    GEOM_CODE CircleCircleIntersection( const rmt::Vector &in_c1, const rmt::Vector &in_c2, const rmt::Vector &in_N,
        float in_r1, float in_r2, bool in_all,
        rmt::Vector &o_p1, rmt::Vector &o_p2 );
    
    //bool PointElColl(CollisionVolume *El, rmt::Vector& p, float& dist);
    
    
    void AddCollision(CollisionVolume* volA, const rmt::Vector& pa, CollisionVolume* volB, const rmt::Vector& pb, float dist, const rmt::Vector& N);
    void TestNormalIntegrity(Collision* c);
    TArray<Collision>* GetCollisionList();
    void SetCollisionList( TArray<Collision>& inlist );
    
    float mCollisionDistance;
    float mMinCollisionDistance;
    rmt::Vector mT;
    rmt::Vector mNormal;
    float mNt;
    int mNewColl;
    bool mSphereApproxA : 1;
    bool mSphereApproxB : 1;

    unsigned int mCollisionFilter; 
    
private:
    TArray<Collision>* mCollisionList;

};

} // sim

#endif // _PHCOLLDETECTSUB_H_
