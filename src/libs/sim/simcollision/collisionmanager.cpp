
#include "simcollision/collisionmanager.hpp"
#include "simcommon/simlistinitsize.hpp"
#include "simcommon/simmath.hpp"
#include "simcommon/simutility.hpp"

namespace sim
{

CollisionManager* CollisionManager::sInstance = NULL;

CollisionManager* CollisionManager::GetInstance()
{
    if (sInstance == NULL)
    {
        sInstance = new CollisionManager;
        sInstance->AddRef();
        rAssert(sInstance);
    }
    return sInstance;
}

CollisionManager::CollisionManager(): 
    mCMAttributes(CM_DetectIfMoving | CM_SolveAll),
    mNbArea(CollisionAreaListSize),
    mCacheActivatedArea( 64 )
{
    mAreaArray = new Area[mNbArea];
    SetAreaActivated(0, true);
}

CollisionManager::~CollisionManager()
{
    delete[] mAreaArray;
    sInstance = NULL;
}

void CollisionManager::SetCollisionManagerAttributes(CollisionManagerAttributes inCMAttributes) 
{ 
    mCMAttributes = inCMAttributes; 
}

CollisionManagerAttributes CollisionManager::GetCollisionManagerAttributes() const 
{ 
    return mCMAttributes; 
}


void CollisionManager::SetNbArea(int nbArea)
{
    if( mNbArea == nbArea )
    {
        return;
    }
    rAssertMsg( false, "Instead of calling this function, just set the default value correctly" );
    rAssert(nbArea > 0);
    delete[] mAreaArray;
    radMemoryAllocator old = ::radMemorySetCurrentAllocator( RADMEMORY_ALLOC_TEMP );
    mAreaArray = new Area[nbArea];
    ::radMemorySetCurrentAllocator( old );
    mNbArea = nbArea;
    mCacheActivatedArea.Clear();
}

void CollisionManager::ResetArea(int area)
{
    rAssert((area >= 0) && (area < mNbArea));
    mAreaArray[area].Reset();
}

void CollisionManager::ResetAll()
{
    for (int i=0; i<mNbArea; ++i)
    {
        mAreaArray[i].Reset();
    }
}

void CollisionManager::SetUseExclusiveAutoPair(bool b, int area)
{
    if (area == -1)
    {
        for (int i=0; i<mNbArea; ++i)
        {
            mAreaArray[i].SetUseExclusiveAutoPair(b);
        }
    }
    else if (area < mNbArea)
    {
        mAreaArray[area].SetUseExclusiveAutoPair(b);
    }
}

void CollisionManager::ClearCollisions()
{
    for (int i=0; i<mNbArea; ++i)
    {
        mAreaArray[i].ClearCollisions();
    }
}

bool CollisionManager::Contains(CollisionObject* inObject, int area) const
{
    return mAreaArray[area].mObjectList.Contains(inObject);
}

bool CollisionManager::Contains(CollisionObject* inObject) const
{
    bool ret = false;
    
    for (int i=0; i<mNbArea; ++i)
    {
        if (Contains(inObject, i))
        {
            ret = true;
            break;
        }
    }
    return ret;
}

void CollisionManager::AddCollisionObject(CollisionObject* inObject, int area)
{
    rAssert((area >= 0) && (area < mNbArea));
    mAreaArray[area].AddCollisionObject(inObject);
}

bool CollisionManager::RemoveCollisionObject(CollisionObject* inObject, int area)
{
    rAssert((area >= 0) && (area < mNbArea));
    return mAreaArray[area].RemoveCollisionObject(inObject);
}

void CollisionManager::RemovePair(CollisionObject* inObjectA, CollisionObject* inObjectB, int area)
{
    rAssert((area >= 0) && (area < mNbArea));
    mAreaArray[area].RemovePair(inObjectA, inObjectB);
}

void CollisionManager::AddPair(CollisionObject* inObjectA, CollisionObject* inObjectB, int area)
{
    rAssert((area >= 0) && (area < mNbArea));
    mAreaArray[area].AddPair(inObjectA, inObjectB);
}

void CollisionManager::DetectCollision(int area, float inDt, float gameTime, const bool printOut )
{
    if (mAreaArray[area].mActivated)
    {
        SimStats::StartDetectCollision();
            unsigned int objectListSize = mAreaArray[ area ].mObjectList.GetSize();
            unsigned int numberOfPairs = mAreaArray[area].mPairList.GetSize();
        #ifdef RAD_DEBUG
            if( printOut )
            {
                rDebugPrintf( "AREA:%d - objects:%d pairs:%d\n", area, objectListSize, numberOfPairs );
            }
        #endif

        mAreaArray[area].PrepareForCollisionDetection(mCMAttributes);
    
        mCollisionDetector.DetectCollision(&mAreaArray[area].mPairList, &mAreaArray[area].mCollisionList, inDt, printOut);
    
        for( unsigned int i = 0; i < objectListSize; ++i )
        {
            CollisionObject* object = mAreaArray[area].mObjectList[i];

            if (object->GetSelfCollisionEnabled())
            {
                for (int j=0; j<object->GetSelfCollisionList().GetSize(); j++)
                {
                    CollisionObject::SelfCollision& sc = object->GetSelfCollisionList()[j];
                    sc.mCollisionVolume1->UpdateAll();
                    sc.mCollisionVolume2->UpdateAll();
                    mCollisionDetector.DetectCollision(sc.mCollisionVolume1, sc.mCollisionVolume2, &mAreaArray[area].mCollisionList);
                }
            }
        }
        SimStats::EndDetectCollision();
    }
}

void CollisionManager::DetectCollision(float inDt, float gameTime, const bool printOut )
{
    unsigned int size = mCacheActivatedArea.GetSize();
    for (unsigned int i = 0; i < size; ++i )
    {
        int areaToDetect = mCacheActivatedArea[i];
        DetectCollision( areaToDetect, inDt, gameTime, printOut );
    }
    EndDetectCollision();
}

void CollisionManager::SolveCollision(int area, float inDt, float gameTime)
{
    if (mAreaArray[area].mActivated)
    {
        SimStats::StartSolveCollision();
        mImpulseBasedCollisionSolver.SolveCollisions(&mAreaArray[area].mCollisionList, inDt, gameTime);
        SimStats::EndSolveCollision();
    }
}

void CollisionManager::SolveCollision(float inDt, float gameTime)
{
    for (int i=0; i<mCacheActivatedArea.GetSize(); ++i)
    {
        SolveCollision(mCacheActivatedArea[i], inDt, gameTime);
    }
}

void CollisionManager::EndDetectCollision()
{
    SimStats::StartDetectCollision();
    for (int i=0; i<mCacheActivatedArea.GetSize(); ++i)
    {
        mAreaArray[mCacheActivatedArea[i]].EndCollisionDetection(mCMAttributes);
    }
    SimStats::EndDetectCollision();
}

void CollisionManager::Update(float inDt, float gameTime)
{
    DetectCollision(inDt, gameTime);
    SolveCollision(inDt, gameTime);
}

void CollisionManager::UpdateSelfCollision(CollisionObject* inObject, float inDt, float gameTime)
{
    if (inObject->GetSelfCollisionEnabled())
    {
        TArray<Collision> list;
        for (int j=0; j<inObject->GetSelfCollisionList().GetSize(); j++)
        {
            CollisionObject::SelfCollision& sc = inObject->GetSelfCollisionList()[j];
            sc.mCollisionVolume1->UpdateAll();
            sc.mCollisionVolume2->UpdateAll();
            mCollisionDetector.DetectCollision(sc.mCollisionVolume1, sc.mCollisionVolume2, &list);
        }
        if (list.GetSize() > 0)
            mImpulseBasedCollisionSolver.SolveCollisions(&list, inDt, gameTime);
    }
}

void CollisionManager::SetAreaActivated(int area, bool activated)
{
    if (mAreaArray[area].mActivated != activated)
    {
        if (activated)
        {
            mCacheActivatedArea.Add(area);
        }
        else
        {
            mCacheActivatedArea.Remove(area);
        }
        mAreaArray[area].mActivated = activated;
    }
}

void CollisionManager::ActivateAllAreas()
{
    DeActivateAllAreas();
    for (int i=0; i<mNbArea; ++i)
    {
        mAreaArray[i].mActivated = true;
        mCacheActivatedArea.Add(i);
    }
}

void CollisionManager::DeActivateAllAreas()
{
    for (int i=0; i<mCacheActivatedArea.GetSize(); ++i)
    {
        mAreaArray[mCacheActivatedArea[i]].mActivated = false;
    }
    mCacheActivatedArea.Clear();
}

bool CollisionManager::DetectRayIntersection(   TArray<RayIntersectionInfo>& inList, 
                                                const rmt::Vector& sourceOfRay, 
                                                const rmt::Vector& endOfRay, 
                                                bool inStaticObjectOnly,
                                                int inArea)
{
    SimStats::StartRCasSimulation();

    rmt::Vector sray;
    sray.Sub(endOfRay, sourceOfRay);
    rmt::Vector ray = sray;
    float rayLen = ray.NormalizeSafe();

    if (RayIntersectionInfo::ReturnClosestOnly() && inList.GetSize() > 0)
    {
        RayIntersectionInfo info;
        info.mCollisionObject = inList[0].mCollisionObject;
        inList.Clear();
        info.mCollisionVolume = info.mCollisionObject->GetCollisionVolume();
        if (info.mCollisionObject->GetRayCastingEnabled() && (!inStaticObjectOnly || info.mCollisionObject->IsStatic()) && RayIntersectVolume(sourceOfRay, ray, sray, rayLen, info))
        {
            info.mCollisionObject = info.mCollisionVolume->GetCollisionObject();
            RayIntersectionInfo::Add(inList, info, ray, sray, rayLen);
        }
    }

    if (inArea == -1)
    {
        for (int i=0; i<mNbArea; ++i)
        {
            mAreaArray[i].DetectRayIntersection(inList, sourceOfRay, ray, sray, rayLen, inStaticObjectOnly);
        }
    }
    else if (inArea >= 0 && inArea < mNbArea)
    {
        mAreaArray[inArea].DetectRayIntersection(inList, sourceOfRay, ray, sray, rayLen, inStaticObjectOnly);
    }
    
    SimStats::EndRCasSimulation();

    return inList.GetSize() > 0 ? true : false;
}

bool CollisionManager::DetectApproxRayIntersection(ApproxRayIntersectionInfo& info, const rmt::Vector& sourceOfRay, const rmt::Vector& endOfRay, bool inStaticObjectOnly, int inArea)
{
    SimStats::StartRCasSimulation();

    bool ret = false;

    rmt::Vector sray;
    sray.Sub(endOfRay, sourceOfRay);
    rmt::Vector ray = sray;
    float rayLen = ray.NormalizeSafe();

    if (info.mFirstHit)
    {
        ret = ApproxRayIntersectVolume( sourceOfRay, 
                                        ray, 
                                        sray, 
                                        rayLen, 
                                        info.mFirstHit, 
                                        info);
    }

    if (ret == false)
    {
        if (inArea == -1)
        {
            for (int i=0; i<mNbArea; ++i)
            {
                if (mAreaArray[i].mActivated)
                {
                    for (int j=0; j<mAreaArray[i].mObjectList.GetSize(); ++j)
                    {
                        CollisionObject* obj = mAreaArray[i].mObjectList[j];
                        if (    obj->GetRayCastingEnabled() && 
                                obj->GetCollisionVolume() != info.mFirstHit &&
                                (!inStaticObjectOnly || obj->IsStatic()))
                        {
                            ret = ApproxRayIntersectVolume( sourceOfRay, 
                                                            ray, 
                                                            sray, 
                                                            rayLen, 
                                                            obj->GetCollisionVolume(), 
                                                            info);
                            if (ret)
                            {
                                break;
                            }
                        }
                    }
                }
            }
        }
        else if (inArea >= 0 && inArea < mNbArea)
        {
            for (int j=0; j<mAreaArray[inArea].mObjectList.GetSize(); ++j)
            {
                CollisionObject* obj = mAreaArray[inArea].mObjectList[j];
                if (    obj->GetRayCastingEnabled() && 
                        obj->GetCollisionVolume() != info.mFirstHit &&
                        (!inStaticObjectOnly || obj->IsStatic()))
                {
                    ret = ApproxRayIntersectVolume( sourceOfRay, 
                                                    ray, 
                                                    sray, 
                                                    rayLen, 
                                                    obj->GetCollisionVolume(), 
                                                    info);
                    if (ret)
                    {
                        break;
                    }
                }
            }
        }
    }
    if (ret == false)
    {
        info.mFirstHit = NULL;
    }

    SimStats::EndRCasSimulation();
    return ret;
}

void CollisionManager::FindClosestPointOnVolume(const rmt::Vector& inPos, CollisionVolume* inVolume, rmt::Vector& outNormal, float& outDist)
{
    SimStats::StartRCasSimulation();

    inVolume->UpdateAll();
    
    outDist = VERY_LARGE;
    sim::FindClosestPointOnVolume(inPos, inVolume, outNormal, outDist);

    SimStats::EndRCasSimulation();
}

//
//
//

bool CollisionManager::Area::sUpdateSimStateSTBC = false;

CollisionManager::Area::Area()
: mObjectList(CollisionObjectListSize),
  mPairList(CollisionObjectPairListSize),
  mCollisionList(CollisionListSize),
  mUseExclusiveAutoPair(false),
  mActivated(false)
{
}

CollisionManager::Area::~Area()
{
    Reset();
}

void CollisionManager::Area::Reset()
{
    int i;
    for (i=0; i<mObjectList.GetSize(); ++i)
    {
        mObjectList[i]->Release();
    }
    mObjectList.RemoveAll();
    mPairList.Clear();
    mCollisionList.Clear();
}

void CollisionManager::Area::AddCollisionObject(CollisionObject* inObject)
{
    rAssert(!mObjectList.Contains(inObject));
    
    if (mUseExclusiveAutoPair)
    {
        if (inObject->GetAutoPair())
        {
            for (int i=0; i<mObjectList.GetSize(); ++i)
            {
                CollisionObject* other = mObjectList[i];

                if (other->GetAutoPair())
                {
                    if (!inObject->IsStatic() || !other->IsStatic())
                    {
                        if (!inObject->CollideWithStatic() && other->IsStatic())
                            continue;
                        if (!other->CollideWithStatic() && inObject->IsStatic())
                            continue;
            
                        if (inObject->IsStatic()) // make sure the static object if first
                            mPairList.NewElem().Set(inObject, other);
                        else
                            mPairList.NewElem().Set(other, inObject);
                    }
                }
            }
        }
    }
    else
    {
        for (int i=0; i<mObjectList.GetSize(); ++i)
        {
            CollisionObject* other = mObjectList[i];

            if (inObject->GetAutoPair() || other->GetAutoPair())
            {
                if (!inObject->IsStatic() || !other->IsStatic())
                {
                    if (!inObject->CollideWithStatic() && other->IsStatic())
                        continue;
                    if (!other->CollideWithStatic() && inObject->IsStatic())
                        continue;
            
                    if (inObject->IsStatic()) // make sure the static object if first
                        mPairList.NewElem().Set(inObject, other);
                    else
                        mPairList.NewElem().Set(other, inObject);
                }
            }
        }
    }

    mObjectList.Add(inObject);
    inObject->AddRef();
}

bool CollisionManager::Area::RemoveCollisionObject(CollisionObject* inObject)
{
    rAssert(inObject);
    bool found = false;

    if (mObjectList.Remove(inObject))
        inObject->Release();
    
    for (int i=mPairList.GetSize()-1; i>=0; --i)
    {
        if (mPairList[i].Contains(inObject))
        {
            mPairList.RemoveAt(i);
            found = true;
        }
    }
    return found;
}

void CollisionManager::Area::RemovePair(CollisionObject* inObjectA, CollisionObject* inObjectB)
{
    if( inObjectA != inObjectB )
    {
        for (int i=0; i<mPairList.GetSize(); ++i)
        {
            if (mPairList[i].Contains(inObjectA) && mPairList[i].Contains(inObjectB))
            {
                mPairList.RemoveAt(i);
                return;
            }
        }
    }
}

void CollisionManager::Area::AddPair(CollisionObject* inObjectA, CollisionObject* inObjectB)
{
    rAssert(mObjectList.Contains(inObjectA) && mObjectList.Contains(inObjectB));
    rAssert(inObjectA != inObjectB);

    for (int i=0; i<mPairList.GetSize(); ++i)
    {
        if (mPairList[i].Contains(inObjectA) && mPairList[i].Contains(inObjectB))
        {
            return; // already there
        }
    }
    mPairList.NewElem().Set(inObjectA, inObjectB);
}

void CollisionManager::Area::PrepareForCollisionDetection(CollisionManagerAttributes inCMAttributes)
{
    ClearCollisions();
    
    int i;
    
    if (inCMAttributes & CM_DetectAll)
    {
        for (i=0; i<mPairList.GetSize(); i++)
        {
            // Only relocate if not static and not manual update
            if (!mPairList[i].mObjectA->IsStatic() && !mPairList[i].mObjectA->IsManualUpdate())
            {
                mPairList[i].mObjectA->Relocated();
            }
            if (!mPairList[i].mObjectB->IsStatic() && !mPairList[i].mObjectB->IsManualUpdate())
            {
                mPairList[i].mObjectB->Relocated();
            }
            mPairList[i].PrepareForCollisionDetection();
        }
    }
    else
    {
        for (i=0; i<mPairList.GetSize(); i++)
        {
            mPairList[i].PrepareForCollisionDetection();
        }
    }
}

void CollisionManager::Area::EndCollisionDetection(CollisionManagerAttributes inCMAttributes)
{
    if (inCMAttributes & CM_DetectAll)
    {
        for (int i=0; i<mObjectList.GetSize(); i++)
        {
            mObjectList[i]->EndCollision();
        }
    }
    else
    {
        if (sUpdateSimStateSTBC)
        {
            // Now that all pairs have their mSafeTimeBeforeCollision updated 
            // by the collision detection that have just finished, update the minimum
            // value for all simstate
            int i;
            for (i=0; i<mObjectList.GetSize(); i++)
            {
                CollisionObject* obj = mObjectList[i];
                obj->EndCollision();
                obj->GetSimState()->SafeTimeBeforeCollision() = CollisionDetector::SafeTimeBeforeCollision();
            }

            for ( i=0; i<mPairList.GetSize(); i++)
            {
                if (mPairList[i].mNeedsUpdate)
                {
                    SimState *simState;
                    simState = mPairList[i].mObjectA->GetSimState();
                    simState->SafeTimeBeforeCollision() = rmt::Min(simState->SafeTimeBeforeCollision(), mPairList[i].mSafeTimeBeforeCollision);
            
                    simState = mPairList[i].mObjectB->GetSimState();
                    simState->SafeTimeBeforeCollision() = rmt::Min(simState->SafeTimeBeforeCollision(), mPairList[i].mSafeTimeBeforeCollision);
                }
            }
        }
        else
        {
            for (int i=0; i<mObjectList.GetSize(); i++)
            {
                mObjectList[i]->EndCollision();
            }
        }
    }
}

void CollisionManager::Area::DetectRayIntersection(TArray<RayIntersectionInfo>& inList, 
                                                const rmt::Vector& sourceOfRay, 
                                                const rmt::Vector& ray,
                                                rmt::Vector& sray,
                                                float& rayLen,
                                                bool inStaticObjectOnly)
{
    if (mActivated)
    {
        RayIntersectionInfo info;

        for (int j=0; j<mObjectList.GetSize(); ++j)
        {
            CollisionObject* obj = mObjectList[j];
            info.mCollisionVolume = obj->GetCollisionVolume();

            if (    obj->GetRayCastingEnabled() && 
                    (!inStaticObjectOnly || obj->IsStatic()) && 
                    RayIntersectVolume(sourceOfRay, ray, sray, rayLen, info)
               )
            {
                info.mCollisionObject = info.mCollisionVolume->GetCollisionObject();
                RayIntersectionInfo::Add(inList, info, ray, sray, rayLen);
            }
        }
    }
}

} // sim
