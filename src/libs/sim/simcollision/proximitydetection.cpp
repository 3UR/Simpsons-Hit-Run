//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================



#include "simcollision/proximitydetection.hpp"
#include "simcommon/simmath.hpp"
#include "simcollision/collisionobject.hpp"

namespace sim
{


RayIntersectionMethodEnum RayIntersectionInfo::sMethod = RayIntersectionBBox;
float RayIntersectionInfo::sRayThickness = 0;
bool RayIntersectionInfo::sReturnClosestOnly = false;
bool RayIntersectionInfo::sRootOnly = true;

// returns true if position is in box
inline bool PointInBox(const rmt::Vector& position, const rmt::Vector& size)
{
    return (rmt::Fabs(position.x) <= size.x) && (rmt::Fabs(position.y) <= size.y) && (rmt::Fabs(position.z) <= size.z);
}

inline bool BoxGridSideTest(float de, float ds, float s)
{
    return ((de > s && ds > s) || (de < -s && ds < -s));
}

// returns false if ray doesn't intersect for sure, true otherwise
// ds/de is ray source/end location in box frame
inline bool BoxGridTest(const rmt::Vector& ds, const rmt::Vector& de, const rmt::Vector& size)
{
    if (BoxGridSideTest(de.x, ds.x, size.x))
    {
        return true; // no intersection
    }
    if (BoxGridSideTest(de.z, ds.z, size.z))
    {
        return true; // no intersection
    }
    if (BoxGridSideTest(de.y, ds.y, size.y))
    {
        return true; // no intersection
    }
    return false; // doesn't know
}

inline float TestRayInBBox(const rmt::Vector& source, const rmt::Vector& sray, const rmt::Vector& bsize)
{
    if (PointInBox(source, bsize))
    {
        return 0;
    }

    float tmpt;
    float t = 2.0f; // anything larger than 1
    
    if (sray.x * source.x < 0)
    {
        tmpt = (bsize.x*rmt::Sign(source.x) - source.x)/sray.x;
        if (tmpt >= 0.0f && tmpt <= 1.0f)
        {
            if (rmt::Fabs(sray.y*tmpt + source.y) < bsize.y && rmt::Fabs(sray.z*tmpt + source.z) < bsize.z)
            {
                t = tmpt;
            }
        }
    }
    if (sray.y * source.y < 0)
    {
        tmpt = (bsize.y*rmt::Sign(source.y) - source.y)/sray.y;
        if (tmpt >= 0.0f && tmpt <= 1.0f)
        {
            if (rmt::Fabs(sray.x*tmpt + source.x) < bsize.x && rmt::Fabs(sray.z*tmpt + source.z) < bsize.z)
            {
                t = rmt::Min(t, tmpt);
            }
        }
    }
    if (sray.z * source.z < 0)
    {
        tmpt = (bsize.z*rmt::Sign(source.z) - source.z)/sray.z;
        if (tmpt >= 0.0f && tmpt <= 1.0f)
        {
            if (rmt::Fabs(sray.x*tmpt + source.x) < bsize.x && rmt::Fabs(sray.y*tmpt + source.y) < bsize.y)
            {
                t = rmt::Min(t, tmpt);
            }
        }
    }
    return t;
}

bool RayIntersectSphere(    const rmt::Vector& source,
                            const rmt::Vector& ray, 
                            float rayLen, 
                            float radius,
                            float& value)
{
    float radius2 = rmt::Sqr(radius);

    if (source.MagnitudeSqr() <= radius2)
    {
        value = 0;
        return true; // source is in sphere
    }

    float source_dot_ray = source.DotProduct(ray);

    if (source_dot_ray >= 0)
    {
        return false; // ray points away from sphere
    }

    rmt::Vector t;
    t.ScaleAdd(source, -source_dot_ray, ray);
    float t2 = t.MagnitudeSqr();
    
    if (t2 < radius2)
    {
        float crossLen = rmt::Sqrt(radius2 - t2);
        t.ScaleAdd(-crossLen, ray); // closest intersection point with sphere for sphere at origin
        
        t.Sub(source); // vector from point on sphere to source of ray;
        t2 = t.MagnitudeSqr();
        if (t2 <= rmt::Sqr(rayLen))
        {
            value = rmt::Sqrt(t2);
            return true;
        }
    }
    return false;
}

bool DetectRayIntersectSphere(  rmt::Vector source,
                                rmt::Vector end, 
                                const rmt::Vector& position, 
                                float radius,
                                float rayThickness,
                                rmt::Vector* hitPosition)
{
    source.Sub(position);
    end.Sub(position);
    float value = radius+rayThickness;
    rmt::Vector ray(value, value, value);

    if (BoxGridTest(source, end, ray))
    {
        return false; // quick rejection
    }

    ray.Sub(end, source);
    float rayLen = ray.NormalizeSafe();

    if (RayIntersectSphere( source, ray, rayLen, radius+rayThickness, value))
    {
        if (hitPosition)
        {
            hitPosition->Add(source, position);
            hitPosition->ScaleAdd(value, ray);
        }
        return true;
    }
    return false;
}

bool RayIntersectBBox(      const rmt::Vector& source, 
                            const rmt::Vector& end, 
                            const rmt::Vector& sray,
                            const rmt::Vector& bsize,
                            float& value)
{
    /* BoxGridTest should be done before calling RayIntersectBBox
    if (BoxGridTest(source, end, bsize))
    {
        return false; // quick rejection
    }
    */

    value = TestRayInBBox(source, sray, bsize);

    if (value >= 0 && value <= 1.0f)
        return true;

    return false;
}

bool DetectRayIntersectBBox(rmt::Vector source, 
                            rmt::Vector end, 
                            const rmt::Vector& position,
                            rmt::Vector bsize,
                            float rayThickness,
                            rmt::Vector* hitPosition)
{
    source.Sub(position);
    end.Sub(position);
    bsize.x += rayThickness;
    bsize.y += rayThickness;
    bsize.z += rayThickness;

    if (BoxGridTest(source, end, bsize))
    {
        return false; // quick rejection
    }
    rmt::Vector sray;
    sray.Sub(end, source);
    float value;

    if (RayIntersectBBox(   source, 
                            end, 
                            sray, 
                            bsize,
                            value))
    {
        if (hitPosition)
        {
            hitPosition->Add(source, position);
            hitPosition->ScaleAdd(value, sray);
        }
        return true;
    }
    return false;

}

bool RayIntersectCylinder(  const rmt::Vector& s, 
                            const rmt::Vector& e, 
                            const rmt::Vector& axis,
                            float length,
                            float radius,
                            rmt::Vector& intersection)
{
    float s2 = s.MagnitudeSqr();
    float sa = s.DotProduct(axis);
    float radius2 = rmt::Sqr(radius);

    bool couldGetInthroughCap = rmt::Fabs(sa) > length;

    if (s2 <= radius2 && !couldGetInthroughCap)
    {
        intersection = s;
        return true;
    }

    rmt::Vector se;
    se.Sub(e, s);

    float ea = e.DotProduct(axis);

    if (couldGetInthroughCap)
    {
        if (ea*sa > 0) // easy tests
        {
            if (ea > length && sa > length)
                return false;
            if (-ea > length && -sa > length)
                return false;
        }

        float sol = (rmt::Sign(sa)*length - sa) / (ea - sa);

        if (BetweenP(sol, 0.01f, 0.0f, 1.0f))
        {
            rmt::Vector q1, test;
            q1.ScaleAdd(s, sol, se);
            //rAssert(rmt::Fabs(rmt::Fabs(q1.DotProduct(axis))-length) < length*0.01f);
            test.ScaleAdd(q1, -rmt::Sign(sa)*length, axis);
            if (test.MagnitudeSqr() <= radius2)
            {
                intersection = q1;
                return true;
            }
        }
    }

    float sa2 = rmt::Sqr(sa);
    float sea = ea-sa;
    float A = se.MagnitudeSqr() - rmt::Sqr(sea);
    float B = 2.0f * (se.DotProduct(s) - sa*sea);
    float C = s2 - sa2 - radius2;

    float B2_minus_4AC = rmt::Sqr(B) - 4.0f*A*C;

    if (B2_minus_4AC < 0)
    {
        return false;
    }

    B2_minus_4AC = rmt::Sqrt(B2_minus_4AC);

    float sol1 = (-B + B2_minus_4AC) / (2.0f * A);
    float sol2 = (-B - B2_minus_4AC) / (2.0f * A);
    
    bool sol1Valid = BetweenP(sol1, 0.01f, 0.0f, 1.0f);
    bool sol2Valid = BetweenP(sol2, 0.01f, 0.0f, 1.0f);

    float ls1=0, ls2=0;
    rmt::Vector q1, q2;

    if (sol1Valid)
    {
        q1.ScaleAdd(s, sol1, se);
        ls1 = q1.DotProduct(axis);
        sol1Valid = rmt::Fabs(ls1) <= length;
    }

    if (sol2Valid)
    {
        q2.ScaleAdd(s, sol2, se);
        ls2 = q2.DotProduct(axis);
        sol2Valid = rmt::Fabs(ls2) <= length;
    }

    if (sol1Valid || sol2Valid)
    {
        if (sol1Valid && sol2Valid)
        {
            intersection = (sol1 < sol2 ? q1 : q2);
        }
        else if (sol1Valid)
        {
            intersection = q1;
        }
        else
        {
            intersection = q2;
        }
        return true;
    }
    else
    {
        return false;
    }
}

bool DetectRayIntersectCylinder(    rmt::Vector s, 
                                    rmt::Vector e,
                                    const rmt::Vector& position,
                                    const rmt::Vector& axis,
                                    float length,
                                    float radius,
                                    float rayThickness,
                                    rmt::Vector* intersection)
{
    s.Sub(position);
    e.Sub(position);
    float lr = length + radius + rayThickness;
    rmt::Vector hitPosition(lr, lr, lr);

    if (BoxGridTest(s, e, hitPosition))
    {
        return false; // quick rejection
    }

    if (RayIntersectCylinder(   s, 
                                e, 
                                axis, 
                                length+rayThickness, 
                                radius+rayThickness, 
                                hitPosition))
    {
        if (intersection)
        {
            hitPosition.Add(position);
            *intersection = hitPosition;
        }
        return true;
    }
    return false;
}

bool RayIntersectOBBox(     const rmt::Vector& source, 
                            const rmt::Vector& end, 
                            const rmt::Vector& sray, 
                            float thickness,
                            OBBoxVolume* box,
                            float& value)
{
    /* assumes done by the caller
    if (BoxGridTest(source, end, volume->mBoxSize, info.sRayThickness))
    {
        return false; // quick rejection
    }
    */

    rmt::Vector ts = source;
    rmt::Vector tsray = sray;
    rmt::Vector size = *((rmt::Vector*)box->mLength);
    size.x += thickness;    
    size.y += thickness;
    size.z += thickness;

    if (box->mAxisOrientation == VolAxisNotOriented)
    {
        rmt::Matrix m;
        m.Identity();
        m.Row(0) = box->mAxis[0];
        m.Row(1) = box->mAxis[1];
        m.Row(2) = box->mAxis[2];
        m.InvertOrtho();

        m.RotateVector(source, &ts);
        m.RotateVector(sray, &tsray);

        rmt::Vector te;
        m.RotateVector(end, &te);

        if (BoxGridTest(ts, te, size))
        {
            return false; // quick rejection
        }
    }

    value = TestRayInBBox(ts, tsray, size);

    if (value >= 0 && value <= 1.0f)
        return true;

    return false;
}

bool DetectRayIntersectOBBox(rmt::Vector source, 
                            rmt::Vector end, 
                            OBBoxVolume* box,
                            float rayThickness,
                            rmt::Vector* hitPosition)
{
    source.Sub(box->mPosition);
    end.Sub(box->mPosition);
    rmt::Vector sray(rayThickness, rayThickness, rayThickness);
    sray.Add(box->mBoxSize);

    if (BoxGridTest(source, end, sray))
    {
        return false; // quick rejection
    }

    sray.Sub(end, source);
    float value;

    if (RayIntersectOBBox(source, end, sray, rayThickness, box, value))
    {
        if (hitPosition)
        {
            hitPosition->Add(source, box->mPosition);
            hitPosition->ScaleAdd(value, sray);
        }
        return true;
    }
    return false;
}

/*
    Find the closest point of intersection of the ray with the collision volume hierarchy
*/
bool RayIntersectVolume(    const rmt::Vector& sourceOfRay, 
                            const rmt::Vector& ray, 
                            const rmt::Vector& sray, 
                            float rayLen, 
                            RayIntersectionInfo& info)
{
    if (info.mCollisionVolume->IsVisible() == false)
        return false;

    // this might be a sub volume that still think its updated. need to ask the parent
    if (!info.mCollisionVolume->GetCollisionObject()->GetCollisionVolume()->Updated())
        info.mCollisionVolume->UpdateAll();

    rmt::Vector source = sourceOfRay;
    source.Sub(info.mCollisionVolume->mPosition);
    rmt::Vector end = source;
    end.Add(sray);

    rmt::Vector size(info.sRayThickness, info.sRayThickness, info.sRayThickness);

    if (info.sMethod == RayIntersectionBSphere && info.mCollisionVolume->mSphereRadius > 0)
    {
        size.x += info.mCollisionVolume->mSphereRadius;
        size.y = size.z = size.x;
    }
    else
    {
        size.Add(info.mCollisionVolume->mBoxSize);
    }

    if (BoxGridTest(source, end, size))
    {
        return false; // quick rejection
    }

    bool ret = false;

    if (info.mCollisionVolume->Type() == BBoxVolumeType)
    {
        float value;
        bool isRootVolume = info.mCollisionVolume->GetCollisionObject()->GetCollisionVolume() == info.mCollisionVolume;

        if (info.sMethod == RayIntersectionBSphere && isRootVolume)
        {
            ret = RayIntersectSphere(   source, 
                                        ray, 
                                        rayLen, 
                                        info.mCollisionVolume->mSphereRadius + info.sRayThickness, 
                                        info.mDistFromSource);
        }
        else
        {
            ret = RayIntersectBBox( source, 
                                    end, 
                                    sray, 
                                    size, 
                                    value);
            if (ret && info.sRootOnly)
            {
                info.mDistFromSource = value * rayLen;
            }
        }
        if (ret && (info.sMethod == RayIntersectionVolume || !info.sRootOnly))
        {
            info.mDistFromSource = VERY_LARGE;
            RayIntersectionInfo subinfo;
            CollisionVolume* volume = info.mCollisionVolume;
            rmt::Vector subsray = sray;
            float subrayLen = rayLen;
            bool found = false;
            for (int i=0; i<volume->SubVolumeList()->GetSize(); i++)
            {
                subinfo.mCollisionVolume = volume->SubVolumeList()->GetAt(i);

                if (RayIntersectVolume(     sourceOfRay, 
                                            ray, 
                                            subsray, 
                                            subrayLen, 
                                            subinfo) 
                    && subinfo.mDistFromSource < info.mDistFromSource)
                {
                    info = subinfo;
                    found = true;

                    // can shorten the ray for the other voluemes since 
                    // we only want the closest one
                    //rAssert(subinfo.mDistFromSource < subrayLen);
                    subrayLen = subinfo.mDistFromSource;
                    subsray.Scale(subrayLen, ray);
                }
            }
            ret = found;
        }
    }
    else if (info.mCollisionVolume->Type() == OBBoxVolumeType && info.sMethod == RayIntersectionVolume)
    {
        float value;
        if (RayIntersectOBBox(  source, 
                                end, 
                                sray, 
                                info.sRayThickness, 
                                (OBBoxVolume*)info.mCollisionVolume, 
                                value))
        {
            info.mDistFromSource = value * rayLen;
            ret = true;
        }
    }
    else if (info.mCollisionVolume->Type() == CylinderVolumeType && info.sMethod == RayIntersectionVolume)
    {
        CylinderVolume* c = (CylinderVolume*) info.mCollisionVolume;
        float len = c->mLength;
        if (!c->mFlatEnd)
            len += c->mCylinderRadius;
        rmt::Vector intersection;

        if (RayIntersectCylinder(   source, 
                                    end, 
                                    c->mAxis, 
                                    len+info.sRayThickness, 
                                    c->mCylinderRadius+info.sRayThickness, 
                                    intersection))
        {
            intersection.Sub(source);
            info.mDistFromSource = intersection.Magnitude();
            ret = true;
        }
    }
    else if ((info.mCollisionVolume->Type() == SphereVolumeType && info.sMethod == RayIntersectionVolume) || info.sMethod == RayIntersectionBSphere)
    {
        ret = RayIntersectSphere(   source, 
                                    ray, 
                                    rayLen, 
                                    info.mCollisionVolume->mSphereRadius + info.sRayThickness, 
                                    info.mDistFromSource);
    }
    else // BBox test
    {
        float value;

        if (RayIntersectBBox(       source, 
                                    end, 
                                    sray, 
                                    size, 
                                    value))
        {
            info.mDistFromSource = value * rayLen;
            ret = true;
        }
    }

    return ret;
}

bool ApproxRayIntersectVolume(  const rmt::Vector& sourceOfRay, 
                                const rmt::Vector& ray, 
                                const rmt::Vector& sray, 
                                float rayLen,
                                CollisionVolume* inVolume,
                                ApproxRayIntersectionInfo& info)
{
    if (inVolume->IsVisible() == false)
        return false;

    bool isRootVolume = inVolume->GetCollisionObject()->GetCollisionVolume() == inVolume;
    
    rmt::Vector source = sourceOfRay;
    source.Sub(inVolume->mPosition);

    if (isRootVolume || inVolume->Type() != BBoxVolumeType)
    {
        rmt::Vector dp;
        dp.ScaleAdd(source, -source.DotProduct(ray), ray);
        if (dp.MagnitudeSqr() > rmt::Sqr(info.mRayThickness + inVolume->mSphereRadius))
        {
            return false; // quick rejection;
        }
    }

    // this might be a sub volume that still think its updated. need to ask the parent
    if (    !inVolume->GetCollisionObject()->GetCollisionVolume()->Updated() && 
            (   info.mMethod == RayIntersectionVolume || 
                !info.mRootOnly || 
                !isRootVolume || 
                inVolume->GetCollisionObject()->GetSimState()->GetControl() == simSimulationCtrl
            )
       )
        inVolume->UpdateAll();

    rmt::Vector end = source;
    end.Add(sray);

    rmt::Vector size(info.mRayThickness, info.mRayThickness, info.mRayThickness);
    
    if (info.mMethod == RayIntersectionBSphere && inVolume->mSphereRadius > 0)
    {
        size.x += inVolume->mSphereRadius;
        size.y = size.z = size.x;
    }
    else
    {
        size.Add(inVolume->mBoxSize);
    }

    if (BoxGridTest(source, end, size))
    {
        return false; // quick rejection
    }

    if (inVolume->Type() == BBoxVolumeType)
    {
        bool ret = false;
        float value;
        
        if (info.mMethod == RayIntersectionBSphere && isRootVolume)
        {
            // nb the mSphereRadius is updated only for the rootvolume. see BBoxVolume::UpdateBBox().
            // could be changed if necessary.
            ret = RayIntersectSphere(   source, 
                                        ray, 
                                        rayLen, 
                                        inVolume->mSphereRadius + info.mRayThickness, 
                                        value);
            if (info.mRootOnly)
            {
                if (ret)
                {
                    info.mFirstHit = inVolume;
                    info.mHitPosition = sourceOfRay;
                    info.mHitPosition.ScaleAdd(value, ray);
                }
                return ret;
            }
        }
        else
        {
            ret = RayIntersectBBox( source, 
                                    end, 
                                    sray, 
                                    size, 
                                    value);
            if (info.mRootOnly && info.mMethod == RayIntersectionBBox)
            {
                if (ret)
                {
                    info.mFirstHit = inVolume;
                    info.mHitPosition = sourceOfRay;
                    info.mHitPosition.ScaleAdd(value, sray);
                }
                return ret;
            }
        }
        if (ret)
        {
            for (int i=0; i<inVolume->SubVolumeList()->GetSize(); i++)
            {
                if (ApproxRayIntersectVolume(   sourceOfRay, 
                                                ray, 
                                                sray, 
                                                rayLen,
                                                inVolume->SubVolumeList()->GetAt(i),
                                                info))
                {
                    return true;
                }
            }
        }
    }
    else if (inVolume->Type() == OBBoxVolumeType && info.mMethod == RayIntersectionVolume)
    {
        float value;
        if (RayIntersectOBBox(  source, 
                                end, 
                                sray, 
                                info.mRayThickness, 
                                (OBBoxVolume*)inVolume, 
                                value))
        {
            info.mFirstHit = inVolume;
            info.mHitPosition = sourceOfRay;
            info.mHitPosition.ScaleAdd(value, sray);
            return true;
        }
    }
    else if (inVolume->Type() == CylinderVolumeType && info.mMethod == RayIntersectionVolume)
    {
        CylinderVolume* c = (CylinderVolume*) inVolume;
        float len = c->mLength;
        if (!c->mFlatEnd)
            len += c->mCylinderRadius;

        if (RayIntersectCylinder(   source, 
                                    end, 
                                    c->mAxis, 
                                    len+info.mRayThickness, 
                                    c->mCylinderRadius+info.mRayThickness, 
                                    info.mHitPosition))
        {
            info.mFirstHit = inVolume;
            info.mHitPosition.Add(inVolume->mPosition);
            return true;
        }
    }
    else if (inVolume->Type() == SphereVolumeType || info.mMethod == RayIntersectionBSphere)
    {
        float value;

        if (RayIntersectSphere( source, 
                                ray, 
                                rayLen, 
                                inVolume->mSphereRadius + info.mRayThickness, 
                                value))
        {
            info.mFirstHit = inVolume;
            info.mHitPosition = sourceOfRay;
            info.mHitPosition.ScaleAdd(value, ray);
            return true;
        }
    }
    else // BBox test
    {
        float value;

        if (RayIntersectBBox(   source, 
                                end, 
                                sray, 
                                size, 
                                value))
        {
            info.mFirstHit = inVolume;
            info.mHitPosition = sourceOfRay;
            info.mHitPosition.ScaleAdd(value, sray);
            return true;
        }
    }

    return false;
}

//
//
//

bool TestPointInVolume(const rmt::Vector& point, CollisionVolume* inVolume)
{
    bool ret = false;

    if (!inVolume->Updated())
        inVolume->UpdateAll();

    rmt::Vector dp;
    dp.Sub(point, inVolume->mPosition);

    switch (inVolume->Type())
    {
    case BBoxVolumeType:
        {
            if (    rmt::Fabs(dp.x) <= inVolume->mBoxSize.x && 
                    rmt::Fabs(dp.y) <= inVolume->mBoxSize.y && 
                    rmt::Fabs(dp.z) <= inVolume->mBoxSize.z )
            {
                for (int i=0; i<inVolume->SubVolumeList()->GetSize(); i++)
                {
                    if (TestPointInVolume(point, inVolume->SubVolumeList()->GetAt(i)))
                    {
                        ret = true;
                        break;
                    }
                }
            }
        }
        break;
    case SphereVolumeType:
        {
            ret = dp.MagnitudeSqr() <= rmt::Sqr(((SphereVolume*)inVolume)->mSphereRadius);
        }
        break;
    case CylinderVolumeType:
        {
            CylinderVolume* cyl = (CylinderVolume*)inVolume;
            float dl = cyl->mAxis.DotProduct(dp);
            if (rmt::Fabs(dl) <= cyl->mLength + (cyl->mFlatEnd ? 0 : cyl->mCylinderRadius))
            {
                dp.ScaleAdd(-dl, cyl->mAxis);
                if (dp.MagnitudeSqr() <= cyl->mCylinderRadius)
                {
                    ret = true;
                }
            }
        }
        break;
    case OBBoxVolumeType:
        {
            OBBoxVolume* box = (OBBoxVolume*)inVolume;
            if (box->mAxisOrientation == VolAxisNotOriented)
            {
                if (    rmt::Fabs(dp.DotProduct(box->mAxis[0])) <= box->mLength[0] && 
                        rmt::Fabs(dp.DotProduct(box->mAxis[1])) <= box->mLength[1] && 
                        rmt::Fabs(dp.DotProduct(box->mAxis[2])) <= box->mLength[2] )
                {
                    ret = true;
                }
            }
            else
            {
                if (    rmt::Fabs(dp.x) <= box->mLength[0] && 
                        rmt::Fabs(dp.y) <= box->mLength[1] && 
                        rmt::Fabs(dp.z) <= box->mLength[2] )
                {
                    ret = true;
                }
            }
        }
        break;
    case CollisionVolumeType:
    case WallVolumeType:
    case MaxCollisionVolumeEnum:
        break;
    }
    return ret;
}


//
//
//

inline void FindBoxCoordAndInOutBool(float point, float sidelength, float& coordinate, bool& inout)
{
    if(point < -sidelength)			
    {
        coordinate = -sidelength;
        inout = false; 
    }	
    else if(point< 0.0f)			
    {
        coordinate = point;
        inout = true;
    }	
    else if(point < sidelength)			
    {
        coordinate = point;
        inout = true;
    }	
    else
    {
        coordinate = sidelength;
        inout = false;
    }
}

CollisionVolume* FindClosestPointOnVolume(const rmt::Vector& inPos, CollisionVolume* inVolume, rmt::Vector& outNormal, float& outDist)
{
    CollisionVolume* pOutVolume = inVolume;

    switch (inVolume->Type())
    {
    case SphereVolumeType:
        {
            outNormal.Sub(inPos, inVolume->mPosition);
            outDist = outNormal.NormalizeSafe();
            if (outDist > VERY_SMALL)
            {
                outDist -= inVolume->mSphereRadius;
            }
            else
            {
                outDist = 0;
                outNormal.Set(1.0f, 0, 0);
            }
        }
        break;
    case CylinderVolumeType:
        {
            rmt::Vector pos = inPos;
            pos.Sub(inVolume->mPosition);
            float radius = pos.Magnitude();
            pos = inPos;
            CylinderVolume* cyl = static_cast<CylinderVolume*>(inVolume);
            if (false) //mCollisionDetector.CylSphereColl(cyl, pos, radius, outNormal, outDist))
            {
                outNormal.Sub(inPos, inVolume->mPosition);
                outDist = outNormal.NormalizeSafe() - inVolume->mSphereRadius;
            }
            else
            {
                rmt::Vector testPoint = inPos;
                rmt::Vector nearestPoint;
                rmt::Matrix cylWorldMat, invCylWorldMat;
                cylWorldMat.Identity();

                // Put point into bbox frame
                if (cyl->mAxisOrientation == VolAxisNotOriented)
                {
                    rmt::Vector axis;
                    float angle;
                    static rmt::Vector Yaxis(0.0f, 1.0f, 0.0f);

                    sim::AxisAngle(cyl->mAxis, Yaxis, axis, angle);
                    static float ssign = 1.0f;

                    cylWorldMat.FillRotation(axis, -angle);
                    cylWorldMat.Row(3) = cyl->mPosition;
                    invCylWorldMat = cylWorldMat;
                    invCylWorldMat.InvertOrtho();
                    testPoint.Transform(invCylWorldMat);
                }
                else if (cyl->mAxisOrientation != VolAxisYOriented)
                {
                    if (cyl->mAxisOrientation == VolAxisXOriented)
                    {
                        rmt::Swap(testPoint.x, testPoint.y);
                        testPoint.Sub(cyl->mPosition);
                    }
                    else
                    {
                        rmt::Swap(testPoint.y, testPoint.z);
                        testPoint.Sub(cyl->mPosition);
                    }
                }
                else
                {
                    testPoint.Sub(cyl->mPosition);
                }

                
                // Put point into cylindrical coords 
                // (assumes cylinders are y-axis in obj space)

                float y,r, R;
                bool yIn, rIn;
                y = testPoint.y;
                r = rmt::Sqrt((testPoint.x)*(testPoint.x) + (testPoint.z)*(testPoint.z));
                R = cyl->mCylinderRadius;
                if (r < R)
                {
                    rIn = true;
                    nearestPoint.x = testPoint.x;
                    nearestPoint.z = testPoint.z;
                }
                else
                {
                    rIn = false;
                    nearestPoint.x = testPoint.x *R/r;
                    nearestPoint.z = testPoint.z *R/r;
                }
                
                if(y < -cyl->mLength)			
                {
                    yIn = false;
                    if(cyl->mFlatEnd)
                    {
                        y = -cyl->mLength;
                    }
                    else
                    {
                        rmt::Vector c;
                        c.Set(0,-cyl->mLength, 0);
                        rmt::Vector n;
                        n.Sub(testPoint, c);
                        n.NormalizeSafe();
                        n.Scale(R);
                        nearestPoint.Add(c, n);
                        y = nearestPoint.y;
                    }
                }
                else if(y < 0.0f)
                {
                    yIn = true;
                }
                else if(y <cyl->mLength)
                {
                    yIn = true;
                }
                else
                {
                    yIn = true;
                    if(cyl->mFlatEnd)
                    {
                        y = cyl->mLength;
                    }
                    else
                    {
                        rmt::Vector c;
                        c.Set(0,cyl->mLength, 0);
                        rmt::Vector n;
                        n.Sub(testPoint, c);
                        n.NormalizeSafe();
                        n.Scale(R);
                        nearestPoint.Add(c, n);
                        y = nearestPoint.y;
                    }
                }
                nearestPoint.y = y;
                
                outNormal.Sub(testPoint, nearestPoint);
                outDist = outNormal.Magnitude();

                if (cyl->mAxisOrientation == VolAxisNotOriented)
                    outNormal.Rotate(cylWorldMat);  
                else if (cyl->mAxisOrientation == VolAxisXOriented)
                {
                    rmt::Swap(outNormal.x, outNormal.y);
                }
                else if (cyl->mAxisOrientation == VolAxisZOriented)
                {
                    rmt::Swap(outNormal.z, outNormal.y);
                }
    
                outNormal.NormalizeSafe();
                if(false )// FIXME yIn && rIn)
                    outDist = -outDist;
                
            }
            //            else
            //               rAssert(false); // should not get there
        }
        break;
    case OBBoxVolumeType:
        {
            rmt::Vector testPoint = inPos;
            rmt::Matrix obbWorldMat, invObbWorldMat;
            OBBoxVolume* box = static_cast<OBBoxVolume*>(inVolume);

            if (box->mAxisOrientation == VolAxisNotOriented)
            {
                obbWorldMat.Identity();
            
                // Put point into bbox frame
                obbWorldMat.Row(0) = box->mAxis[0];
                obbWorldMat.Row(1) = box->mAxis[1];
                obbWorldMat.Row(2) = box->mAxis[2];
                obbWorldMat.Row(3) = box->mPosition;
                invObbWorldMat = obbWorldMat;
                invObbWorldMat.Invert();
                testPoint.Transform(invObbWorldMat);
            }
            else
            {
                testPoint.Sub(box->mPosition);
            }
            
            // find coordinates of nearest point, take note if point is inside bbox
            float a,b,c, minDist;
            rmt::Vector nearestPoint;
            bool VxIn, VyIn, VzIn;

            a = box->mLength[0], b = box->mLength[1], c = box->mLength[2];
            
            FindBoxCoordAndInOutBool(testPoint.x, a, nearestPoint.x, VxIn);
            FindBoxCoordAndInOutBool(testPoint.y, b, nearestPoint.y, VyIn); 
            FindBoxCoordAndInOutBool(testPoint.z, c, nearestPoint.z, VzIn);
            
            if(VxIn && VyIn && VzIn)
            {
                float clamp;
                float* coord;
                minDist = rmt::Fabs(nearestPoint.x-testPoint.x);
                coord = &nearestPoint.x;
                clamp = rmt::Sign(nearestPoint.x) * a;
                if(rmt::Fabs(nearestPoint.y-testPoint.y) < minDist)
                {
                    coord = &nearestPoint.y;
                    minDist = rmt::Fabs(nearestPoint.y - testPoint.y);
                    clamp = rmt::Sign(nearestPoint.y) * b;
                }
                if(rmt::Fabs(nearestPoint.z-testPoint.z) < minDist)
                {
                    coord = &nearestPoint.z;
                    minDist = rmt::Fabs(nearestPoint.y - testPoint.y);
                    clamp = rmt::Sign(nearestPoint.z) * c;
                }
                *coord = clamp;
            }
            // calculate distance from testpoint to nearestpoint, negative if inside bbox
            outNormal.Sub( testPoint, nearestPoint);
            outDist = outNormal.Magnitude();
            
            if (box->mAxisOrientation == VolAxisNotOriented)
                outNormal.Rotate(obbWorldMat);

            outNormal.NormalizeSafe();
            if(VxIn && VyIn && VzIn)
                outDist = -outDist;
        }
        break;
    case WallVolumeType:
        {
            outNormal.Sub(inPos, inVolume->mPosition);
            outDist = outNormal.DotProduct(static_cast<WallVolume*>(inVolume)->mNormal);
            outNormal = static_cast<WallVolume*>(inVolume)->mNormal;
        }
        break;
    case BBoxVolumeType:
        {
            float currentDist = VERY_LARGE;
            rmt::Vector currentNormal;
            bool firstTime = true;
            for (int i=0; i<inVolume->SubVolumeList()->GetSize(); i++)
            {
                CollisionVolume* pOutSubVolume = FindClosestPointOnVolume(inPos, inVolume->SubVolumeList()->GetAt(i), currentNormal, currentDist);

                // If first time in loop, want to use currentDist
                if ( firstTime )
                {
                    outDist = currentDist + 1.f;
                    firstTime = false;
                }
                if (currentDist < outDist)
                {
                    outDist = currentDist;
                    outNormal = currentNormal;
                    pOutVolume = pOutSubVolume;
                }
            }
        }
        break;
    case MaxCollisionVolumeEnum:
    case CollisionVolumeType:
        break;
    }
    return pOutVolume;
}

//
//
//

void RayIntersectionInfo::Add(TArray<RayIntersectionInfo>& inList, RayIntersectionInfo& info, const rmt::Vector& ray, rmt::Vector& sray, float& rayLen)
{
    if (RayIntersectionInfo::ReturnClosestOnly())
    {
        if (inList.GetSize() == 0)
            inList.NewElem() = info;
        else if (info.mDistFromSource < inList[0].mDistFromSource)
            inList[0] = info;

        rayLen = info.mDistFromSource;
        sray.Scale(rayLen, ray);
    }
    else
    {
        bool added = false;
    
        for (int i=0; i<inList.GetSize(); i++)
        {
            if (info.mDistFromSource < inList[i].mDistFromSource)
            {
                inList.InsertAt(i, info);
                added = true;
                break;
            }
        }
        if (!added)
        {
            inList.NewElem() = info;
        }
    }
}



} // sim
