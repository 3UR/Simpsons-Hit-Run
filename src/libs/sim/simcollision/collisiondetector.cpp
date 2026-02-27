
#include "simcollision/collisiondetector.hpp"
#include "simcollision/collision.hpp"
#include "simcommon/simmath.hpp"

using namespace RadicalMathLibrary;

// specify if we test for the cyl ends if an axis-axis collision between the cyl has been detected
static bool FULLCYLTEST=false;

// only the deepest coll of an object with the floor is recorded
//#define DEEPEST_ONLY  //This define should stay off. Stack of boxes is many time more unstable otherwise.


namespace sim
{

float CollisionDetector::sSafeTimeBeforeCollision(10.0f);

CollisionDetector::CollisionDetector()
    : mDt(0)
{
    SetCollisionDistance(SimEnvironment::GetDefaultSimEnvironment()->CollisionDistance());
}

CollisionDetector::~CollisionDetector()
{
}

static bool SubIsBBInForWall(WallVolume* wall, CollisionVolume* other, float precision)
{
    rAssert(other->Type() != WallVolumeType);//Two walls is not supported.
    float dist = other->mPosition.y - wall->mPosition.y;
    if (wall->mBoxSize.y == 0.0f)
    {
        if ( dist*wall->mNormal.y > other->mBoxSize.y + precision) return false;
    }
    else
    {
        if (Fabs(dist) > other->mBoxSize.y + wall->mBoxSize.y + precision) return false;
    }
    dist = (other->mPosition.x - wall->mPosition.x);
    if (wall->mBoxSize.x == 0.0f)
    {
        if ( dist*wall->mNormal.x > other->mBoxSize.x + precision) return false;
    }
    else
    {
        if (Fabs(dist) > other->mBoxSize.x + wall->mBoxSize.x + precision) return false;
    }
    dist = other->mPosition.z - wall->mPosition.z;
    if (wall->mBoxSize.z == 0.0f)
    {
        if ( dist*wall->mNormal.z > other->mBoxSize.z + precision) return false;
    }
    else
    {
        if (Fabs(dist) > other->mBoxSize.z + wall->mBoxSize.z + precision) return false;
    }
    return true;
}

// return true if interference
static inline bool IsBBIn(CollisionVolume* thisEl, CollisionVolume* el, float precision)
{
    if(thisEl->Type() == WallVolumeType)
    {
        return SubIsBBInForWall((WallVolume *)thisEl, el, precision);
    }
    else if(el->Type() == WallVolumeType)
    {
        return SubIsBBInForWall((WallVolume *)el, thisEl, precision);
    }
    else
    {
        if (Fabs(el->mPosition.x - thisEl->mPosition.x) > thisEl->mBoxSize.x + el->mBoxSize.x + precision) return false;
        if (Fabs(el->mPosition.z - thisEl->mPosition.z) > thisEl->mBoxSize.z + el->mBoxSize.z + precision) return false;
        if (Fabs(el->mPosition.y - thisEl->mPosition.y) > thisEl->mBoxSize.y + el->mBoxSize.y + precision) return false;
    }    
    return true;
}

void CollisionDetector::DetectCollision(TArray<CollisionObjectPair>* inPList, TArray<Collision>* inCList, float inDt, const bool printOut )
{
    mDt = inDt;
    SetCollisionList( *inCList );
    rAssert( GetCollisionList() != NULL);
    
    for (int i=0; i < inPList->GetSize(); i++)
    {
        mNewColl = 0;
        CollisionObjectPair* p = &inPList->GetAt(i);

        if (p->mNeedsUpdate)
        {
            if(!p->mObjectA->IsStatic() )
            {
                p->VolumeA()->UpdateAll();
            }

            if(!p->mObjectB->IsStatic())
            {
                p->VolumeB()->UpdateAll();
            }

            #ifdef RAD_DEBUG
            if( printOut )
            {
                const char* nameA = p->mObjectA->GetName();
                const char* nameB = p->mObjectB->GetName();
                rDebugPrintf( "\t%s\t\t%s\n", nameA, nameB );
            }
            #endif

            mCollisionDistance = mMinCollisionDistance;
            DetectCollision(p->VolumeA(), p->VolumeB());
            p->mCollide = mNewColl;
        }
    }
}

void CollisionDetector::DetectCollision(CollisionVolume* volA, CollisionVolume* volB, TArray<Collision>* inCList)
{
    mCollisionDistance = mMinCollisionDistance;
    SetCollisionList( *inCList );
    rAssert( inCList != NULL);
    
    DetectCollision(volA, volB);
}

void CollisionDetector::DetectOnSubVolume(CollisionVolume* volA, CollisionVolume* volB)
{
    // test for collision on all the subVolume of volA with all the subEl of volB
    TList<CollisionVolume*>* lsta = volA->SubVolumeList();
    TList<CollisionVolume*>* lstb = volB->SubVolumeList();
    
    if (lsta != NULL && lstb != NULL)
    {
        for (int i=0; i<lsta->GetSize(); i++)
        {
            if (IsBBIn(volB, (*lsta)[i], mCollisionDistance))
            {
                for (int j=0; j<lstb->GetSize(); j++)
                {
                    if (IsBBIn(volA, (*lstb)[j], mCollisionDistance))
                        DetectCollision((*lsta)[i], (*lstb)[j]);
                }
            }
        }
    }
    else if (lsta != NULL)
    {
        for (int i=0; i<lsta->GetSize(); i++)
        {
            if (IsBBIn(volB, (*lsta)[i], mCollisionDistance))
                DetectCollision(volB, (*lsta)[i]);
        }
    }
    else // lstb != NULL
    {
        for (int i=0; i<lstb->GetSize(); i++)
        {
            if (IsBBIn(volA, (*lstb)[i], mCollisionDistance))
                DetectCollision(volA, (*lstb)[i]);
        }
    }
}

void CollisionDetector::DetectCollision(CollisionVolume* volA, CollisionVolume* volB)
{
    if (!IsBBIn(volA, volB, mCollisionDistance))
    {
        return;
    }
    
    if (volA->Type() == BBoxVolumeType || volB->Type() == BBoxVolumeType)
    {
        DetectOnSubVolume(volB, volA);
        return;
    }

    if (volA->Type() > volB->Type())
    {
        DetectColl(volA, volB);
    }
    else
    {
        DetectColl(volB, volA);
    }
}

void CollisionDetector::DetectColl(CollisionVolume* volA, CollisionVolume* volB)
{
    if (!CollTestInit(volA, volB))
        return;
    
    bool testCompleted = false;

    // the following code does sphere approximation for situation where the 
    // collision distance is larger than the size of the smallest object.
    static float SphereApproxScale = 1.0f;

    if (volA->mSphereRadius < volB->mSphereRadius)
    {
        if (volA->mSphereRadius*SphereApproxScale < mCollisionDistance)
        {
            mSphereApproxA = true;

            switch(volB->Type())
            {
                case WallVolumeType:
                    DetectColl((WallVolume*)volB, volA);
                    break;
                case OBBoxVolumeType:
                    DetectBSColl((OBBoxVolume*)volB, volA, volA->mSphereRadius);
                    break;
                case CylinderVolumeType:
                    DetectCSColl((CylinderVolume*)volB, volA);
                    break;
                default:
                    DetectSSColl(volA, volB);
            }
            testCompleted = true;
            
            if (mNewColl)
            {
            }
        }
    }
    else if (volB->mSphereRadius*SphereApproxScale < mCollisionDistance)
    {
        mSphereApproxB = true;

        switch(volA->Type())
        {
            case WallVolumeType:
                DetectColl((WallVolume*)volA, volB);
                break;
            case OBBoxVolumeType:
                DetectBSColl((OBBoxVolume*)volA, volB, volB->mSphereRadius);
                break;
            case CylinderVolumeType:
                DetectCSColl((CylinderVolume*)volA, volB);
                break;
            default:
                DetectSSColl(volA, volB);
        }
        testCompleted = true;
    }

    if (!testCompleted)
    {    
        switch(volA->Type())
        {
            //case BBoxVolumeType:
            //    DetectOnSubVolume(volA, volB);
            //    break;
            case WallVolumeType:
                if (volB->Type() != WallVolumeType) 
                    DetectColl((WallVolume*)volA, volB);
                break;
            case OBBoxVolumeType:
                switch(volB->Type())
                {
                    case OBBoxVolumeType:
                        DetectColl((OBBoxVolume*)volA, (OBBoxVolume*)volB);
                        break;
                    case CylinderVolumeType:
                        DetectColl((OBBoxVolume*)volA, (CylinderVolume*)volB);
                        break;
                    case SphereVolumeType:
                        DetectBSColl((OBBoxVolume*)volA, volB, volB->mSphereRadius);
                        break;
                    default:
                        break;
                }
                break;
            case CylinderVolumeType:
                switch(volB->Type())
                {
                    case CylinderVolumeType:
                        DetectColl((CylinderVolume*)volA, (CylinderVolume*)volB);
                        break;
                    case SphereVolumeType:
                        DetectCSColl((CylinderVolume*)volA, volB);
                        break;
                    default:
                        break;
                }
                break;
            default:
                DetectSSColl(volA, volB);
        }
    }
    mSphereApproxA = false;
    mSphereApproxB = false;
}
    
void CollisionDetector::DetectWallBBox(WallVolume* wall, CollisionVolume* volA)
{
    if (volA->Type() == WallVolumeType)
    {
        return;
    }
    
    mT.Sub(volA->mPosition, wall->mPosition);
    float d = Fabs(volA->mBoxSize.DotProduct(wall->mNormal));
    mNt = mT.DotProduct(wall->mNormal);
    if (mNt < d + mCollisionDistance)
    {
        Vector pwall, pa;
        
        pa.ScaleAdd(volA->mPosition, -d, wall->mNormal);
        pwall.ScaleAdd(volA->mPosition, -mNt, wall->mNormal);
        AddCollision(volA, pa, wall, pwall, mNt-d, wall->mNormal); 
    }
}

void CollisionDetector::DetectColl(WallVolume* wall, CollisionVolume* volA)
{
    mT.Sub(volA->mPosition, wall->mPosition);
    float nt = mT.DotProduct(wall->mNormal);
    if (nt > volA->mSphereRadius + mCollisionDistance)
    {
        return;
    }
    
    float dist;
    
    if (!volA->HasSubVolume())
    {
        Vector p, pa;
        
        switch (volA->Type())
        {
        case OBBoxVolumeType:
            {
                float dst;
                dist = VERY_LARGE;
#ifdef DEEPEST_ONLY
                float tmpd;
                for (int i=0; i<4; i++)
                {
                    ((OBBoxVolume*)volA)->Corner(p, i);
                    dst = p.DotProduct(wall->mNormal);
                    tmpd = nt - Fabs(dst);
                    if (tmpd < dist)
                    {
                        // keep only the deepest collision
                        dist = tmpd;
                        pa.ScaleAdd(volA->mPosition, -Sign(dst), p);
                    }
                }
                if (dist < mCollisionDistance)
                {
                    p.ScaleAdd(pa, -dist, wall->mNormal);
                    AddCollision(volA, pa, wall, p, dist, wall->mNormal); 
                }
#else // DEEPEST_ONLY
                for (int i=0; i<4; i++)
                {
                    ((OBBoxVolume*)volA)->Corner(p, i);
                    dst = p.DotProduct(wall->mNormal);
                    dist = nt - Fabs(dst);
                    if (dist < mCollisionDistance)
                    {
                        pa.ScaleAdd(volA->mPosition, -Sign(dst), p);
                        
                        p.ScaleAdd(pa, -dist, wall->mNormal);
                        AddCollision(volA, pa, wall, p, dist, wall->mNormal); 
                    }
                }
#endif // DEEPEST_ONLY
                break;
            }
        case CylinderVolumeType:
            {
                CylinderVolume* vol = (CylinderVolume*)volA;
                
                if (vol->mFlatEnd )
                {
                    p.CrossProduct(vol->mAxis, wall->mNormal);
                    pa.CrossProduct(vol->mAxis, p);
                    pa.Normalize();
                    float f_1 = pa.DotProduct(wall->mNormal) * vol->mCylinderRadius;
                    float f_2 = vol->mAxis.DotProduct(wall->mNormal) * vol->mLength;
                    dist = nt - Fabs(f_1) - Fabs(f_2);
                    if (dist < mCollisionDistance)
                    {
                        Vector posa, posb;
                        posa.ScaleAdd(vol->mPosition, -Sign(f_1) * vol->mCylinderRadius, pa);
                        posa.ScaleAdd(posa, -Sign(f_2) * vol->mLength, vol->mAxis);
                        
                        posb.ScaleAdd(posa, -dist, wall->mNormal);
                        AddCollision(volA, posa, wall, posb, dist, wall->mNormal); 
                    }
#ifndef DEEPEST_ONLY
                    dist = nt - Fabs(f_1) + Fabs(f_2); // if axis of cylinder is parallel to wall surface.
                    if (dist < mCollisionDistance)
                    {
                        Vector posa, posb;
                        posa.ScaleAdd(vol->mPosition, -Sign(f_1) * vol->mCylinderRadius, pa);
                        posa.ScaleAdd(posa, Sign(f_2) * vol->mLength, vol->mAxis);
                        
                        posb.ScaleAdd(posa, -dist, wall->mNormal);
                        AddCollision(volA, posa, wall, posb, dist, wall->mNormal); 
                    }
#endif
                }
                else // test only the spheres
                {
                    float no = vol->mAxis.DotProduct(wall->mNormal);
                    float dist = nt - ((Fabs(no) * vol->mLength) + vol->mCylinderRadius);
                    if (dist <= mCollisionDistance)
                    {
                        pa.ScaleAdd(vol->mPosition, -Sign(no) * vol->mLength, vol->mAxis);
                        pa.ScaleAdd(pa, -vol->mCylinderRadius, wall->mNormal);
                        p.ScaleAdd(pa, -dist, wall->mNormal);
                        AddCollision(volA, pa, wall, p, dist, wall->mNormal); 
                        
#ifndef DEEPEST_ONLY
                        dist = nt - (-(Fabs(no) * vol->mLength) + vol->mCylinderRadius);
                        if (dist <= mCollisionDistance)
                        {
                            pa.ScaleAdd(vol->mPosition, -Sign(no) * vol->mLength, vol->mAxis);
                            pa.ScaleAdd(pa, -vol->mCylinderRadius, wall->mNormal);
                            p.ScaleAdd(pa, -dist, wall->mNormal);
                            AddCollision(volA, pa, wall, p, dist, wall->mNormal); 
                        }
#endif
                    }
                }
                break;
            }
        case SphereVolumeType:
        case CollisionVolumeType:
            dist = nt - volA->mSphereRadius;
            pa.ScaleAdd(volA->mPosition, -volA->mSphereRadius, wall->mNormal);
            // this is the default test done at the beginning of the funtion
            if (dist < mCollisionDistance)
            {
                p.ScaleAdd(pa, -dist, wall->mNormal);
                AddCollision(volA, pa, wall, p, dist, wall->mNormal); 
            }
            break;
        default:
            rAssertMsg(false, "Unnowned type?"); // not yet
        }
   }
   else
   {
       bool collide = false;
       
       switch (volA->Type())
       {
       case SphereVolumeType:
       case CollisionVolumeType:
           collide = true; // sphere test already done
           break;
       case CylinderVolumeType:
           {
               CylinderVolume* vol = (CylinderVolume*)volA;
               float no = vol->mAxis.DotProduct(wall->mNormal);
               float dist = nt - ((Fabs(no) * vol->mLength) + vol->mCylinderRadius);
               if (dist <= mCollisionDistance)
               {
                   collide = true;
               }
               break;
           }
       case OBBoxVolumeType:
           {
               Vector p;
               
               for (int i=0; i<4; i++)
               {
                   ((OBBoxVolume*)volA)->Corner(p, i);
                   if (nt - Fabs(p.DotProduct(wall->mNormal)) < mCollisionDistance)
                   {
                       collide = true;
                       break;
                   }
               }
               break;
           }
       default:
           rAssertMsg(false, "Type not yet implemented");
       }
       
       if (collide)
           DetectOnSubVolume(volA, wall);
   }
}

// OBBoxVolume vs OBBoxVolume
void CollisionDetector::DetectColl(OBBoxVolume* volA, OBBoxVolume* volB)
{
    //if (!volA->HasSubVolume() && !volB->HasSubVolume())
    //{
    

/*    
    const char* nameA = volA->GetCollisionObject()->GetName();
    const char* nameB = volB->GetCollisionObject()->GetName();
    
    if((strcmp(nameA, "otto_v") == 0 && strcmp(nameB, "cSedan") == 0) || (strcmp(nameB, "otto_v") == 0 && strcmp(nameA, "cSedan") == 0))    
    {    
        int stophere = 1;
        //rDebugPrintf("\n\n\n@@@  Beginning DetectColl for otto_v and cSedan\n");   
    }
*/    
    
    
    if (!OBBoxSepPlane(volA, volB))
    {
        OBBoxV(volA, volB);
        OBBoxV(volB, volA);
        OBBoxEE(volA, volB);
    }
    
    /*
    if(strcmp(nameA, "otto_v") == 0 && strcmp(nameB, "cSedan") == 0)
    {    
        rDebugPrintf("\n@@@  Ending DetectColl for otto_v and cSedan\n\n\n");   
    }
    else if(strcmp(nameB, "otto_v") == 0 && strcmp(nameA, "cSedan") == 0)
    {
        rDebugPrintf("\n@@@  Ending DetectColl for otto_v and cSedan\n\n\n");    
    }
    */
    
    
    
    //}
    //else if (!OBBoxSepPlane(volA, volB))
    //{
    //    // no separation plane if we're still here. We need to perform further test on sub Volumes
    //    DetectOnSubVolume(volA, volB);
    //}
}

void CollisionDetector::DetectColl(OBBoxVolume* volA, CylinderVolume* volB)
{
    //if (!volA->HasSubVolume() && !volB->HasSubVolume())
    //{
        // Do the complete detection
        if (!volB->mFlatEnd  )
        {
            Vector pb;
            //rmV3ScaleAdd(&pb, &volB->mPosition, &volB->mAxis, Sign(rmV3Dot(&mT, &volB->mAxis)) * volB->mLength);
            //BoxSphereColl(volA, volB, pb, volB->mCylinderRadius);
            pb.ScaleAdd(volB->mPosition, volB->mLength, volB->mAxis);
            BoxSphereColl(volA, volB, pb, volB->mCylinderRadius);
            pb.ScaleAdd(volB->mPosition, -volB->mLength, volB->mAxis);
            BoxSphereColl(volA, volB, pb, volB->mCylinderRadius);
        }
        else
        {
            OBBoxCylF(volA, volB); // collision between the cyl and the box faces

            if (mCollisionDistance == mMinCollisionDistance)
                OBBoxCylEC(volA, volB); // collision between the cyl circle and the box edges
        }
        //todo(mc): OBBoxCylF and OBBoxCylEE are responsible of the 
        //problem I saw when a cylinder make a bridge on two standing boxes. Here
        //radial normals make the cylinder pushing away the two boxes.
        if (mCollisionDistance == mMinCollisionDistance)
            OBBoxCylV(volA, volB); // collision between the cyl and the box vertices
        OBBoxCylEE(volA, volB); // collision between the cyl axis and the box edges
    //}
    //else if (!OBBoxCylSepPlane(volA, volB))
    //{
    //    // no separation plane if we're still here. We need to perform further test on sub Volumes
    //    DetectOnSubVolume(volA, volB);
    //}
}

void CollisionDetector::DetectBSColl(OBBoxVolume* volA, CollisionVolume* volB, float radius)
{
    //if (!volA->HasSubVolume() && !volB->HasSubVolume())
    //{
        BoxSphereColl(volA, volB, volB->mPosition, radius);
    //}
    //else if (PointBoxColl(volA, volB->mPosition, NULL, NULL, mNt, radius, false))
    //{
    //    DetectOnSubVolume(volA, volB);
    //}
}

void CollisionDetector::DetectColl(CylinderVolume* volA, CylinderVolume* volB)
{
    //if (!volA->HasSubVolume() && !volB->HasSubVolume())
    //{
        mT.Sub(volB->mPosition, volA->mPosition);
        
        float dist, nn;
        Vector N, pa, pb;
        
        N.CrossProduct(volA->mAxis, volB->mAxis);
        N.Scale(volA->mLength * volB->mLength);
        nn = N.DotProduct(N);
        
        //Cylinder's face versus Cylinder's face
        if (nn > VERY_SMALL) // else edges are parallel... 
        {
            float nt = mT.DotProduct(N);
            
            if (Fabs(nt) > VERY_SMALL) // else no collision possible between these edges
            {
                nn = Sqrt(nn);  // N's norm
                nt = nt / nn; // distance between cyl center along N, the collision normal
                
                float r = Sign(nt) / nn;
                float rr = r * volA->mCylinderRadius;
                pa.Scale(rr, N);
                rr = r * volB->mCylinderRadius;
                pb.Scale(-rr, N);
                dist = Fabs(nt) - volA->mCylinderRadius - volB->mCylinderRadius;
                
                if (dist < mCollisionDistance)
                {
                    if (!volA->HasSubVolume() && !volB->HasSubVolume())
                    {
                        bool dummy = false;
                        if( EdgeEdge(dummy, pa, volA->mAxis, volA->mLength, pb, volB->mAxis, volB->mLength, N, nn, dist, volA, volB) )
                        {
                            //If cylinder's side collide, there is no other possible collision between other Volumeents.
                            if( !FULLCYLTEST )
                                return;
                        }
                    }
                }
            }
        }
        else//continue Cylinder's face / Cylinder's face when cylinders's axes are parallel.
        {
            //Nota:This case is well supported by CyliderFaceEnd and/or CylCylEnd. But in the
            //case where the the radial distance is shorter than the normal distance it is
            //faster to do the following.
            Vector Tn, Tt;
            Tn.Scale(mT.DotProduct(volA->mAxis),volA->mAxis);
            Tt.Sub(mT,Tn);
            float lenTn, lenTt;
            lenTt = Tt.NormalizeSafe();
            if (lenTt > VERY_SMALL)
            {
                dist = lenTt - (volA->mCylinderRadius + volB->mCylinderRadius); //radial distance
                if ( dist <= mCollisionDistance)
                {
                    lenTn = Tn.NormalizeSafe();
                    nn   = lenTn - (volA->mLength + volB->mLength); //normal distance
                    if ( nn <= mCollisionDistance)
                    {
                        if (Fabs(dist) < Fabs(nn)) 
                        {
                            pa.Scale(volA->mLength, Tn);
                            pa.ScaleAdd(volA->mCylinderRadius, Tt);
                            pa.Add(volA->mPosition);
                            pb.ScaleAdd(pa, dist, Tt);
                            Tt.Scale(-1.0f);//Outward normal at element B.
                            AddCollision(volA, pa, volB, pb, dist, Tt);
                            //return;
                        }
                    }
                }
            }
        }
        
        //Cylinder's face versus Cylinder's end.
        CyliderFaceEnd( volA, volB );
        
        //Cylinder's end versus Cylinder's end. 
        CylCylEnd(volA, volB);//All possible end side type combinations will be taken care.
        
    //}
    //else if (!CylCylSepPlane(volA, volB))
    //{
    //    DetectOnSubVolume(volA, volB);
    //}
}

void CollisionDetector::DetectCSColl(CylinderVolume* volA, CollisionVolume* volB)
{
    Vector N;
    
    //if (!volA->HasSubVolume() && !volB->HasSubVolume())
    //{
        Vector pb, pa;
        if (CylSphereColl(volA, volB->mPosition, volB->mSphereRadius, N, mNt, &pa, &pb))
        {
            AddCollision(volA, pa, volB, pb, mNt, N);
        }
    //}
    //else if (CylSphereColl(volA, volB->mPosition, volB->mSphereRadius, N, mNt))
    //{
    //    DetectOnSubVolume(volA, volB);
    //}
}

void CollisionDetector::DetectSSColl(CollisionVolume* volA, CollisionVolume* volB)
{
    //if (!volA->HasSubVolume() && !volB->HasSubVolume())
    //{
        Vector pa, pb;
        
        mNt = Sqrt(mNt);
        mT.Scale(1.0f/mNt);
        pa.ScaleAdd(volA->mPosition, -volA->mSphereRadius, mT);
        pb.ScaleAdd(volB->mPosition, volB->mSphereRadius, mT);
        
        AddCollision(volA, pa, volB, pb, mNt-(volA->mSphereRadius+volB->mSphereRadius), mT);
    //}
    //else
    //{
    //    DetectOnSubVolume(volA, volB);
    //}
}

//
//
//

bool CollisionDetector::CollTestInit(CollisionVolume* volA, CollisionVolume* volB)
{
    if (volA->Type() != WallVolumeType && volA->Type() != BBoxVolumeType)
    {
        mT.Sub(volA->mPosition, volB->mPosition);
        mNt = mT.DotProduct(mT);
        
        // mNt == 0? can't resolve that collision, assert in debug or return in release
        // don't have to assert but its better cause it potentially indicates ai problem 
        // where objects are not initialized properly
        //rAssert(mNt > 0); 
        if (mNt < VERY_SMALL)
        {
            return false; 
        }
        else
        {
            return (mNt < Sqr(volA->mSphereRadius + volB->mSphereRadius + mCollisionDistance));
        }
    }
    else 
    {
        return true;
    }
}

void CollisionDetector::SetMaxSafeTimeBeforeCollision(float inTime)       
{ 
    rAssert(inTime >= 0); 
    sSafeTimeBeforeCollision = inTime;
}

} // sim
