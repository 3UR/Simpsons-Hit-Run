
#ifndef _PROXIMITY_DETECTION_HPP_
#define _PROXIMITY_DETECTION_HPP_

#include "simcommon/tarray.hpp"
#include <radmath/radmath.hpp>

namespace sim
{

class CollisionObject;
class CollisionVolume;
class OBBoxVolume;

enum RayIntersectionMethodEnum { RayIntersectionBSphere, RayIntersectionBBox, RayIntersectionVolume };

class RayIntersectionInfo
{
public:
    RayIntersectionInfo()
        : mDistFromSource(0), 
          mCollisionObject(NULL), 
          mCollisionVolume(NULL) {}
    ~RayIntersectionInfo() {}
    
    float mDistFromSource;
    CollisionObject* mCollisionObject;
    CollisionVolume* mCollisionVolume;

    static void SetMethod(RayIntersectionMethodEnum inMethod)   { sMethod = inMethod; }
    static void SetReturnClosestOnly(bool b)                    { sReturnClosestOnly = b; }
    static bool ReturnClosestOnly()                             { return sReturnClosestOnly; }
    static void Add(TArray<RayIntersectionInfo>& inList, RayIntersectionInfo& info, const rmt::Vector& ray, rmt::Vector& sray, float& rayLen);
    
    // the RootOnly setting applies only to RayIntersectionBBox modes
    static void SetDetectOnRootOnly(bool b)                    { sRootOnly = b; }
    
    static RayIntersectionMethodEnum    sMethod;
    static bool                         sRootOnly;              // RayIntersectionBSphere and RayIntersectionBBox modes, indicates if we detect on subvolumes also
    static float                        sRayThickness;
    static bool                         sReturnClosestOnly;
};

class ApproxRayIntersectionInfo
{
public:
    ApproxRayIntersectionInfo() 
        : mMethod(RayIntersectionVolume),
          mRayThickness(0),
          mFirstHit(NULL),
          mHitPosition(0, 0, 0),
          mRootOnly(true)
    {}

    ~ApproxRayIntersectionInfo() {}

    void SetMethod(RayIntersectionMethodEnum inMethod)   { mMethod = inMethod; }
    
    // the RootOnly setting applies only to RayIntersectionBSphere and RayIntersectionBBox modes
    void SetDetectOnRootOnly(bool b)              { mRootOnly = b; }

    RayIntersectionMethodEnum    mMethod;
    float                        mRayThickness;

    // the algorythm will stop as soon as a volume is encountered. It is not necessary the closest.
    CollisionVolume*             mFirstHit; 
    rmt::Vector                  mHitPosition;
    bool                         mRootOnly; // RayIntersectionBSphere and RayIntersectionBBox modes, indicates if we detect on subvolumes also
};


/*
    Find the closest point of intersection of the ray with the 
    collision volume hierarchy
*/
bool RayIntersectVolume(    const rmt::Vector& sourceOfRay, 
                            const rmt::Vector& ray, 
                            const rmt::Vector& sray, 
                            float rayLen, 
                            RayIntersectionInfo& info);

/*
    Stop at the first point of intersection of the ray with the 
    collision volume hierarchy
*/
bool ApproxRayIntersectVolume(  const rmt::Vector& sourceOfRay, 
                                const rmt::Vector& ray, 
                                const rmt::Vector& sray, 
                                float rayLen,
                                CollisionVolume* inVolume,
                                ApproxRayIntersectionInfo& info);

// usefull methods:

bool DetectRayIntersectOBBox(   rmt::Vector source, 
                                rmt::Vector end, 
                                OBBoxVolume* box,
                                float rayThickness = 0,
                                rmt::Vector* hitPosition = NULL);

bool DetectRayIntersectCylinder(rmt::Vector s, 
                                rmt::Vector e,
                                const rmt::Vector& position,
                                const rmt::Vector& axis,
                                float length,
                                float radius,
                                float rayThickness = 0,
                                rmt::Vector* intersection = NULL);

bool DetectRayIntersectSphere(  rmt::Vector source,
                                rmt::Vector end, 
                                const rmt::Vector& position, 
                                float radius,
                                float rayThickness = 0,
                                rmt::Vector* hitPosition = NULL);

bool DetectRayIntersectBBox(    rmt::Vector source, 
                                rmt::Vector end, 
                                const rmt::Vector& position,
                                rmt::Vector bsize,
                                float rayThickness = 0,
                                rmt::Vector* hitPosition = NULL);

CollisionVolume* FindClosestPointOnVolume(  const rmt::Vector& inPos, 
                                            CollisionVolume* inVolume, 
                                            rmt::Vector& outNormal, 
                                            float& outDist);


bool TestPointInVolume(         const rmt::Vector& point, 
                                CollisionVolume* inVolume);


} // sim

#endif // _PROXIMITY_DETECTION_HPP_
