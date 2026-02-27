
#ifndef _COLLISIONMANAGER_HPP_
#define _COLLISIONMANAGER_HPP_

#include "simcollision/collisiondetector.hpp"
#include "simcollision/impulsebasedcollisionsolver.hpp"
#include "simcollision/collision.hpp"
#include "simcollision/proximitydetection.hpp"
#include "p3d/refcounted.hpp"
#include "simcommon/tlist.hpp"
#include "simcommon/tarray.hpp"

namespace sim
{

class CollisionObject;

typedef unsigned CollisionManagerAttributes;
const CollisionManagerAttributes CM_DetectIfMoving     = 1<<1;
const CollisionManagerAttributes CM_SolveAll           = 1<<2;
const CollisionManagerAttributes CM_DetectAll          = 1<<3;
const CollisionManagerAttributes CM_LastBit            = 1<<4;


class CollisionManager
    : public tRefCounted
{
public:
    static CollisionManager* GetInstance();   // THIS WILL CREATE AN INSTANCE IF DOESN'T EXIST
    static CollisionManager* GetCurrentInstance() { return CollisionManager::sInstance; }
    
    CollisionDetector* GetCollisionDetector() { return &mCollisionDetector; }
    ImpulseBasedCollisionSolver* GetImpulseBasedCollisionSolver() { return &mImpulseBasedCollisionSolver; }
    
    void SetNbArea(int nbArea);               
    int NbArea() const { return mNbArea; }    // This will resize the array of Area
    
    void SetCollisionManagerAttributes(CollisionManagerAttributes inCDAttributes);
    CollisionManagerAttributes GetCollisionManagerAttributes() const;
    
    void ResetArea(int area = 0);
    void ResetAll();
    
    bool Contains(CollisionObject* inObject) const;
    bool Contains(CollisionObject* inObject, int area) const;
    void AddCollisionObject(CollisionObject* inObject, int area = 0);
    bool RemoveCollisionObject(CollisionObject* inObject, int area = 0);
    void RemovePair(CollisionObject* inObjectA, CollisionObject* inObjectB, int area = 0);
    void AddPair(CollisionObject* inObjectA, CollisionObject* inObjectB, int area = 0);
    
    void DetectCollision(float inDt, float gameTime, const bool printOut = false );
    void SolveCollision(float inDt, float gameTime);
    void Update(float inDt, float gameTime);
    void ClearCollisions();

    void UpdateSelfCollision(CollisionObject* inObject, float inDt, float gameTime);
    
    TArray<Collision>* GetCollisionList(int inArea = 0) { return &mAreaArray[inArea].mCollisionList; }
    TList<CollisionObject*>* GetCollisionObjectList(int inArea = 0) { return &mAreaArray[inArea].mObjectList; }
    TArray<CollisionObjectPair>* GetCollisionObjectPairList(int inArea = 0) { return &mAreaArray[inArea].mPairList; }
    
    // adds in the list the collision object interfering with the ray and ordered according to their distance to the source.
    // use RayIntersectionInfo::SetMethod(method) to set the method
    // use RayIntersectionInfo::SetReturnClosestOnly(true/false) if you need only the closest object
    // nb. if SetReturnClosestOnly(true) is used, the previous returned list can be used as a cache.
    bool DetectRayIntersection(TArray<RayIntersectionInfo>& inList, const rmt::Vector& p0, const rmt::Vector& p1, bool inStaticObjectOnly = false, int inArea=0);

    // this method search for intersection between the ray and the collisionobject. 
    // If info is provided, an "approximation" of the hit position is returned
    // If the previous info->mFirstHit is provided, the search will start with this hit
    bool DetectApproxRayIntersection(ApproxRayIntersectionInfo& info, const rmt::Vector& sourceRay, const rmt::Vector& endRay, bool inStaticObjectOnly = false, int inArea=0);
    
    void FindClosestPointOnObject(const rmt::Vector& inPos, CollisionObject* inObject, rmt::Vector& outNormal, float& outDist)
    {
        this->FindClosestPointOnVolume(inPos, inObject->GetCollisionVolume(), outNormal, outDist);
    }
    void FindClosestPointOnVolume(const rmt::Vector& inPos, CollisionVolume* inVolume, rmt::Vector& outNormal, float& outDist);
    

    void SetUseExclusiveAutoPair(bool b, int area=-1);
    void SetAreaActivated(int area, bool activated);
    void ActivateAllAreas();
    void DeActivateAllAreas();
    bool AreaActivated(int area) const { return mAreaArray[area].mActivated; }

    void SetUpdateSimStateSTBC(bool b) { CollisionManager::Area::sUpdateSimStateSTBC = b; }

    void DetectCollision(int area, float inDt, float gameTime, const bool printOut );
    void SolveCollision(int area, float inDt, float gameTime);

protected:
    CollisionManager();
    ~CollisionManager();
    
    void EndDetectCollision();

    CollisionDetector mCollisionDetector;
    ImpulseBasedCollisionSolver mImpulseBasedCollisionSolver;
    
    CollisionManagerAttributes mCMAttributes;
    
    class Area
    {
    public:
        Area();
        ~Area();
        
        void Reset();
        void AddCollisionObject(CollisionObject* inObject);
        bool RemoveCollisionObject(CollisionObject* inObject);
        void RemovePair(CollisionObject* inObjectA, CollisionObject* inObjectB);
        void AddPair(CollisionObject* inObjectA, CollisionObject* inObjectB);
        
        void AddCollision(const Collision& inColl) { mCollisionList.Add(inColl); }
        void ClearCollisions() { mCollisionList.Clear(); }
        
        void PrepareForCollisionDetection(CollisionManagerAttributes inCDAttributes);
        void EndCollisionDetection(CollisionManagerAttributes inCDAttributes);
        
        TList<CollisionObject*> mObjectList;
        TArray<CollisionObjectPair> mPairList;
        TArray<Collision> mCollisionList;

        // mUseExclusiveAutoPair allows to set the autopair behaviour.
        // if true, a pair is create if two object have autopair=false
        // if false, a pair is create if one object has autopair=false
        bool mUseExclusiveAutoPair;
        bool mActivated;

        void SetUseExclusiveAutoPair(bool b) { mUseExclusiveAutoPair = b; }
        // nb. this setting should done only prior to add the objects in the areas.
    
        void DetectRayIntersection( TArray<RayIntersectionInfo>& inList, 
                                    const rmt::Vector& sourceOfRay, 
                                    const rmt::Vector& ray,
                                    rmt::Vector& sray,
                                    float& rayLen,
                                    bool inStaticObjectOnly);
        static bool sUpdateSimStateSTBC;
    };
    
    Area* mAreaArray;
    int mNbArea;

    TArray<int> mCacheActivatedArea;

    private:
        static CollisionManager* sInstance;
};

} // sim


#endif // _COLLISIONMANAGER_HPP_