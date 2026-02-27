
#include "simcollision/subcollisiondetector.hpp"
#include "simcollision/collision.hpp"
#include "simcommon/simmath.hpp"

using namespace RadicalMathLibrary;

namespace sim
{


// testing stuff
//#define TEST_VECTOR
//#define TEST_COLL


SubCollisionDetector::SubCollisionDetector()
    : 
    mCollisionDistance(0),
    mMinCollisionDistance(0),
    mSphereApproxA(false),
    mSphereApproxB(false),
    mCollisionList(NULL)

{
    SetCollisionFilter(FILTER_BY_POSITION | FILTER_BY_DEPTH);
    //SetCollisionFilter(FILTER_NO_FILTERING);
    //SetCollisionFilter(FILTER_BY_POSITION);
}

SubCollisionDetector::~SubCollisionDetector()
{
}

// test for collision between the faces of volA with the Corner of volB 
void SubCollisionDetector::OBBoxV(OBBoxVolume* volA, OBBoxVolume* volB)
{
    Vector p, pa;
    float np[4][3], nt[3];
    float sgn, dist, dist2, dist3;
    float dst[4][3];
    int i, j;
    
    mT.Sub(volB->mPosition, volA->mPosition);
    
    // since these dot products are used at least 4 times, they are temporarilly stored
    if (volA->mAxisOrientation == VolAxisNotOriented)
    {
        for (i=0; i<3; i++)
        {
            nt[i] = mT.DotProduct(volA->mAxis[i]);
        }
        for (j=0; j<4; j++)
        {
            volB->Corner(p, j);
            for (int i=0; i<3; i++)
            {
                np[j][i] = p.DotProduct(volA->mAxis[i]);
            
                // dst is the penetration distance of the vertex j in the face i
                // the Fabs(np) allows to test for two opposite corner at the same time
                // the Fabs(nt) allow to test for two faces at the same time
                dst[j][i] = Fabs(nt[i]) - (Fabs(np[j][i]) + volA->mLength[i]);
            }
        }
    }
    else
    {
        (*(rmt::Vector*)nt) = mT;

        for (j=0; j<4; j++)
        {
            volB->Corner((*(rmt::Vector*)np[j]), j);

            for (int i=0; i<3; i++)
            {
                // dst is the penetration distance of the vertex j in the face i
                // the Fabs(np) allows to test for two opposite corner at the same time
                // the Fabs(nt) allow to test for two faces at the same time
                dst[j][i] = Fabs(nt[i]) - (Fabs(np[j][i]) + volA->mLength[i]);
            }
        }
    }
    
    int order[3];
    for (j=0; j<4; j++)
    {
        FindFaceIndexOrder(order, dst[j][0], dst[j][1], dst[j][2]);
        for (int ii=0; ii<3; ii++)
        {
            i = order[ii];
            dist = dst[j][i];
            sgn = ((-nt[i] * np[j][i]) < 0.f )? -1.f : 1.f;
            volB->Corner(p,j);
            p.ScaleAdd(mT,sgn,p); //from center of A to corner.
            float pDotAxis = p.DotProduct(volA->mAxis[i]);

            // to avoid false collision on large thin boxes
            if (dist < 0.0f && Fabs(pDotAxis) > volA->mLength[i])
                continue;

            if (dist <= mCollisionDistance && (dist > -2*volA->mLength[i] || nt[i]*pDotAxis<0.0f) )
            {
                
                // make sure the vertex cross the face plane inside the face
                int ni = NXTI(i);
                dist2 = Fabs(nt[ni] + (sgn * np[j][ni])) - volA->mLength[ni];

                // mCollisionDistance allow to catch collision between a vertex and an edge or another vertex
                // if dist < 0, interpenetration, do not need the border collision
                if ( dist2 <= (dist > 0 ? mMinCollisionDistance : 0))
                {
                    ni = NXTI(ni);
                    dist3 = Fabs(nt[ni] + (sgn * np[j][ni])) - volA->mLength[ni];
                    if ( dist3 <= (dist > 0 ? mMinCollisionDistance : 0))
                    {
                        float sgn2 = (nt[i] < 0.f)? -1.f : 1.f;
                        // we have a collision...
                        volB->Corner(p, j);                  // mPosition +/- p correspond to two opposite corner of the OBBox
                        p.ScaleAdd(volB->mPosition, sgn, p); // use sgn to get to good 1.0f
                        pa.ScaleAdd(p, -sgn2 * dist, volA->mAxis[i]); // position on the face
                        
                        Vector N;
                        if (    mCollisionDistance == mMinCollisionDistance &&
                                (dist > 0 && (dist2 > 0 || dist3 > 0) ) && 
                                (Sqr(dist)+Sqr(dist2>0?dist2:0)+Sqr(dist3>0?dist3:0)) > MILLI_EPS)
                        { 
                            // we have a border collision, pa is then a little outside the face and needs to be adjusted
                            OBBoxBorderCollMove(volA, pa, dist2, dist3, i);
                            N.Sub(pa, p);
                            dist = N.NormalizeSafe();
                            rAssert(dist!=0.0f);
                        }
                        else
                        {
                            N.Scale(-sgn2, volA->mAxis[i]);
                        }
                        
                                             
                                            
                        /*                                             
                        const char* nameA = volA->GetCollisionObject()->GetName();
                        const char* nameB = volB->GetCollisionObject()->GetName();
                                            
                        if(strcmp(nameA, "otto_v") == 0 && strcmp(nameB, "cSedan") == 0)
                        {    
                            rDebugPrintf("\n@@@  OBBoxV called AddCollision");   
                        }
                        else if(strcmp(nameB, "otto_v") == 0 && strcmp(nameA, "cSedan") == 0)
                        {
                            rDebugPrintf("\n@@@  OBBoxV called AddCollision");   
                            
                        }
                        */
                                            
                        
                        
                        
                        
                        
                        AddCollision(volA, pa, volB, p, dist, N);
                        break; // a vertex can collide with only 1 face at a time
                    }
                }
            }
        }
    }
}

void SubCollisionDetector::OBBoxBorderCollMove(OBBoxVolume* vol, Vector& pos, float dist2, float dist3, int i)
{
    Vector tmp;
    float sgn;
    int ni = NXTI(i);
    
    tmp.Sub(pos, vol->mPosition);
    
    if (dist2 > 0)
    {
        sgn = -Sign(tmp.DotProduct(vol->mAxis[ni]));
        pos.ScaleAdd(sgn * dist2, vol->mAxis[ni]);
    }
    if (dist3 > 0) 
    {
        ni = NXTI(ni);
        sgn = -Sign(tmp.DotProduct(vol->mAxis[ni]));
        pos.ScaleAdd(sgn * dist3, vol->mAxis[ni]);
    }
}

void SubCollisionDetector::OBBoxEE(OBBoxVolume* volA, OBBoxVolume* volB)
{
    Vector N;
    float nt, nn;
    bool hasCollided = false;

    
    mT.Sub(volB->mPosition, volA->mPosition);
    
    for (int i=0; i<3; i++)
    {
        for (int j=0; j<3; j++)
        {
            N.CrossProduct(volA->mAxis[i], volB->mAxis[j]);
            N.Scale(volA->mLength[i] * volB->mLength[j]);
            
            nn = N.DotProduct(N);
            
            if (nn > VERY_SMALL) // else edges are parallel... the vertex-edges collision are handled throw OBBoxV 
            {
                nt = mT.DotProduct(N); 
                
                if (Fabs(nt) > VERY_SMALL) // else no collision possible between these edges
                {
                    nn = Sqrt(nn);
                    float invn = 1.0f/nn;
                    nt = nt * invn; // distance between box center along N, the collision normal
                    // there is 4 parallel edges in a cube, we do 2 at a time on each box
                    int ii[4];
                    Vector pa[4], pb[4];
                    float npa[4], npb[4], dist[4];
                    int kl=0;
                    for (int k=0; k<2; k++)
                    {
                        for (int l=0; l<2; l++, kl++)
                        {
                            volA->Side(pa[kl], i, k);
                            volB->Side(pb[kl], j, l);
                            npa[kl] = pa[kl].DotProduct(N) * invn;
                            npb[kl] = pb[kl].DotProduct(N) * invn;
                            
                            dist[kl] = Fabs(nt) - (Fabs(npa[kl]) + Fabs(npb[kl]));
                        }
                    }
                    FindSizeOrder4( ii, dist );

                    for ( kl = 3 ; kl>=0 ; kl-- )
                    {
                        int ind = ii[kl];

                        float mindist = Min(Fabs(npa[ind]), Fabs(npb[ind]));
                        
                        // Strict test
                        if (Fabs(npa[ind]) > VERY_SMALL && Fabs(npb[ind]) > VERY_SMALL && dist[ind] < mCollisionDistance && dist[ind] > -mindist) // else, no collision possible between these axes
                        {
                            if (nt * npa[ind] < 0) 
                                pa[ind].Scale(-1.0f);  // identify the correct side on a
                            if (-nt * npb[ind] < 0) 
                                pb[ind].Scale(-1.0f);  // identify the correct side on b
                            
                            EdgeEdge(hasCollided, pa[ind], volA->mAxis[i], volA->mLength[i], pb[ind], volB->mAxis[j], volB->mLength[j], N, nn, dist[ind], volA, volB);
                            if (mNewColl>3)
                                break;
                        }
                    }
                }
            }
        }
    }


    // only do lax test if we're a car (1) hitting a redBrickPhizMoveable (131072) or a StateProp (262146)
    if (!hasCollided && 
        (   (volA->GetCollisionObject()->GetSimState()->mAIRefIndex == 1 && (volB->GetCollisionObject()->GetSimState()->mAIRefIndex == 131072 || 
                                                                             volB->GetCollisionObject()->GetSimState()->mAIRefIndex == 262146))
            ||
            (volB->GetCollisionObject()->GetSimState()->mAIRefIndex == 1 && (volA->GetCollisionObject()->GetSimState()->mAIRefIndex == 131072 || 
                                                                             volA->GetCollisionObject()->GetSimState()->mAIRefIndex == 262146))     ))            
                                                                            
    
    {
        for (int i=0; i<3; i++)
        {
            for (int j=0; j<3; j++)
            {
                N.CrossProduct(volA->mAxis[i], volB->mAxis[j]);
                N.Scale(volA->mLength[i] * volB->mLength[j]);
                
                nn = N.DotProduct(N);
                
                if (nn > VERY_SMALL) // else edges are parallel... the vertex-edges collision are handled throw OBBoxV 
                {
                    nt = mT.DotProduct(N); 
                    
                    if (Fabs(nt) > VERY_SMALL) // else no collision possible between these edges
                    {
                        nn = Sqrt(nn);
                        float invn = 1.0f/nn;
                        nt = nt * invn; // distance between box center along N, the collision normal
                        // there is 4 parallel edges in a cube, we do 2 at a time on each box
                        int ii[4];
                        Vector pa[4], pb[4];
                        float npa[4], npb[4], dist[4];
                        int kl=0;
                        for (int k=0; k<2; k++)
                        {
                            for (int l=0; l<2; l++, kl++)
                            {
                                volA->Side(pa[kl], i, k);
                                volB->Side(pb[kl], j, l);
                                npa[kl] = pa[kl].DotProduct(N) * invn;
                                npb[kl] = pb[kl].DotProduct(N) * invn;
                                
                                dist[kl] = Fabs(nt) - (Fabs(npa[kl]) + Fabs(npb[kl]));
                            }
                        }
                        FindSizeOrder4( ii, dist );

                        for ( kl = 3 ; kl>=0 ; kl-- )
                        {
                            int ind = ii[kl];

                            float mindist = Min(Fabs(npa[ind]), Fabs(npb[ind]));
                            
                            // Lax test
                            if (Fabs(npa[ind]) > VERY_SMALL && Fabs(npb[ind]) > VERY_SMALL && dist[ind] < mCollisionDistance) // else, no collision possible between these axes
                            {
                                if (nt * npa[ind] < 0) 
                                    pa[ind].Scale(-1.0f);  // identify the correct side on a
                                if (-nt * npb[ind] < 0) 
                                    pb[ind].Scale(-1.0f);  // identify the correct side on b
                                
                                EdgeEdge(hasCollided, pa[ind], volA->mAxis[i], volA->mLength[i], pb[ind], volB->mAxis[j], volB->mLength[j], N, nn, dist[ind], volA, volB);
                                if (mNewColl>3)
                                    break;
                            }
                        }
                    }
                }
            }
        }
    }
}

// OBBox interference test, adaptation from Gottschaclk, Lin & Manocha, "OBBTree..." Siggraph 96
bool SubCollisionDetector::OBBoxSepPlane(OBBoxVolume* volA, OBBoxVolume* volB)
{
    // go quickly throw the 15 tests to find separation plane starting with the vertex-face cause its faster
    
    float projection;
    
    // 3 tests with faces of 'a' and 3 tests with faces of 'b'
    int i;
    for (i=0; i<3; i++)
    {
        projection = volB->Projection(volA->mAxis[i]) + volA->mLength[i];
        if (Fabs(mT.DotProduct(volA->mAxis[i])) > mCollisionDistance + projection)
            return true;
        projection = volA->Projection(volB->mAxis[i]) + volB->mLength[i];
        if (Fabs(mT.DotProduct(volB->mAxis[i])) > mCollisionDistance + projection)
            return true;
    }
    
    // 9 pairs of edges to test
    Vector axe;
    
    for (i=0; i<3; i++)
    {
        for (int j=0; j<3; j++)
        {
            axe.CrossProduct(volA->mAxis[i], volB->mAxis[j]);
            if (mCollisionDistance > 0) 
                axe.Normalize();
            projection = volA->Projection(axe, i) + volB->Projection(axe, j); 
            if (Fabs(mT.DotProduct(axe)) > mCollisionDistance + projection)
                return true;
        }
    }
    
    return false;
}

// detect collision between the two given edges
bool SubCollisionDetector::EdgeEdge(bool &hasCollided, Vector& pa, Vector& oa, float la, Vector& pb, Vector& ob, float lb, Vector& N, float nn, 
                                    float dist, CollisionVolume* volA, CollisionVolume* volB)
{
    Vector dp;
    
    dp.Sub(pb, pa);
    dp.Add(mT); // vector between center of edge a and center of edge b
    
    float kb;
    Vector tmp;
    
    tmp.CrossProduct(dp, N);
    nn = 1.0f/(Sqr(nn));
    kb = la * tmp.DotProduct(oa) * nn;//This is the same thing as projecting dp along oax(oaxob)
    
    if (Fabs(kb) <= 1.0f) // or the edges projection on the perpendicular plane don't intersect
    {
        float ka;
        
        ka = lb * tmp.DotProduct(ob) * nn;//This is the same thing as projecting dp along obx(oaxob)
        if (Fabs(ka) <= 1.0f) // or the edges projection on the perpendicular plane don't intersect
        {
            // should have a collision
            if (volA != NULL && volB != NULL)
            {
                pa.Add(volA->mPosition);
                pb.Add(volB->mPosition);
                pa.ScaleAdd(-ka * la, oa);
                pb.ScaleAdd(-kb * lb, ob);
                
                dp.Sub(pa, pb);
                float dd = dp.NormalizeSafe();
                
                if (dd < VERY_SMALL)
                {
                    // can't compute a normal... would be better to compute a new normal
                    return false;
                }

                if (dist < 0) // make sure N is correctly oriented
                {
                    dd = -dd;
                    dp.Scale(-1.0f);
                }
                AddCollision(volA, pa, volB, pb, dd, dp); 
        
                
        
                /*  
                const char* nameA = volA->GetCollisionObject()->GetName();
                const char* nameB = volB->GetCollisionObject()->GetName();
                                    
                if(strcmp(nameA, "otto_v") == 0 && strcmp(nameB, "cSedan") == 0)
                {    
                    rDebugPrintf("\n@@@  EdgeEdge called AddCollision");   
                }
                else if(strcmp(nameB, "otto_v") == 0 && strcmp(nameA, "cSedan") == 0)
                {
                    rDebugPrintf("\n@@@  EdgeEdge called AddCollision");   
                    
                }
                */
                            
                
                
                hasCollided = true;
                
#ifdef TEST_VECTOR
                dp.Sub(pa, pb);
                ka = Fabs(dp.DotProduct(oa));
                kb = Fabs(dp.DotProduct(ob));
                if (ka > VERY_SMALL || kb > VERY_SMALL)
                {
                    kb=0; // fuck 
                }
#endif // TEST_VECTOR
            }
            return true;
        }
    }
    return false;
}

bool SubCollisionDetector::PointBoxColl
    (
        OBBoxVolume *vol, 
        Vector& pp,   //input: position of the point. output: contact's posisition on the sphere.
        Vector* pa,   //contact's position on the box. May be Null if getpos is false.
        Vector* n,    //contact's Normal on output. May be Null if getpos is false.
        float &dist, 
        float radius, 
        bool getpos)
{
    rAssert( !getpos || (pa && n) );
    Vector dp;
    float ndp[3];
    float borderDist = radius + mMinCollisionDistance;
    float dst[3];
    
    dp.Sub(pp, vol->mPosition);
    int i;
    
    if (vol->mAxisOrientation == VolAxisNotOriented)
    {
        for (i=0; i<3; i++)
        {
            ndp[i] = dp.DotProduct(vol->mAxis[i]);
            dst[i] = Fabs(ndp[i]) - radius - vol->mLength[i];
        }
    }
    else
    {
        (*(rmt::Vector*)ndp) = dp;
        dst[0] = Fabs(ndp[0]) - radius - vol->mLength[0];
        dst[1] = Fabs(ndp[1]) - radius - vol->mLength[1];
        dst[2] = Fabs(ndp[2]) - radius - vol->mLength[2];
    }
    
    int order[3];
    FindFaceIndexOrder(order, dst[0], dst[1], dst[2]);
    
    float mindist = Min(radius, vol->mLength[order[0]]);
    
    if (dst[order[0]] < mCollisionDistance && dst[order[0]] < -mindist)
    {
        // the sphere is completely inside, return the collision toward the closest face
        if (getpos)
        {
            i = order[0];
            *pa = pp;
            n->Scale(-Sign(ndp[i]),vol->mAxis[i]);
            pa->ScaleAdd( (dst[i]+radius), *n); // position on the face
            pp.ScaleAdd ( radius, *n); // position on the sphere
            dist = dst[i];
        }
        return true;
    }
    
    for (int ii=0; ii<3; ii++)
    {
        // the Fabs(ndp) allows to test for the two opposite face of the cube at the same time
        
        i = order[ii];
        dist = dst[i];
        mindist = Min(radius, vol->mLength[i]);
        
        if (dist < mCollisionDistance && dist > -mindist)
        {
            // make sure the vertex cross the face plane inside the face + its border
            int ni = NXTI(i);
            float d2 = Max(0.0f, Fabs(ndp[ni]) - vol->mLength[ni]);
            if (d2 <= borderDist)
            {
                ni = NXTI(ni);
                float d3 = Max(0.0f, (Fabs(ndp[ni]) - vol->mLength[ni]));
                if (d3 <= borderDist)
                {
                    if (d2 > 0 || d3 > 0)
                    {
                        dist = Sqrt(Sqr(dist+radius) + Sqr(d2) + Sqr(d3)) - radius;
                    }
                    if (dist < mCollisionDistance)
                    {
                        if (getpos)
                        {
                            float sign = -Sign(ndp[i]);
                            *pa = pp;
                            pa->ScaleAdd( sign * (dst[i]+radius), vol->mAxis[i]); // position on the face
                            if ( (d2 > 0 || d3 > 0 ) && (dist + radius) > MILLI_EPS )
                            {
                                OBBoxBorderCollMove(vol, *pa, d2, d3, i);
                                n->Sub(*pa, pp);
                                n->NormalizeSafe();
                                pp.ScaleAdd( radius, *n );
                            }
                            else
                            {
                                n->Scale( sign, vol->mAxis[i]);
                                pp.ScaleAdd (radius, *n); // position on the face
                            }
                        }
                        return true;
                    }
                }
            }
        }
    }
    
    return false;
}

void SubCollisionDetector::FindFaceIndexOrder(int* i, float d0, float d1, float d2) const
{
    if (d0 == d1 && d0 == d2)
    {
        i[0] = 0; i[1] = 1; i[2] = 2;
    }
    else
    {
        i[0] = (d0>=d1 && d0>=d2 ? 0 : (d1>=d2 && d1>=d0 ? 1 : 2));
        i[2] = (d0<d1 && d0<d2 ? 0 : (d1<d2 && d1<d0 ? 1 : 2));
        i[1] = (i[0] != 0 && i[2] != 0 ? 0 : (i[0] != 1 && i[2] != 1 ? 1 : 2));
    }
}

void SubCollisionDetector::FindSizeOrder4(int i[], float d[]) const
{
    if ( d[0] >= d[1] && d[0] >= d[2] && d[0] >= d[3] )
    {
        i[0] = 0;
        FindFaceIndexOrder( i+1, d[1], d[2], d[3] );
        i[1] += 1;
        i[2] += 1;
        i[3] += 1;
    }
    else if (d[1] >= d[0] && d[1] >= d[2] && d[1] >= d[3] )
    {
        i[0] = 1;
        FindFaceIndexOrder( i+1, d[0], d[2], d[3] );
        if(i[1]>0) i[1] += 1;
        if(i[2]>0) i[2] += 1;
        if(i[3]>0) i[3] += 1;
    }
    else if (d[2] >= d[0] && d[2] >= d[1] && d[2] >= d[3] )
    {
        i[0] = 2;
        FindFaceIndexOrder( i+1, d[0], d[1], d[3] );
        if(i[1]>1) i[1] += 1;
        if(i[2]>1) i[2] += 1;
        if(i[3]>1) i[3] += 1;
    }
    else
    {
        i[0] = 3;
        FindFaceIndexOrder( i+1, d[0], d[1], d[2] );
        if(i[1]>2) i[1] += 1;
        if(i[2]>2) i[2] += 1;
        if(i[3]>2) i[3] += 1;
    }
}

// test for collision between the faces of a box with the cylinder 
void SubCollisionDetector::OBBoxCylF(OBBoxVolume* volA, CylinderVolume* volB)
{
    float nt[3];
    
    int i;
    if (volA->mAxisOrientation == VolAxisNotOriented)
    {
        for (i=0; i<3; i++)
        {
            nt[i] = mT.DotProduct(volA->mAxis[i]);
        }
    }
    else
    {
        (*(rmt::Vector*)nt) = mT;
    }

    for (i=0; i<3; i++)
    {
        float dist = Fabs(nt[i]) - volA->mLength[i];
        
        if (dist > 0 && dist < volB->mSphereRadius + mCollisionDistance) // else, no collision possible on these faces or we're deep into it
        {
            bool completed = false;
            bool firstSide = true;

            float sgn = Sign(nt[i]);
            float sgna = sgn;
            float dotAxis = volB->mAxis.DotProduct(volA->mAxis[i]);

            while(!completed)
            {
                Vector pb, tmp1, tmp2;
                pb.Scale(volB->mLength * sgna * Sign(dotAxis), volB->mAxis); // vector from mPosition to the center of the circle
                tmp1.CrossProduct(volA->mAxis[i], volB->mAxis);
                tmp2.CrossProduct(volB->mAxis, tmp1);
                tmp2.Normalize();
                pb.ScaleAdd(sgn * volB->mCylinderRadius, tmp2); // the point on the cyl
            
                float dist2 = Fabs(pb.DotProduct(volA->mAxis[i]));
                dist -= dist2;
            
                if (dist <= mCollisionDistance)// && dist > -volB->mCylinderRadius)
                {
                    // make sure the vertex cross the face plane inside the face
                    int ni = NXTI(i);
                    dist2 = Fabs(nt[ni] - pb.DotProduct(volA->mAxis[ni])) - volA->mLength[ni];
                
                    if ( dist2 <= 0)
                    {
                        ni = NXTI(ni);
                        float dist3 = Fabs(nt[ni] - pb.DotProduct(volA->mAxis[ni])) - volA->mLength[ni];
                        if ( dist3 <= 0)
                        {
                            // we have a collision...
                            pb.Add(volB->mPosition);

                            Vector pa;
                            pa.Scale(sgn * dist, volA->mAxis[i]);
                            pa.Add(pb); // position on the face
                        
                            Vector N;
                            N.Scale(sgn, volA->mAxis[i]);
                            AddCollision(volA, pa, volB, pb, dist, N);
                        }
                    }
                }
                // this will make the detector to look for a second collision on the same face 
                // with the other extremity of the cylinder. 2 collision might be added if the
                // cylinder if flat on the face. the goal is to make the sim more stable and to
                // get to rest faster
                if (firstSide && dist + 2.0f*volB->mLength*Fabs(dotAxis) <= mCollisionDistance)
                {
                    sgna = -sgna;
                    dist = Fabs(nt[i]) - volA->mLength[i];
                    firstSide = false;
                }
                else
                {
                    completed = true;
                }
            }
        }
    }
}

// collision of the box vertices with the cylinder
void SubCollisionDetector::OBBoxCylV(OBBoxVolume* volA, CylinderVolume* volB)
{
    Vector pa, pb, dp;
    float dist, dist2, dist3;
    float rad = (volB->mFlatEnd  ? 0 : volB->mCylinderRadius);
    bool collide = false;
    
    for (int j=0; j<4; j++)
    {
        volA->Corner(pa, j);
        dist = pa.DotProduct(mT);
        if (Fabs(dist) > VERY_SMALL) 
        {
            if (dist > 0)
                pa.Scale(-1.0f);
            
            dp.Add(mT, pa); // mT = volA->mPosition - volB->mPosition => dp is the vector from the corner to volB->mPosition
            
            dist2 = dp.DotProduct(volB->mAxis);
            dist3 = Fabs(dist2) - volB->mLength;
            
            if (dist3 - rad <= mCollisionDistance) // longitudinal distance
            {
                mNormal.Scale(-dist2, volB->mAxis);
                mNormal.Add(dp); // from the cyl axis to pa
                dist = mNormal.Magnitude();
                dist -= volB->mCylinderRadius; //dist = Fabs(mNormal.DotProduct(dp)) - volB->mCylinderRadius;
                
                if (dist <= mCollisionDistance) // radial distance
                {
                    if (!volB->mFlatEnd && Fabs(dist2) >= volB->mLength)
                    {
                        //In the half-sphere.
                        mNormal.Scale(-Sign(dist2) * volB->mLength, volB->mAxis);
                        mNormal.Add(dp); // from pa to center of sphere
                        dist = mNormal.NormalizeSafe() - volB->mCylinderRadius;
                        if (dist <= mCollisionDistance)
                        {
                            collide = true;
                            pa.Add(volA->mPosition);
                            pb.Scale(-dist, mNormal);
                            pb.Add(pa);
                        }
                    }
                    else
                    {
                        pa.Add(volA->mPosition); // absolute position of box corner
                        if( volB->mFlatEnd  && dist3 > -mCollisionDistance ) // collision with the cyl end
                        {
                            collide = true; // we have a collision...
                            mNormal.Scale( Sign(dist2), volB->mAxis );
                            pb.ScaleAdd( pa, -dist3, mNormal);
                            dist = dist3;
                        }
                        else  if (dist != -volB->mCylinderRadius) // else the vertex is exactly on the axis, don't know the collision orientation, could use the speed...
                        {
                            collide = true; // we have a collision...
                            // collision with the side
                            mNormal.Scale(-1.0f/(dist+volB->mCylinderRadius)); // dist+volB->mCylinderRadius is the norme of mNormal
                            pb.Scale(dist, mNormal);
                            pb.Add(pa);
                            if (dist3 > 0)
                            {  
                                // collision with the circle, pb must be moved 
                                pb.ScaleAdd(-Sign(dist2) * dist3, volB->mAxis);
                                mNormal.Sub(pa, pb);
                                dist = mNormal.NormalizeSafe();
                            }
                            else
                                mNormal.Scale(-1.0f);
                        }
                    }
                    
                    if (collide)
                    {
                        AddCollision(volA, pa, volB, pb, dist, mNormal);
                        //break; // a vertex can collide with only 1.0f face at a time
                        collide = false;
                    }
                }
            }
        }
    }
}

void SubCollisionDetector::OBBoxCylEE(OBBoxVolume* volA, CylinderVolume* volB)
{
    Vector pa, pb, N;
    float npa, npb, nt, nn;
    float dist;
    
    mT.Sub(volB->mPosition, volA->mPosition);
    
    bool hasCollided = false;
    
    for (int i=0; i<3; i++)
    {
        N.CrossProduct(volA->mAxis[i], volB->mAxis);
        N.Scale(volA->mLength[i] * volB->mLength);
        
        nn = N.DotProduct(N);
        
        if (nn > VERY_SMALL) // else edges are parallel... the vertex-edges collision are handled throw OBBoxV 
        {
            nt = mT.DotProduct(N); 
            
            if (Fabs(nt) > VERY_SMALL) // else no collision possible between these edges
            {
                nn = Sqrt(nn);
                float invn = 1.0f/(nn);
                nt = nt * invn; // distance between box center and cyl axis along N, the collision normal
                
                //spb.Scale(volB->mCylinderRadius/nn, N);
                npb = volB->mCylinderRadius;
                // there is 4 parallel edges in a cube, we do 2 at a time on the box
                for (int k=0; k<2; k++)
                {
                    volA->Side(pa, i, k);
                    npa = pa.DotProduct(N) * invn;
                    
                    dist = Fabs(nt) - (Fabs(npa) + npb);
                    float mindist = Min(Fabs(npa), volB->mCylinderRadius);
                    
                    
                    // greg, june 16 2003
                    // after nearly a week of debugging, with Stan's help, I get to comment out the last bit of this test so that if a capsule is wholly inside the box
                             
                    // strict test         
                    if (Fabs(npa) > VERY_SMALL /*&& Fabs(npb) > VERY_SMALL*/ && dist < mCollisionDistance  && dist > -mindist) // else, no collision possible between these axes
                    {
                        if (nt * npa < 0) 
                            pa.Scale(-1.0f);  // identify the correct side on a
                        pb.Scale((nt > 0 ? -volB->mCylinderRadius * invn : volB->mCylinderRadius * invn), N);   // identify the correct side on b

                        EdgeEdge(hasCollided, pa, volA->mAxis[i], volA->mLength[i], pb, volB->mAxis, volB->mLength, N, nn, dist, volA, volB);
                    }
                }
            }
        }
    }
    
    // sweet magic numbers :)
    //if(!hasCollided && !volB->mCharacter)                                       // player character                                                   // npc character
    if(!hasCollided && 
    
    
        // we're not a character
        (
            ( !( (volB->GetCollisionObject()->GetSimState()->mAIRefIndex == 131073) || (volB->GetCollisionObject()->GetSimState()->mAIRefIndex == 131074) ) )
        
            ||
            
            // we're a vehicle moving at non-trivial speed
            (volA->GetCollisionObject()->GetSimState()->mAIRefIndex == 1 && (volA->GetCollisionObject()->GetSimState()->GetLinearVelocity()).MagnitudeSqr() > 25.0f)
        )
    
    )
    
    
    
    {
        // redo test with lax test
                    
        for (int i=0; i<3; i++)
        {
            N.CrossProduct(volA->mAxis[i], volB->mAxis);
            N.Scale(volA->mLength[i] * volB->mLength);
            
            nn = N.DotProduct(N);
            
            if (nn > VERY_SMALL) // else edges are parallel... the vertex-edges collision are handled throw OBBoxV 
            {
                nt = mT.DotProduct(N); 
                
                if (Fabs(nt) > VERY_SMALL) // else no collision possible between these edges
                {
                    nn = Sqrt(nn);
                    float invn = 1.0f/(nn);
                    nt = nt * invn; // distance between box center and cyl axis along N, the collision normal
                    
                    //spb.Scale(volB->mCylinderRadius/nn, N);
                    npb = volB->mCylinderRadius;
                    // there is 4 parallel edges in a cube, we do 2 at a time on the box
                    for (int k=0; k<2; k++)
                    {
                        volA->Side(pa, i, k);
                        npa = pa.DotProduct(N) * invn;
                        
                        dist = Fabs(nt) - (Fabs(npa) + npb);
                        float mindist = Min(Fabs(npa), volB->mCylinderRadius);
                        
                                 
                        // lax test         
                        if (Fabs(npa) > VERY_SMALL /*&& Fabs(npb) > VERY_SMALL*/ && dist < mCollisionDistance)//  && dist > -mindist) // else, no collision possible between these axes
                        {
                            if (nt * npa < 0) 
                                pa.Scale(-1.0f);  // identify the correct side on a
                            pb.Scale((nt > 0 ? -volB->mCylinderRadius * invn : volB->mCylinderRadius * invn), N);   // identify the correct side on b

                            EdgeEdge(hasCollided, pa, volA->mAxis[i], volA->mLength[i], pb, volB->mAxis, volB->mLength, N, nn, dist, volA, volB);
                        }
                    }
                }
            }
        }       
    
    }
    
    
    
    
    
}


void SubCollisionDetector::OBBoxCylEC(OBBoxVolume* volA, CylinderVolume* volB)
{
    float nft = mT.DotProduct(volB->mAxis);
    // if (Fabs(nft) < volB->mLength) return;
    
    Vector pa;
    float no;
    
    for (int i=0; i<3; i++)
    {
        no = volA->mAxis[i].DotProduct(volB->mAxis) * volA->mLength[i];
        for (int k=0; k<2; k++)
        {
            volA->Side(pa, i, k);
            CircleEdge(volA, pa, volA->mAxis[i], volA->mLength[i], volB, nft, no, false );
        }
    }
}

bool SubCollisionDetector::OBBoxCylSepPlane(OBBoxVolume* volA, CylinderVolume* volB)
{
    if (!volB->mFlatEnd )
    {
        Vector pb;
        float dist;
        pb.Scale(volB->mLength, volB->mAxis);
        pb.Add(volB->mPosition);
        
        if (PointBoxColl(volA, pb, NULL, NULL, dist, volB->mCylinderRadius, false)) 
            return false;
        
        pb.Scale(-volB->mLength, volB->mAxis);
        pb.Add(volB->mPosition);
        
        if (PointBoxColl(volA, pb, NULL, NULL, dist, volB->mCylinderRadius, false)) 
            return false;
    }
    
    float projection, p;
    
    // as we know at that point that there is no coll with the sphere (if mFlatEnd ), 
    // we don't need to include volB->mCylinderRadius in the projection
    p = volA->Projection(volB->mAxis) + volB->mLength;
    
    if (Fabs(mT.DotProduct(volB->mAxis)) > mCollisionDistance + p)
        return true;
    
    // 3 tests with faces of 'a' 
    float a;
    int i;
    for (i=0; i<3; i++)
    {
        a = Fabs(volA->mAxis[i].DotProduct(volB->mAxis));
        projection = a * volB->mLength + Sqrt(1.0f-Sqr(a)) * volB->mCylinderRadius + volA->mLength[i];
        if (Fabs(mT.DotProduct(volA->mAxis[i])) > mCollisionDistance + projection)
            return true;
    }
    
    // 3 pairs of edges to test
    Vector axe;
    
    for (i=0; i<3; i++)
    {
        axe.CrossProduct(volA->mAxis[i], volB->mAxis);
        if (mCollisionDistance > 0) axe.Normalize();
        projection = volA->Projection(axe, i) + volB->mCylinderRadius; 
        if (Fabs(mT.DotProduct(axe)) > mCollisionDistance + projection)
            return true;
    }
    
    return false;
}

void SubCollisionDetector::CircleEdge(CollisionVolume* volA, Vector& pa, Vector& oa, float la, CylinderVolume* volB, float nft, float no, bool use_all_inside )
{ 
    
    if( la <= MICRO_EPS ) //edge length is too small. No edge circle collision.
        return;
    
    if (mT.DotProduct(pa) > 0)
        pa.Scale(-1.0f);     // Choose the appropriate side.
    
    Vector   pb;            // From center of cyl to center of edge.
    bool     all_inside;    // Edge projected on mAxis is shorter than mLength.
    float    tc, interp;
    float    fOoa = Fabs(volB->mAxis.DotProduct( oa ));
    float    npa = pa.DotProduct(volB->mAxis);
    
    
    // A segment is seen as a degenerated obox with two sides having zero length.
    // mT + pa represent the vector position of the obox relative to the cylinder B.
    // First criterial to have intersection is to see if the axis of the cylinder is
    // a seperating axis. no is the radius of the obox with respect the cylinder axis.
    // volB->mLength is the radius of the cylinder with respect to the cylinder axis.
    // interp is the difference between the projected distance of the two objects on
    // the separating axis and the sum of the radius.
    
    tc = Fabs( nft + npa ); // Projected distance.
    interp = tc - ( volB->mLength + Fabs(no) );
    if( interp <= mCollisionDistance )
    {
        // If all_inside == true, the projected edge on the cylinder's axis is all inside the cylinder.
        all_inside = tc + Fabs(no) < volB->mLength;
        
        if( fOoa >= 1.0f-MILLI_EPS && !all_inside )
        {  // the edge is parallel to the circle's normal.
            pb.Add(pa, mT); 
            no = pb.DotProduct(volB->mAxis);
            float dn = Fabs(no) - la - volB->mLength;
            if ( dn <= mCollisionDistance )
            {
                float depth;
                float nr = pb.DotProduct(pb) - Sqr(no);
                if( nr <= Sqr(volB->mCylinderRadius + mCollisionDistance) )
                {  // collision
                    if( nr >= MICRO_EPS && nr < Sqr(dn) )
                    {
                        // Chose normal along a radius
                        mNormal.ScaleAdd( pb, -no, volB->mAxis );
                        float sign = Sign(mNormal.DotProduct(pa));
                        pa.ScaleAdd( mNormal, Sign(no)*volB->mLength, volB->mAxis );
                        pa.Add( volB->mPosition );
                        depth = mNormal.NormalizeSafe();
                        if( sign < 0.0f || mNormal.DotProduct(mT) > 0.0f )
                        {
                            depth -= volB->mCylinderRadius;
                        }
                        else
                        {
                            depth = -(depth + volB->mCylinderRadius);
                            mNormal.Scale(-1.0f);
                        }
                        pb.ScaleAdd( pa, -depth, mNormal );                      
                    }
                    else
                    {
                        // The normal is along the axis of the cylinder
                        float s1 = Sign(no);
                        pa.ScaleAdd(pb, -Sign(pb.DotProduct(oa))*la, oa );
                        pa.Add( volB->mPosition );
                        pb.ScaleAdd( pa, -s1*dn, volB->mAxis );
                        mNormal.Scale( s1, volB->mAxis );
                        depth = dn;
                    }
                    AddCollision(volA, pa, volB, pb, depth, mNormal);
                }
            }
        }
        else if( fOoa/la <= MILLI_EPS )// The numerical precision make difficult to trap this case. Here, we devide by la but this is not correct.
        {
            // circle's normal perp to the edge
            float pbfo, dn;
            pb.Add(mT,pa); //Center's cylinder to edge middle point.
            pbfo = pb.DotProduct(volB->mAxis);
            dn   = Fabs(pbfo) - volB->mLength; //depth along the cylinder's normal.
            if( dn <= mCollisionDistance )
            {
                float  dr;
                mNormal.CrossProduct(volB->mAxis, oa);
                dr = Fabs(pb.DotProduct(mNormal));
                if( dr <= mCollisionDistance + volB->mCylinderRadius )
                {
                    //Does the segment overlaps the circle?
                    float pboa; 
                    float da = Sqr(volB->mCylinderRadius + mCollisionDistance) - Sqr(dr);
                    rAssert( da >= 0.0f );
                    if( da < 0 )
                        return;
                    da = Sqrt(da);
                    pboa = pb.DotProduct(oa);
                    float t1oa=pboa+la, t2oa=pboa-la, sign_t1t2 = Sign(t1oa*t2oa);
                    if( sign_t1t2 < 0 || Fabs(t1oa) <= da || Fabs(t2oa) <= da )
                    {
                        // Collision:
                        dr -= volB->mCylinderRadius; //depth along circle's radius.
                        if( (dr > dn || Fabs(dn/volB->mLength)>0.1f) && Fabs(da-volB->mCylinderRadius) >= MICRO_EPS ) //Rembering that dr and dn are negative.
                        {  //Use the Normal along a radius.
                            if( sign_t1t2 > 0 )
                            {
                                float s1 = Fabs(t1oa) < Fabs(t2oa) ? 1.0f : -1.0f ;
                                pa.ScaleAdd( pb, s1*la, oa );
                                pb.Scale( pbfo, volB->mAxis );
                                mNormal.Sub( pa, pb );
                                dr = mNormal.NormalizeSafe()-volB->mCylinderRadius;
                                pa.Add(volB->mPosition);
                                pb.Add( volB->mPosition );
                                pb.ScaleAdd( volB->mCylinderRadius, mNormal );
                            }
                            else
                            {
                                if( mNormal.DotProduct(pb) < 0.0f )
                                    mNormal.Scale(-1.0f);
                                pa.ScaleAdd( volB->mPosition, pbfo, volB->mAxis );
                                pa.ScaleAdd( dr + volB->mCylinderRadius, mNormal );
                                pb.ScaleAdd( volB->mPosition, Sign(pbfo)*volB->mLength, volB->mAxis );
                                pb.ScaleAdd( volB->mCylinderRadius, mNormal );
                            }
                            AddCollision( volA, pa, volB, pb, dr, mNormal );
                        }
                        else
                        {  //Use the Normal along a cylinder's axis.
                            if( Fabs(pboa) <= da )
                            {
                                // Center of edge is somewhere on the disk.
                                // Contact on edge is set at pb.
                                pa = pb;
                            }
                            else 
                            {
                                // Collision:
                                // Edge overlaps da. Contact on edge is set where circle intersects the edge.
                                float ds;
                                ds = da - sign_t1t2*Min( Fabs(t1oa), Fabs(t2oa) );
                                rAssert( (la-ds) > 0.0f );
                                pa.ScaleAdd(pb, -Sign(pboa)*(la-ds), oa );
                            }
                            pa.Add( volB->mPosition );
                            mNormal.Scale( Sign(pbfo), volB->mAxis );
                            pb.Scale( -dn, mNormal );
                            pb.Add(pa);
                            AddCollision( volA, pa, volB, pb, dn, mNormal );
                        }
                    }
                }
            }
        }
        else if ( all_inside && use_all_inside )
        {
            // This code should not be here because we can't intersect the circle in
            // this case. Edge can intersect the cylinder but this is suppose
            // to be cathed elsewhere. 
            Vector t1; //End points of the segment
            
            pb.Add(mT,pa); //Center's cylinder to edge middle point.
            
            //there is potencially two cases, one for each end points of the segment.
            Vector pbc; //Point of contact on volB.
            for( int i=0 ; i < 2 ; i++, la *=-1 )
            {
                float tn, tp, dn, dp, sign;
                t1.ScaleAdd( pb, la, oa );
                tn = t1.DotProduct( volB->mAxis );
                tp = t1.DotProduct(t1) - Sqr(tn);
                if( tp > 0.0f )
                    tp = Sqrt( tp );
                else
                    tp = 0.0f;
                dn = Fabs(tn) - volB->mLength;
                dp = tp - volB->mCylinderRadius;
                if( dp < mCollisionDistance )
                {
                    rAssert( dn <= mCollisionDistance ); //always true because of the "all inside" condition.
                    //collision
                    sign = Sign( tn );
                    pa.Add( volB->mPosition, t1 );
                    if( dn > dp )
                    {//Use the normal along the cylinder's axis.
                        mNormal = volB->mAxis;
                        mNormal.Scale( sign );
                        pbc.ScaleAdd( pa, sign*Fabs(dn), volB->mAxis );
                    }
                    else
                    {//Use the normal along a circle's radius.
                        mNormal.ScaleAdd( t1, -tn,  volB->mAxis );
                        mNormal.NormalizeSafe();
                        pbc.ScaleAdd( volB->mPosition, tn, volB->mAxis );
                        pbc.ScaleAdd( volB->mCylinderRadius, mNormal );
                        dn = dp;
                    }
                    AddCollision(volA, pa, volB, pbc, dn, mNormal);
                }
            }
        }
        else
        {
            float oafo, pbfo, sign_pbfo, sign_oafo, sign_pboa; 
            float a, c2, s2, p;
            
            pb.Add(mT,pa); //Center's cylinder to edge middle point.
            
            pbfo = pb.DotProduct( volB->mAxis );
            sign_pbfo = Sign(pbfo);
            oafo = oa.DotProduct( volB->mAxis );
            sign_pboa = Sign(pb.DotProduct(oa));
            sign_oafo = Sign( oafo );
            c2 = Sqr(oafo);
            s2 = 1 - c2 ;
            for( int i=0 ; i<2 ; i++ )
            {
                if( i==0 )
                {
                    a = volB->mLength - pbfo;
                }
                else
                {
                    a = volB->mLength + pbfo;
                }
                
                rAssert( c2 != 0.0f );   //Since edge is not perpendicular to cylinder's normal.
                if( c2 == 0.0f )
                    continue;
                p = Sqr(a)*( 1 + s2/c2 );
                if( p < Sqr(la) ) // This length must be smaller than the length of the edge.
                {
                    float sign;
                    sign = -sign_oafo*sign_pbfo;
                    if( Fabs(pbfo) < volB->mLength )
                    {
                        if( (sign_pbfo > 0 && i==0) || (sign_pbfo < 0 && i==1) )
                            sign *= -1;
                    }
                    pa.Scale( sign*Sqrt( p ), oa );
                    pa.Add( pb ); // from cyl's center to intersecting point on the disk.
                    
                    mNormal.ScaleAdd( pa, -Sign(pa.DotProduct(volB->mAxis))*volB->mLength, volB->mAxis );
                    float dr; //Radial depth
                    dr = mNormal.DotProduct(mNormal); 
                    if( dr < Sqr(volB->mCylinderRadius + mCollisionDistance) )
                    {  
                        //Collision
                        float depth;
                        dr = Sqrt(dr);
                        Vector l_pb;
                        if( c2 < .5f ) // Instead of chosing the normal type with depth comparison,
                        {              // use the following case when the edge is more parallel than perpendicular to the circle.
                            
                            //Use normal along mAxis.
                            //Here pa is the intersecting point between the cylinder and the edge. Let
                            //us calculating the one that is bellow the circle ( the infinite edge
                            //cross the cylinder at two points one is above the circle the other is
                            //bellow. We are only interesting in the bellow one ).
                            
                            Vector e1, e2;
                            float pbe1, sign_pbe1, pbe2;
                            e1.CrossProduct( oa, volB->mAxis );
                            e1.NormalizeSafe();
                            pbe1 = pb.DotProduct(e1);
                            sign_pbe1 = Sign( pbe1 );
                            if( sign_pbe1 < 0 )
                            {
                                e1.Scale(-1.0f);
                                e2.CrossProduct( e1, volB->mAxis );
                            }
                            else
                            {
                                e2.CrossProduct( volB->mAxis, e1 );
                            }
                            e2.NormalizeSafe();
                            
                            //At this poit e1, e2 anf mAxis is a right handed ortho unit frame. e1 is 
                            //perpendicular to the oa and e2 is parallel to oa.
                            
                            float oae2, s1, da;
                            oae2 = oa.DotProduct(e2); // Product oa.e2 guaranty to be positive and none zero.
                            
                            rAssert( Sqr(volB->mCylinderRadius + mCollisionDistance ) - Sqr(pbe1) > 0 );
                            if( Sqr(volB->mCylinderRadius + mCollisionDistance ) - Sqr(pbe1) <= 0.0f )
                                continue;
                            
                            //Half length of the projected segment on the disk.
                            da = Sqrt( Sqr(volB->mCylinderRadius + mCollisionDistance) - Sqr(pbe1) ); //Should not be zero.
                            
                            float sign_pbe2;
                            pbe2 = pb.DotProduct(e2);
                            sign_pbe2 = Sign(pbe2);
                            pbe2 = Fabs(pbe2);
                            s1 = ( pbe2 > da ) ? -1.0f : 1.0f;
                            
                            Vector t1, t2; //The two potential points on the edge which intersect the infinite cylinder surface.
                            float mpt1, mpt2;
                            
                            mpt2 = pbe2 - s1*da; // Projected length on circle
                            mpt1 = 2*da - Fabs(mpt2);
                            mpt1 /= oae2;// True length on edge
                            mpt2 /= oae2;
                            if( Fabs( mpt1 ) > la ) // Clamped length
                                mpt1 = Sign(mpt1)*la;
                            if( Fabs( mpt2 ) > la )
                                mpt2 = Sign(mpt2)*la;
                            
                            if( sign_pbe2*sign_pboa > 0 )
                            {
                                t1.ScaleAdd( pb, -sign_pboa*s1*mpt1, oa );
                                t2.ScaleAdd( pb, -sign_pboa*mpt2, oa );
                            }
                            else
                            {
                                t1.ScaleAdd( pb, sign_pboa*s1*mpt1, oa );
                                t2.ScaleAdd( pb, sign_pboa*mpt2, oa );
                            }
                            
                            float sign_pafo = Sign( pa.DotProduct( volB->mAxis ) );
                            mNormal.Scale( sign_pafo, volB->mAxis );
                            depth = sign_pafo*t1.DotProduct( volB->mAxis ) - volB->mLength;
                            if( depth < mCollisionDistance*1.03f )//Need more tolerance here!!
                            {
                                pa = t1;
                            }
                            else
                            {
                                pa = t2; 
                                depth = sign_pafo*t2.DotProduct( volB->mAxis ) - volB->mLength;
                                rAssert( depth < mCollisionDistance*1.03f );
                                if( depth >= mCollisionDistance*1.03f )
                                    continue;
                            }
                            pa.Add( volB->mPosition );
                            l_pb.ScaleAdd( pa, -depth, mNormal );
                        }
                        else
                        {
                            //Use normal along the radius that connect the center of the circle to pa.
                            pa.Add( volB->mPosition );
                            if( dr != 0.0f )
                                mNormal.Scale( 1/dr);
                            else
                                mNormal = volB->mAxis;
                            depth = dr - volB->mCylinderRadius;
                            l_pb.ScaleAdd( pa, -depth, mNormal );
                        }
                        AddCollision( volA, pa, volB, l_pb, depth, mNormal );
					}
				}	
			}
		}
    }
}


// a vertex can collide with more than a face at a time. we only want the less deep collision
// we then start we the less deep and so on
void SubCollisionDetector::BoxSphereColl(OBBoxVolume* volA, CollisionVolume* volB, Vector& Pb, float rad)
{
    Vector N, pa, pb = Pb;
    float dist;
    if (PointBoxColl(volA, pb, &pa, &N, dist, rad, true)) // true means pa will be computed
    {
        //Test to not add a collision that hit a capsule inside it's cylindrical part.
        if (volB->Type() == CylinderVolumeType && !mSphereApproxB)
        {
            CylinderVolume* cyl = (CylinderVolume*)volB;
            if (!cyl->mFlatEnd)
            {            
                Vector lpb;
                lpb.Sub(pb,volB->mPosition);
                if (Fabs(lpb.DotProduct(cyl->mAxis)) < cyl->mLength - mMinCollisionDistance )
                    return;
            }
        }
        AddCollision(volA, pa, volB, pb, dist, N);
    }
}

// Return true if it exist a separating axis between the two given cylinder.
// else return false indicating that there is some collision to be detected.
bool SubCollisionDetector::CylCylSepPlane(CylinderVolume* volA, CylinderVolume* volB)
{
    //test the sphere
    if (!volA->mFlatEnd  || !volB->mFlatEnd )
        if (CylCylEnd(volA, volB, false, false)) return false;
        
        //else the two are flat end and there is nothing special to do with ends.
        
        // test the cyl
        float projection, a, b;
        a = Fabs(volA->mAxis.DotProduct(volB->mAxis));
        b = Sqrt(1.0f-Sqr(a));
        projection = (a * volB->mLength) + (b * volB->mCylinderRadius) + volA->mLength;
        if (Fabs(mT.DotProduct(volA->mAxis)) > mCollisionDistance + projection)
            return true;
        projection = (a * volA->mLength) + (b * volA->mCylinderRadius) + volB->mLength;
        if (Fabs(mT.DotProduct(volB->mAxis)) > mCollisionDistance + projection)
            return true;
        
        Vector axe;
        axe.CrossProduct(volA->mAxis, volB->mAxis);
        a = axe.Magnitude();
        projection = volA->mCylinderRadius + volB->mCylinderRadius; 
        if (Fabs(mT.DotProduct(axe)) > a * (mCollisionDistance + projection))
            return true;
        
        return false;
}

// return true if there is collision.
// Do not use this method in attempt to search for a separating plane in the case 
// where the two cylinders have flat end. 
bool SubCollisionDetector::CylCylEnd(CylinderVolume* volA, CylinderVolume* volB, bool testSphereA, bool addColl)
{
    Vector pb, pa, psb;
    Vector *ppa, *ppb;
    bool collide = false;
    
    if (addColl)
    {
        ppa = &pa;
        ppb = &pb;
    }
    else
    {
        ppa = ppb = NULL;
    }
    
    if( !volA->mFlatEnd  && !volB->mFlatEnd  )
    {
        //Test seperating plane or collision between the four sphere/sphere possible
        //combinations.
        int i, j;
        Vector ca, cb, wca, wcb;
        float  dist, la, lb;
        for( i=0, la=volA->mLength ; i<2 ; i++, la *= -1.0f )
        {
            for( j=0, lb=volB->mLength ; j<2 ; j++, lb *= -1.0f )
            {
                ca.Scale(la, volA->mAxis);
                cb.Scale(lb, volB->mAxis);
                wca.Add( volA->mPosition, ca );
                wcb.Add( volB->mPosition, cb );
                mNormal.Sub( wca, wcb ); //from sphere's center b to sphere's center a
                dist = mNormal.NormalizeSafe();
                if( dist <= volA->mCylinderRadius + volB->mCylinderRadius + mCollisionDistance )
                {
                    if( ca.DotProduct(mNormal) > 0.0f )//Test only upper half-side
                        continue;
                    if( cb.DotProduct(mNormal) < 0.0f )//Test only upper half-side
                        continue;
                    
                    collide = true;
                    if( addColl )
                    {
                        pa.ScaleAdd( wca, -volA->mCylinderRadius, mNormal );
                        pb.ScaleAdd( wcb,  volB->mCylinderRadius, mNormal );
                        dist -= ( volA->mCylinderRadius + volB->mCylinderRadius );
                        AddCollision(volA, pa, volB, pb, dist, mNormal);
                    }
                    return collide;
                }
            }
        }
    }
    else if( volA->mFlatEnd  && volB->mFlatEnd  )
    {
        // The two cylinders have flat ends.
        // Have to look for collision between the four circle/circle possible
        // combinations.
        int i, j;
        float  la, lb, e1_m, fnfn;
        Vector ca, cb;
        Vector e1_e1, e1;
        fnfn = volB->mAxis.DotProduct(volA->mAxis);
        e1.CrossProduct( volA->mAxis, volB->mAxis );
        e1_e1 = e1; // A copy of e1 not normalized.
        e1_m  = e1.NormalizeSafe(); 
        for( i=0, la=volA->mLength ; i<2 ; i++, la *= -1.0f )
        {
            for( j=0, lb=volB->mLength ; j<2 ; j++, lb *= -1.0f )
            {
                ca.ScaleAdd( volA->mPosition, la, volA->mAxis );//world position of one circle.
                cb.ScaleAdd( volB->mPosition, lb, volB->mAxis );//world position of the other circle.
                mT.Sub( ca, cb ); //from circle's center b to circle's center a 
                if( la*lb*fnfn > 0 )
                    continue; //cercle's outward normal are in the same direction.
                
                if( e1_m < mCollisionDistance/Max(volA->mCylinderRadius,volB->mCylinderRadius) )
                {
                    //circles are almost parallels. Fast, simple and enough accurate.
                    float depth;
                    mNormal.Scale( Sign(lb), volB->mAxis ); //outward normal from circle b.
                    depth = mT.DotProduct(mNormal);
                    if( depth < mCollisionDistance && -depth < Min(volA->mLength,volB->mLength)*2.0f )
                    {
                        //Look for an intersection.
                        GEOM_CODE res;
                        cb.ScaleAdd( depth, mNormal);//this bring circle b in the same plane as circle a.
                        Vector pa2;
                        if( volA->mCylinderRadius < volB->mCylinderRadius )
                            res = CircleCircleIntersection(ca,cb,volA->mAxis,volA->mCylinderRadius,volB->mCylinderRadius,true,pa,pa2 );
                        else
                            res = CircleCircleIntersection(cb,ca,volB->mAxis,volB->mCylinderRadius,volA->mCylinderRadius,true,pa,pa2 );
                        
                        if( res == NO_INTERSECTION )
                            continue;
                        collide = true;
                        if( addColl == false )
                            return true;
                        
                        pb.ScaleAdd( pa, -depth, mNormal );
                        AddCollision(volA, pa, volB, pb, depth, mNormal );
                        
                        if( res == TWO_POINTS )
                        {
                            pb.ScaleAdd( pa2, -depth, mNormal );
                            AddCollision(volA, pa2, volB, pb, depth, mNormal );
                        }
                        return true;
                    }
                    else
                        continue;
                }
                else
                {
                    float ftoa, ftob, intr_a, intr_b;
                    
                    ftoa = mT.DotProduct(volA->mAxis);
                    intr_a = Fabs(ftoa) - e1_m*volB->mCylinderRadius;
                    
                    if( intr_a > mCollisionDistance )
                        continue;
                    
                    ftob = mT.DotProduct(volB->mAxis);
                    intr_b = Fabs(ftob) - e1_m*volA->mCylinderRadius;
                    
                    if( intr_b > mCollisionDistance )
                        continue;
                    
                    //First two separating axes are actually not. Go further for e1 the last
                    //separating axis to test. But, first call PlanePlaneIntersection
                    //to compute the reference point to locate the axis in space.
                    
                    //Chose the normal according to the deppest intersection.
                    bool a_collide;
                    if( intr_a <= intr_b )
                    {
                        a_collide = true;
                        mNormal.Scale( Sign(lb), volB->mAxis ); //outward normal from circle b.
                    }
                    else
                    {
                        a_collide=false;
                        mNormal.Scale( Sign(la), volA->mAxis ); //outward normal from circle a.
                        mT.Scale(-1);
                    }
                    
                    Vector pt, c1, c2;
                    float b1, b2, c1_e1, c2_e1;
                    
                    //e1 must be not normalized when pass in PlanePlaneIntersection.
                    //On return, e1 and pt define the line making the intersection of the
                    //two planes which support the two circles.
                    PlanePlaneIntersection( ca, cb, volA->mAxis, volB->mAxis, &pt, &e1_e1, false );
                    
                    float dc,dp;
                    c1.Sub( ca, pt );
                    c2.Sub( cb, pt );
                    c1_e1 = c1.DotProduct(e1);
                    c2_e1 = c2.DotProduct(e1);
                    b1 = c1.DotProduct(c1) - Sqr(c1_e1);
                    b2 = c2.DotProduct(c2) - Sqr(c2_e1);
                    b1 = Max( 0.0f, b1 );//Can be negative only by numerical precicion error.
                    b2 = Max( 0.0f, b2 );
                    
                    float da1, da2;
                    da1 = Sqr(volA->mCylinderRadius) - b1;
                    if( da1 > 0 )
                        da1 = Sqrt( da1 ); 
                    else
                        da1 = 0.0f;
                    
                    da2 = Sqr(volB->mCylinderRadius) - b2;
                    if( da2 > 0 )
                        da2 = Sqrt( da2 );
                    else
                        da2 = 0.0f;
                    
                    dc = Fabs( c1_e1-c2_e1 );//distance between the two circles on e1.
                    dp = dc - (da1+da2);
                    
                    if( dp < mCollisionDistance )//else e1 is a separating axis
                    {
                        float depth;
                        Vector pa2;
                        GEOM_CODE res;
                        
                        Vector *cap, *cbp;
                        CylinderVolume  *Elap, *Elbp;
                        if( a_collide )
                        {
                            Elap = volA;
                            Elbp = volB;
                            cap  = &ca;
                            cbp  = &cb;
                        }
                        else
                        {
                            Elap = volB;
                            Elbp = volA;
                            cap  = &cb;
                            cbp  = &ca;
                        }
                        
                        dp= 1-Sqr(e1_m);
                        if( dp > 0 )
                            dp = Sqrt(dp);
                        else
                            dp = 0.0f;
                        
                        res = CircleCircleIntersection(*cbp,*cap,Elbp->mAxis,Elbp->mCylinderRadius,Elap->mCylinderRadius*dp,true,pa,pa2);
                        
                        pb.Sub( pa, *cbp );
                        depth = pb.DotProduct(mNormal);
                        
                        //Next two collisions points are not accurate at all when the two cercle's 
                        //normals are too far from being parallel.
                        if( (res == ONE_POINT || res == TWO_POINTS) && depth < mCollisionDistance && e1_m<0.5f )
                        {
                            if( addColl == false )
                                return true;
                            pb.ScaleAdd( pa, -depth, mNormal );
                            AddCollision(Elap, pa, Elbp, pb, depth, mNormal );
                        }
                        pb.Sub( pa2, *cbp );
                        depth = pb.DotProduct(mNormal);
                        if( res == TWO_POINTS && depth < mCollisionDistance && e1_m<0.5f )
                        {
                            if( addColl == false )
                                return true;
                            pb.ScaleAdd( pa2, -depth, mNormal );
                            AddCollision(Elap, pa2, Elbp, pb, depth, mNormal );
                        }
                        
                        //Now set a collision point somewhere on the colliding circle along
                        //one of its radius. clip to stay inside the "obstacle" cylinder.
                        
                        Vector e1p;
                        float sign_e1p_fN;
                        e1p.CrossProduct( Elap->mAxis, e1 );
                        e1p.NormalizeSafe();
                        sign_e1p_fN = Sign(e1p.DotProduct(mNormal));
                        
                        //Contact point relative to ca at a fraction f of ra.
                        //Compute f to make sure that the contact point is inside cylinder b
                        
                        float f1=0.0f, f2=0.0f;
                        float delta, a, b, c, q;
                        float e1_fOb = e1p.DotProduct(Elbp->mAxis);
                        float T_fOb  = mT.DotProduct(Elbp->mAxis);
                        
                        //Here we solve the quadratic equation ax2+bx+c=0
                        a = Sqr(Elap->mCylinderRadius)*(1-Sqr(e1_fOb));
                        b = 2*Elap->mCylinderRadius*(mT.DotProduct(e1p) - e1_fOb*T_fOb);
                        c = mT.DotProduct(mT) - Sqr(T_fOb) - Sqr(Elbp->mCylinderRadius);
                        delta = Sqr(b) - 4*a*c;
                        if( Fabs(a*c) < MICRO_EPS )//From numerical receipes, suppose to be more accurate.
                        {
                            if( delta >= 0 )
                                q = -(b+Sign(b)*Sqrt(delta))/.5f;
                            else
                                q = 0.0f;
                            
                            if( q == 0.0f || a == 0.0f ) //otherwise e1p and e2p are perpendicular
                            {
                                f1 = f2 = 0.0f;
                                if( mT.Magnitude() < Elbp->mCylinderRadius )
                                {
                                    f1 = 1.0f;
                                    f2 =-1.0f;
                                }
                            }
                            else
                            {
                                f1 = q/a;
                                if( q != 0 )
                                    f2 = c/q;
                            }
                        }
                        else
                        {
                            if( delta >= 0 )
                            {
                                f1 = (-b + Sqrt(delta)) / a / 2;
                                f2 = (-b - Sqrt(delta)) / a / 2;
                            }
                            else
                                continue;
                        }
                        
                        //Clip f1 and f2 to 1 to make sure to be inside circle a.
                        f1 = ( Fabs(f1) <= 1.0f ) ? f1 : Sign(f1)*1.0f;
                        f2 = ( Fabs(f2) <= 1.0f ) ? f2 : Sign(f2)*1.0f;
                        pa.ScaleAdd( *cap, f1*Elap->mCylinderRadius, e1p );
                        pb.Sub( pa, *cbp );
                        depth = pb.DotProduct(mNormal);
                        
                        //keep only inside points.
                        if( depth < mCollisionDistance )
                        {
                            if( addColl == false )
                                return true;
                            pb.ScaleAdd( pa, -depth, mNormal );
                            AddCollision(Elap, pa, Elbp, pb, depth, mNormal );
                        }
                        
                        pa.ScaleAdd( *cap, f2*Elap->mCylinderRadius, e1p );
                        pb.Sub( pa, *cbp );
                        depth = pb.DotProduct(mNormal);
                        if( depth < mCollisionDistance )
                        {
                            if( addColl == false )
                                return true;
                            pb.ScaleAdd( pa, -depth, mNormal );
                            AddCollision(Elap, pa, Elbp, pb, depth, mNormal );
                        }
                        return true;
               }
            }
         }
      }
   }
   else
   {
       //One and only one of the two cylinders is flat end.
       //The none flat end cylinder must appears in the second position.
       CylinderVolume *l_Ela=volA, *l_Elb=volB;
       if( volB->mFlatEnd  )
       {
           mT.Scale(-1.0f);
           l_Ela=volB;
           l_Elb=volA;
       }
       float dist, length = l_Elb->mLength;
       for( int i=0 ; i<2 ; i++, length*=-1 )
       {
           psb.ScaleAdd(l_Elb->mPosition, length, l_Elb->mAxis);
           mT.Sub(l_Ela->mPosition, psb); //from center of sphere b to center of cylinder a.
           dist = mT.DotProduct(mT);
           if (CylSphereColl(l_Ela, psb, l_Elb->mCylinderRadius, mNormal, dist, ppa, ppb, testSphereA))
           {
               collide = true;
               if (addColl)
               {
                   if( l_Ela==volA )
                       AddCollision(l_Ela, pa, l_Elb, pb, dist, mNormal);
                   else
                   {
                       Vector tmp = mNormal;
                       tmp.Scale(-1.0f);
                       AddCollision(l_Elb, pb, l_Ela, pa, dist, tmp);
                   }
               }
               else return collide;
           }
       }
   }
   return collide;
}

bool computeEdge( CylinderVolume* volA, CylinderVolume* volB, Vector &in_N, Vector &o_pa )
{
    
    float r2, c1, s1;
    
    r2 = in_N.NormalizeSafe();
    
    c1 = in_N.DotProduct(volA->mAxis);
    Clamp( c1, -1.0f, 1.0f );
    if( 1.0f - Fabs(c1) < MICRO_EPS )
        return false; //No edge defined.
    s1 = Sqrt( 1.0f - Sqr(c1) );
    r2 = volA->mCylinderRadius/s1;
    o_pa.Scale(r2,in_N);
    o_pa.ScaleAdd( -r2*c1, volA->mAxis );
    
    return true;
    
}

// this will make sure only the deepest collision is kept otherwise 
// the result could be different depending on volume order
bool SubCollisionDetector::CylCylSphereEndSpecialFilter(float newdist)
{
    int last = mCollisionList->GetSize()-1;
    if (newdist < mCollisionList->GetAt(last).mDistance)
    {
        mCollisionList->RemoveAt(last);
        mNewColl--;
        return false;
    }
    return true;
}

//This will detect collision between cylinder's face and none-flat ends (if any). 
//All combination. 
void SubCollisionDetector::CylCylSphereEnd(CylinderVolume* inVolA, CylinderVolume* inVolB)
{
    Vector N, pb, pa;
    CylinderVolume *l_volA=inVolA, *l_volB=inVolB;

    int currentNewColl = mNewColl;

    for( int i=0 ; i<2 ; i++, l_volA=inVolB, l_volB=inVolA )
    {
        if( l_volB->mFlatEnd )
            continue;
        
        Vector pos;
        pos.ScaleAdd(l_volB->mPosition, l_volB->mLength, l_volB->mAxis);
        mT.Sub(l_volA->mPosition, pos);
        mNt = mT.DotProduct(mT);
        if (CylSphereColl(l_volA, pos, l_volB->mCylinderRadius, N, mNt, &pa, &pb))
        {
            float test = N.MagnitudeSqr();
            if (Fabs(test-1.0f) > 0.01f)
                N.Scale(Inverse(Sqrt(test)));

            if (currentNewColl < mNewColl && CylCylSphereEndSpecialFilter(mNt))
            {
                continue;
            }
            AddCollision(l_volA, pa, l_volB, pb, mNt, N);
            continue;
        }
        pos.ScaleAdd(l_volB->mPosition, -l_volB->mLength, l_volB->mAxis);
        mT.Sub(l_volA->mPosition, pos);
        mNt = mT.DotProduct(mT);
        if (CylSphereColl(l_volA, pos, l_volB->mCylinderRadius, N, mNt, &pa, &pb))
        {
            if (currentNewColl < mNewColl && CylCylSphereEndSpecialFilter(mNt))
            {
                continue;
            }
            AddCollision(l_volA, pa, l_volB, pb, mNt, N);
            continue;
        }
    }
}


// Add collision in case where cylinder's face intersects the flat end of the other one.
void SubCollisionDetector::CircleCyl(CylinderVolume* volA, CylinderVolume* volB)
{
    
    CylinderVolume *l_Ela=volA, *l_Elb=volB;
    
    float foa_fob = Fabs(volA->mAxis.DotProduct(volB->mAxis));
    //Test the best candidate edge of cylinder A to the circle of cylinder B.
    for( int i=0 ; i<2 ; i++, l_Ela=volB, l_Elb=volA )
    {
        //Cylinder B do not define an circle, skip this test. 
        if( !l_Elb->mFlatEnd  )
            continue;
        
        Vector l_N, pa;
        
        //This is not easy to find the best candidate edge. In the first case,
        //set of edges are parallels to the circle. We chose the one that is along
        //the normal of the circle. This defines the closest edge to the circle.
        //This become incorrect when the cylinder is near the edge of the circle. But
        //we can't do better for now.
        bool perpendicular;
        if( foa_fob < 0.1f )
        {
            perpendicular = true;
            l_N=l_Elb->mAxis;
        }
        else
        {
            perpendicular = false;
            l_N.Sub( l_Elb->mPosition, l_Ela->mPosition );
        }
        
        if( computeEdge( l_Ela, l_Elb, l_N, pa ) == false )
            continue;
        
        mT.Sub( l_Ela->mPosition, l_Elb->mPosition );
        if( perpendicular )
        {
            //Need to test if this one is not outside the circle
            Vector pb;
            pb.Add(mT,pa); //Center's cylinder to edge middle point.
            float  dr;
            l_N.CrossProduct(l_Elb->mAxis,l_Ela->mAxis);
            dr = Fabs(pb.DotProduct(l_N));
            if( dr > l_Elb->mCylinderRadius )
            {
                //This edge is off the circle we get in trouble. Use the other edge candidate.
                l_N.Sub( l_Elb->mPosition, l_Ela->mPosition );
                if( computeEdge( l_Ela, l_Elb, l_N, pa ) == false )
                    continue;
            }
        }
        
        float nft = mT.DotProduct(l_Elb->mAxis);
        float no = l_Ela->mAxis.DotProduct(l_Elb->mAxis) * l_Ela->mLength;
        CircleEdge( l_Ela, pa, l_Ela->mAxis, l_Ela->mLength, l_Elb, nft, no, true);
        
    }
}

//This method returns true if the cylinder and the sphere collide.
bool SubCollisionDetector::CylSphereColl(CylinderVolume *volA, const Vector& Pb, float rb, Vector& N, float& dist, Vector *ppa, Vector *ppb, bool testSphereA)
{
    float dl    = mT.DotProduct(volA->mAxis);  // projection on the axe of the dist between the object center
    float dr    = rmt::Max(0.0f, dist - Sqr(dl));  // dr2, projection on the radius
    float rad   = (volA->mFlatEnd  ? 0 : volA->mCylinderRadius);
    bool  collide = false;
    
    if (Fabs(dl) < volA->mLength + rb + mCollisionDistance + rad && dr < Sqr(volA->mCylinderRadius + rb + mCollisionDistance))
    {
        if (ppa == NULL || ppb == NULL) return true;
        
        rad = volA->mCylinderRadius;
        rAssert(dr>=0.0f);
        if( dr<0.0f )
            return collide;
        dr = Sqrt(dr);
        if (volA->mFlatEnd  && Fabs(dl) - volA->mLength > dr - rad && dr < rad) // coll with the end face 
        {
            //radial penetration is depper than the normal penetration.
            dist = Fabs(dl) - ( volA->mLength + rb );
            if (dist < mCollisionDistance )
            {
                N.Scale(Sign(dl), volA->mAxis);
                ppb->ScaleAdd(Pb, rb, N);     // pos on b
                ppa->ScaleAdd(*ppb, dist, N); // pos on a
                collide = true;
            }
        }
        else // test for collision with "side of cylinder" / sphere or circle / sphere
        {
            if (!volA->mFlatEnd  && Fabs(dl) >= volA->mLength)
            {  
                if (!testSphereA) return collide;
                ppa->ScaleAdd(volA->mPosition, -volA->mLength * Sign(dl), volA->mAxis);
                N.Sub(*ppa, Pb);
                dist = N.NormalizeSafe() - volA->mCylinderRadius - rb;
                if (dist < mCollisionDistance) // collision with the end sphere
                {
                    ppa->ScaleAdd(-volA->mCylinderRadius, N);
                    ppb->ScaleAdd(*ppa, -dist, N);
                    collide = true;
                }
            }
            else if (dr > VERY_SMALL) // coll with side
            {
                Vector dp;
                dp.Scale(-dl, volA->mAxis);
                N.Add(mT, dp);
                
                dist = dr - rad - rb;
                N.Scale(1.0f/dr);
                
                float tmp = N.MagnitudeSqr();
                if (Fabs(tmp-1.0f) > 0.01f)
                    N.Scale(1.0f/Sqrt(tmp)); // if the Magnitude is not close enough to 1...
                
                if (Fabs(dl) > volA->mLength) //Sphere is above the flat end.
                    dp.Scale(volA->mLength/Fabs(dl));
                
                ppa->ScaleAdd(dp, -rad, N);
                ppa->Add(volA->mPosition); // pos on a
                
                if (Fabs(dl) <= volA->mLength) // coll with the side of the Cyl
                {
                    ppb->ScaleAdd(Pb, rb, N); // pos on b
                    collide = true;
                }
                else if (volA->mFlatEnd ) // collision with the circle
                {
                    N.Sub(*ppa, Pb);
                    dist = N.NormalizeSafe();

                    if (dist == 0) // points are very close, try again with scaling
                    {
                        N.Sub(*ppa, Pb);
                        N.Scale(1000.0f);
                        dist = N.NormalizeSafe() / 1000.0f;
                    }
                    
                    if (dist != 0 && dist <= rb + mCollisionDistance)
                    {
                        dist -= rb;
                        ppb->ScaleAdd(*ppa, -dist, N); // pos on b
                        collide = true;
                    }
                }
            }
        }
    }
    return collide;
}

/* 
Computed the intersection of the two given planes. The result, if it exist, is a line with
direction io_ns and reference point o_pt. 
If the caller already know the the direction it can pass it in io_ns ( this must
be not normalized) along with in_computeDirection == false. 
Else, pass in in_computeDirection == true. 
On return, io_ns will be set to the direction of the line (not normalized).
*/
bool SubCollisionDetector::PlanePlaneIntersection
( 
 Vector &in_c1, /* Reference point of the first plane. */
 Vector &in_c2, /* Reference point of the second plane. */
 Vector &in_n1, /* Normal of the first plane. */
 Vector &in_n2, /* Normal of the second plane. */
 Vector *o_pt,  /* Reference point of the output line. */
 Vector *io_ns, /* Line's direction of the intersection.*/
 bool   in_computeDirection 
 )
{
    rAssert( o_pt );
    rAssert( io_ns );
    
    if( in_computeDirection )
        io_ns->CrossProduct( in_n1, in_n2 );
    
    float L1;
    L1 = io_ns->Magnitude();
    if( L1 < MICRO_EPS )
        return false;
    
    float A1, A2;
    A1 = -in_c1.DotProduct(in_n1);
    A2 = -in_c2.DotProduct(in_n2);
    
    //find the largest componante of io_ns
    int u=0, v=1, w = GetIndexOfMaxVectorComponent(*io_ns,true);
    switch(w)
    {
    case 0:
        u=1;v=2;
        break;
    case 1:
        u=2;v=0;
        break;
    case 2:
        u=0;v=1;
        break;
    }
    float pu = GetVectorComponent(in_n1,v)*A2 - GetVectorComponent(in_n2,v)*A1;
    float pv = GetVectorComponent(in_n2,u)*A1 - GetVectorComponent(in_n1,u)*A2;
    
    pu /= GetVectorComponent(*io_ns,w);
    pv /= GetVectorComponent(*io_ns,w);
    
    SetVectorComponent( *o_pt, w, 0.0f );
    SetVectorComponent( *o_pt, u, pu );
    SetVectorComponent( *o_pt, v, pv );
    
    return true;
}

/*
    This method compute and returns intersectings points between 2 circles.
    The return value indicates the status of the collision. It can be:
    NO_INTERSECTION, ALL_INSIDE, ONE_POINT or TWO_POINTS.
    If one circle is completely inside the other one, o_p1 is set to the position of the
    first circle and the method return ALL_INSIDE. The current implentation does no distinction
    between the case where a circle is completely inside the other and the case where
    the two circles overlaps.
    When there is no intersection the function returns NO_INTERSECTION, and o_p1 and o_p2 
    are untouched.
    IMPORTANT: This method assume that the two circles are in the SAME plane.
*/
GEOM_CODE  SubCollisionDetector::CircleCircleIntersection
 ( 
     const Vector &in_c1, //Position of the center of the first circle.
     const Vector &in_c2, //Position of the center of the second circle.
     const Vector &in_n1, //Normal of one circle. ( this method assume that the two circle are in the same plane).
     float in_r1,         //Radius of the first circle.
     float in_r2,         //Radius of the second circle.
     bool  in_all,        //If false return only the mid point on the segment joining the two real intersecting points
     Vector &o_p1,        //One intersecting point on output. ( valid if the returned value is equal to 1 or 2 ).
     Vector &o_p2         //Second intersecting point on output (valid if the returned value equal 2 ).
 )
{
    float d;
    Vector c1_c2;
    c1_c2.Sub( in_c2, in_c1 );
    d = c1_c2.DotProduct(c1_c2);
    
    if( d > Sqr(in_r1+in_r2+2.0f*mMinCollisionDistance) )
        return NO_INTERSECTION;
    
    d = Sqrt(d);
    if( d + in_r1 < in_r2 + mMinCollisionDistance || d + in_r2 < in_r1 + mMinCollisionDistance)
    {
        o_p1 = in_c1;
        return ALL_INSIDE;
    }
    float a, h, r1_2=Sqr(in_r1);
    a = ( r1_2 - Sqr(in_r2) + Sqr(d) ) / (2.0f*d);
    h = r1_2 - Sqr(a);
    
    o_p1.ScaleAdd( in_c1, a/d, c1_c2 );
    if( !in_all ) //This is the mid point on the segement that joints the two real points.
        return ONE_POINT;
    
    if( h <= Sqr(mCollisionDistance) ) 
        return ONE_POINT; //In the given tolerance the two points are the same.
    
    //But if we want the two real points:
    Vector e1;
    e1.CrossProduct(c1_c2,in_n1);
    e1.NormalizeSafe();
    rAssert(h>=0.0f);
    if( h < 0.0f )
        return NO_INTERSECTION;
    h = Sqrt(h);
    o_p2.ScaleAdd( o_p1,   h, e1 );
    o_p1.ScaleAdd( o_p2, -2.0f*h, e1 );
    return TWO_POINTS;
    
}

/*
bool SubCollisionDetector::PointElColl(CollisionVolume *vol, Vector& p, float &dist)
{
bool ret=false;

  switch (vol->Type())
  {
  case OBBoxEl:
  {
  ret = PointBoxColl((OBBoxVolume*)vol, p, dist);
  break;
  }
  default:
  rAssertMsg(false, "Not yet implemented", "");
  }
  return ret;
  }
*/


#ifdef TEST_COLL
void SubCollisionDetector::TestNormalIntegrity(Collision* c)
{
    
    Vector dp;
    Vector pa = c->GetPa();
    Vector pb = c->GetPb();
    
    dp.Sub(pa, pb);
    float dist = dp.NormalizeSafe();
    dist = dist * Sign(dp.DotProduct(c->mNormal));
    if (Fabs(dist-c->mDistance) > Fabs(c->mDistance) * 0.01f)
    {
        int i=0; // put a break point here: get here if problem 
    }
    
    dp.NormalizeSafe();
    dist = dp.DotProduct(c->mNormal);//Test outward Normals
    if( dist * Sign(c->mDistance) < 0 && Fabs(dist) > .05 )
    {
        int i=0; // put a break point here: get here if problem 
    }
    
    dist = c->mNormal.Magnitude(); //Test normal's magnitude
    if( Fabs(dist - 1.0f) > .01f && dist>MICRO_EPS)
    {
        int i=0;
    }
    /*
    float CDSCALE = RMTOREAL(5.0);
    if (c->mDistance * rmV3Dot(&dp, &c->mNormal) < 0)
    int i=0;
    else
    int i=1;
    if (c->mDistance > CDSCALE * mCollisionDistance)
    int i=0;
    if (rmV3Dot(&dp, &dp) > CDSCALE * mCollisionDistance)
    int i=0;
    */
}
#endif // TEST_COLL

void SubCollisionDetector::AddCollision(CollisionVolume* volA, const Vector& pa, CollisionVolume* volB, const Vector& pb, float dist, const Vector& N)
{
    float test = Fabs(N.MagnitudeSqr()-1.0f);
    rAssert(test < 0.01f);
    if (test > 0.01f)
        return;
    
    Collision newcoll(volA, pa, volB, pb, dist, N);
    
#ifdef TEST_COLL
    TestNormalIntegrity(&newcoll);
#endif
    
    if( mCollisionFilter && mNewColl > 0)
    {
        bool l_bAdd = true;
        CollisionVolume *vola, *volb, *new_vola, *new_volb;
        Vector      new_vPa, new_vPb;
        Vector      new_vN;
        float       new_depth;
        
        new_vola = volA;
        new_volb = volB;
        
        if( mCollisionFilter & FILTER_BY_POSITION )
        {
            new_vPa  = newcoll.mPositionA;
            new_vPb  = newcoll.mPositionB;
        }
        if( mCollisionFilter & FILTER_BY_DEPTH )
        {
            new_vN      = newcoll.mNormal;
            new_depth   = newcoll.mDistance;
        }
        if( mCollisionFilter & ~(FILTER_BY_DEPTH|FILTER_BY_POSITION) )
        {
            rAssertMsg( 0, "Invalide case");
        }
        
        Collision *pColl;
        int l_nbColl = mCollisionList->GetSize();
        for( int i=l_nbColl-1 ; i >= l_nbColl-mNewColl ; i-- )
        {
            pColl = &mCollisionList->GetAt( i );
            vola  = pColl->mCollisionVolumeA;
            volb  = pColl->mCollisionVolumeB;
            
            int identity = (new_vola == vola && new_volb == volb) ? 1 : 0 ;
            if( identity == 0 )
                identity = (new_vola == volb && new_volb == vola) ? -1 : 0 ;
            
            if (identity)
            {
                //At this point the new collision and the current to compare have the same
                //objects. Now test if they relate the same index.
                if (identity==1)
                {
                    if ( newcoll.mIndexA != pColl->mIndexA || newcoll.mIndexB != pColl->mIndexB )
                    {
                        continue;
                    }
                }
                else
                {
                    if ( newcoll.mIndexA != pColl->mIndexB || newcoll.mIndexB != pColl->mIndexA )
                    {
                        continue;
                    }
                }

                // try to eliminate two collisions at same position
                if( mCollisionFilter & FILTER_BY_POSITION )
                {
                    Vector   *vPa, *vPb;
                    
                    if( identity == 1 )
                    {
                        vPa   = &pColl->mPositionA;
                        vPb   = &pColl->mPositionB;
                    }
                    else
                    {
                        vPa   = &pColl->mPositionB;
                        vPb   = &pColl->mPositionA;
                    }
                    
                    Vector vtmp;
                    vtmp.Sub(*vPa, new_vPa);
                    float l_dist1 = vtmp.DotProduct(vtmp);
                    vtmp.Sub(*vPb,new_vPb);
                    float l_dist2 = vtmp.DotProduct(vtmp);
                    float minDist = Sqr(mMinCollisionDistance/1000.0f);
                    if( l_dist1 < minDist && l_dist2 < minDist )//TODO: Have a caracteristic length instead of an hard coded number.
                    {
                        // Dist collision is too close from an existing one.
                        // Do not add it in the list.
                        l_bAdd = false;
                        break;
                    }
                }

                // try to eliminate two contradictory collisions. Typically, if a sphere 
                // gets in the box, which way should it leaves?here we choose the direction
                // with minimum of penetration.
                if( (mCollisionFilter & FILTER_BY_DEPTH) &&
                    !(volA->GetCollisionObject()->GetSimState()->mAIRefIndex == 1 && volB->GetCollisionObject()->GetSimState()->mAIRefIndex == 1) )
                {
                    static float Almost_Parallel = 0.7f;

                    if (Fabs(pColl->mNormal.DotProduct(new_vN)) < Almost_Parallel)
                    {
                        if (newcoll.mDistance > pColl->mDistance)
                        {
                            *pColl = newcoll;
                        }
                        l_bAdd = false;
                        break;
                    }
                }
            }
            else
            {
                //Starting by the end of the list, as soon as collisions do not
                //relate the same two objects, we can stop to search.
                break;
            }
        }
        
        if( l_bAdd )
        {
            mNewColl++;
            mCollisionList->Add(newcoll);

#ifdef COLLECT_PAIR_COLLISION_HISTORY
            mPairCollisionInfoHolder.AddCollision(newcoll);
#endif
        }
   }
   else
   {
        mNewColl++;
        mCollisionList->Add(newcoll);

#ifdef COLLECT_PAIR_COLLISION_HISTORY
        mPairCollisionInfoHolder.AddCollision(newcoll);
#endif
   }
}

TArray<Collision>* SubCollisionDetector::GetCollisionList()
{
    return mCollisionList;
}

void SubCollisionDetector::SetCollisionList( TArray<Collision>& inlist )
{
    mCollisionList = &inlist;
}

} // sim
