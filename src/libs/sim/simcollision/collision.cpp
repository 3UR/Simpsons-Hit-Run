
#include "simcollision/collision.hpp"
#include "simcommon/simmath.hpp"

using namespace RadicalMathLibrary;

namespace sim
{

Collision::Collision()
  : mCollisionVolumeA(NULL),
    mCollisionVolumeB(NULL),
    mCollisionObjectA(NULL),
    mCollisionObjectB(NULL),
    mIndexA(-1),
    mIndexB(-1),
    mStatus(Collision_Unsolved)
{
}

Collision::Collision(CollisionVolume* a, const rmt::Vector& pa, CollisionVolume* b, const rmt::Vector& pb, float inDist, const rmt::Vector& inNormal)
{
    Set(a, pa, b, pb, inDist, inNormal);
}

void Collision::Set(CollisionVolume* a, const rmt::Vector& pa, CollisionVolume* b, const rmt::Vector& pb, float dist, const rmt::Vector& N)
{
    rAssert(a != NULL && b != NULL);
    mCollisionVolumeA = a;
    mCollisionVolumeB = b;
    mCollisionObjectA = a->GetCollisionObject();
    mCollisionObjectB = b->GetCollisionObject();
    mIndexA = a->ObjRefIndex();
    mIndexB = b->ObjRefIndex();
    mPositionA = pa;
    mPositionB = pb;
    mDistance = dist;
    mNormal = N;
    mStatus = Collision_Unsolved;
    
    //uncomment for debug
    //P3DVERIFY(TestPointOnSurf(mCollisionVolumeA, mPositionA), "pointa not on surf","");
    //P3DVERIFY(TestPointOnSurf(mCollisionVolumeB, mPositionB), "pointb not on surf","");
    
    // test if collision is dynamic (needs to be solved) and set the position relative to 
    // the object position
}

//
//
//

CollisionObjectPair::CollisionObjectPair()
    : mObjectA(NULL), 
    mObjectB(NULL), 
    mCollide(false), 
    mSafeTimeBeforeCollision(0),
    mNeedsUpdate(false)
{ 
}

CollisionObjectPair::CollisionObjectPair(CollisionObject *a, CollisionObject *b)
{ 
    Set(a, b);
}

void CollisionObjectPair::Set(CollisionObject*a, CollisionObject*b)
{
    rAssert(a != NULL);
    rAssert(b != NULL);
    
    mObjectA = a; 
    mObjectB = b;
    mCollide = false;
    mSafeTimeBeforeCollision = 0;
    mNeedsUpdate = true;
}

void CollisionObjectPair::PrepareForCollisionDetection()
{
    mCollide = false;
    
    if (!mObjectA->GetCollisionEnabled() || !mObjectB->GetCollisionEnabled())
    {
        mNeedsUpdate = false;
    }
    else if (mObjectA->HasRelocated() || mObjectB->HasRelocated())
    {
        mSafeTimeBeforeCollision = 0;
        mNeedsUpdate = true;
    }
    else if (mObjectA->HasMoved() || mObjectB->HasMoved())
    {
        mNeedsUpdate = true;
    }
    else
    {
        mNeedsUpdate = false;
    }
}

float CollisionObjectPair::SafeCollisionDistance(float dt)
{
    Vector v;
    //static float angular_consibution_scale = 0.0f;

    if (mObjectA->HasMoved())
    {
        //va.CrossProduct(mObjectA->GetSimState()->GetAngularVelocity(), mObjectA->GetCollisionVolume()->mBoxSize);
        //va.Scale(angular_consibution_scale);
        //va.Add(mObjectA->GetSimState()->GetLinearVelocity());
        v = mObjectA->GetSimState()->GetLinearVelocity();
    }
    else
    {
        v.Clear();
    }

    if (mObjectB->HasMoved())
    {
        //vb.CrossProduct(mObjectB->GetSimState()->GetAngularVelocity(), mObjectB->GetCollisionVolume()->mBoxSize);
        //vb.Scale(angular_consibution_scale);
        //vb.Add(mObjectB->GetSimState()->GetLinearVelocity());
        v.Sub(mObjectB->GetSimState()->GetLinearVelocity());
    }

    float vn = v.MagnitudeSqr();

    if (vn > VERY_SMALL)
    {
        vn = Sqrt(vn);
        v.Scale(Inverse(vn));

        static float ScaleSafeRadius = 0.5f;
        static float ScaleFrameDist = 1.2f;
        CollisionVolume* vol = mObjectA->GetCollisionVolume();
        float d = vol->Type() == WallVolumeType ? VERY_LARGE : Fabs(v.DotProduct(vol->mBoxSize));
        vol = mObjectB->GetCollisionVolume();
        float d2 = vol->Type() == WallVolumeType ? VERY_LARGE : Fabs(v.DotProduct(vol->mBoxSize));
        d = Min(d, d2);

        float framedist = vn * dt;

        if (framedist >  d * ScaleSafeRadius)
        {
            return framedist * ScaleFrameDist;
        }
    }
    return 0.0f;
}

} // sim
