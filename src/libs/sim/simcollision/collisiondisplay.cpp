#include "simcollision/collisiondisplay.hpp"
#include "simcollision/collisionmanager.hpp"
#include "simcommon/dline2.hpp"
#include "simcommon/simmath.hpp"
#include "p3d/debugdraw.hpp"
#include <p3d/matrixstack.hpp>
#include <p3d/shader.hpp>
#include <p3d/utility.hpp>

namespace sim
{
    
void DrawBBox(CollisionVolume* volume);
void DrawCyl(CylinderVolume* volume);
void DrawSphere(SphereVolume* volume);
void DrawOBBox(OBBoxVolume* volume);
void DrawWall(WallVolume* volume);

void DrawCylinderShape(CylinderVolume* el);
void DrawSphereShape(SphereVolume* el);
void DrawOBBoxShape(OBBoxVolume* el);


static DrawVolumeMethod sDrawVolumeMethod = DrawVolumeOutline;//DrawVolumeShape;
static float sDrawVolumeShapeScale = 1.01f; // draw the shape 1% larger tahn the volumes


void SetDrawVolumeMethod(DrawVolumeMethod inDrawVolumeMethod)
{
    sDrawVolumeMethod = inDrawVolumeMethod;
}

// in the case of STreeObj, if both DRAW are defined, the STreeSubObj will be drawn twice
#define DRAW_PARENT
#define DRAW_CHILD

int MAXLEVEL = 0;

// the draw the CollEl AND its BBox
//#define DRAW_ALL_BBOX

void DrawCollisionObject(CollisionObject* inObject)
{
    DrawLineToggler toggler;

    //inObject->Update();
    DrawCollisionVolume(inObject->GetCollisionVolume());
}

void DrawCollisionVolume(CollisionVolume* volume)
{
    DrawLineToggler toggler;

#ifdef DRAW_CHILD
    TList<CollisionVolume*>* list = volume->SubVolumeList();
    if (list != NULL && list->GetSize())
    {
        for (int i=0; i<list->GetSize(); i++)
        {
            DrawCollisionVolume((*list)[i]);
        }
#ifndef DRAW_PARENT
        return; // so the parent elem won't be drawn
#endif // DRAW_PARENT
    }
#endif // DRAW_CHILD
    
#ifdef DRAW_ALL_BBOX
    DrawBBox(volume);
#endif
    
    if (volume->IsVisible())
    {
        if (sDrawVolumeMethod == DrawVolumeOutline)
        {
            switch (volume->Type())
            {
            case CollisionVolumeType:
                break;
            case BBoxVolumeType:
#ifndef DRAW_ALL_BBOX
                DrawBBox(volume);
#endif
                break;
            case SphereVolumeType:
                DrawSphere((SphereVolume*)volume);
                break;
            case CylinderVolumeType:
                DrawCyl((CylinderVolume*)volume);
                break;
            case OBBoxVolumeType:
                DrawOBBox((OBBoxVolume*)volume);
                break;
            case WallVolumeType:
                DrawWall((WallVolume*)volume);
                break;
            default:
                break;
            }
        }
        else
        {
            switch (volume->Type())
            {
            case CollisionVolumeType:
                break;
            case BBoxVolumeType:
#ifndef DRAW_ALL_BBOX
                DrawBBox(volume);
#endif
                break;
            case SphereVolumeType:
                DrawSphereShape((SphereVolume*)volume);
                break;
            case CylinderVolumeType:
                DrawCylinderShape((CylinderVolume*)volume);
                break;
            case OBBoxVolumeType:
                DrawOBBoxShape((OBBoxVolume*)volume);
                break;
            case WallVolumeType:
                DrawWall((WallVolume*)volume);
                break;
            default:
                break;
            }
        }
    }
}

void DrawBBox(CollisionVolume* volume)
{
    DrawLineToggler toggler;

    if (MAXLEVEL && volume->SubVolumeList() != NULL) return;
    
    int colorScale = 1; // (volume->Level()-1)
    dBox3(volume->mPosition, volume->mBoxSize, tColour(rmt::Min(255, 25 * colorScale), 0, rmt::Max(0, 255 - 25 * colorScale)));
}

void DrawCyl(CylinderVolume* volume)
{
    DrawLineToggler toggler;

    rmt::Vector p1, p2;
    tColour colour(0, 255, 0);
    
    p1.ScaleAdd(volume->mPosition, volume->mLength, volume->mAxis);
    p2.ScaleAdd(volume->mPosition, -volume->mLength, volume->mAxis);
    dLine2(p1, p2, colour);
    
    if (!volume->mFlatEnd)
    {
        rmt::Vector p;
        p.ScaleAdd(volume->mPosition, volume->mLength, volume->mAxis);
        dLine3(p, volume->mCylinderRadius, colour);
        
        p.ScaleAdd(volume->mPosition, -volume->mLength, volume->mAxis);
        dLine3(p, volume->mCylinderRadius, colour);
    }
}

void DrawSphere(SphereVolume* volume)
{
    DrawLineToggler toggler;
    tColour colour(0, 255, 0);
    
    dLine3(volume->mPosition, volume->mSphereRadius, colour);
}

void DrawOBBox(OBBoxVolume* volume)
{
    DrawLineToggler toggler;

    rmt::Vector a;
    rmt::Vector p1, p2;
    tColour colour(0, 255, 0);
    
    for (int i=0; i<4; i++)
    {
        volume->Corner(a, i);
        p1.Add(volume->mPosition, a);
        p2.Sub(volume->mPosition, a);
        dStreamLine(p1, p2, colour, colour);
    }
}

void DrawWall(WallVolume* volume)
{
    DrawLineToggler toggler;

    // draw the wall
    rmt::Vector a, b;
    rmt::Vector p1, p2;
    int colorScale = 1; // (volume->Level()-1)
    tColour colour(25 * colorScale, 0, 255 - 25 * colorScale);
    
    const float scale = 4.0f;
    
    b.Set(1.0f, 1.0f, 1.0f);
    a.CrossProduct(b, volume->mNormal);
    b.CrossProduct(a, volume->mNormal);
    a.Normalize();
    b.Normalize();
    
    a.Scale(scale);
    b.Scale(scale);
    
    p1.x = (a.x + b.x) + volume->mPosition.x;
    p1.y = (a.y + b.y) + volume->mPosition.y;
    p1.z = (a.z + b.z) + volume->mPosition.z;
    p2.x = (-a.x - b.x) + volume->mPosition.x;
    p2.y = (-a.y - b.y) + volume->mPosition.y;
    p2.z = (-a.z - b.z) + volume->mPosition.z;
    dStreamLine(p1, p2, colour, colour);
    p1.x = (-a.x + b.x) + volume->mPosition.x;
    p1.y = (-a.y + b.y) + volume->mPosition.y;
    p1.z = (-a.z + b.z) + volume->mPosition.z;
    p2.x = (a.x - b.x) + volume->mPosition.x;
    p2.y = (a.y - b.y) + volume->mPosition.y;
    p2.z = (a.z - b.z) + volume->mPosition.z;
    dStreamLine(p1, p2, colour, colour);
    p1.x =  volume->mPosition.x;
    p1.y =  volume->mPosition.y;
    p1.z =  volume->mPosition.z;
    a.Scale(scale * 0.25f, volume->mNormal);
    p2.x =  a.x + volume->mPosition.x;
    p2.y =  a.y + volume->mPosition.y;
    p2.z =  a.z + volume->mPosition.z;
    dStreamLine(p1, p2, colour, colour);
}


void SubDisplayCollisions(CollisionManager* collisionManager, int area)
{
    tColour c(200, 100, 100);
    
    TArray<Collision>* list = collisionManager->GetCollisionList(area);
    for (int j=0; j<list->GetSize(); j++)
    {
        dStreamLine(list->GetAt(j).GetPositionA(), list->GetAt(j).GetPositionB(), c, c);
    }
}

void DisplayCollisions(CollisionManager* collisionManager, int area)
{
    DrawLineToggler toggler;

    if (collisionManager)
    {
        if (area == -1)
        {
            for (int i=0; i<collisionManager->NbArea(); i++)
                SubDisplayCollisions(collisionManager, i);
        }
        else
        {
            SubDisplayCollisions(collisionManager, area);
        }
    }
}

void DisplayCollisionObjects(CollisionManager* collisionManager, int area)
{
    DrawLineToggler toggler;

    if (collisionManager)
    {
        if (area == -1)
        {
            for (int i=0; i<collisionManager->NbArea(); i++)
            {
                TList<CollisionObject*>* list = collisionManager->GetCollisionObjectList(i);
                for (int j=0; j<list->GetSize(); j++)
                {
                    DrawCollisionObject(list->GetAt(j));
                }
            }
        }
        else
        {
            TList<CollisionObject*>* list = collisionManager->GetCollisionObjectList(area);
            for (int j=0; j<list->GetSize(); j++)
            {
                DrawCollisionObject(list->GetAt(j));
            }
        }
    }
}

void SubDisplayCollisionObjectPairs(CollisionManager* collisionManager, int area)
{
    tColour c(200, 0, 0);
    
    TArray<CollisionObjectPair>* list = collisionManager->GetCollisionObjectPairList();
    for (int j=0; j<list->GetSize(); j++)
    {
        rmt::Vector v1 = list->GetAt(j).mObjectA->GetSimState()->GetPosition();
        rmt::Vector v2 = list->GetAt(j).mObjectB->GetSimState()->GetPosition();
        dStreamLine(v1, v2, c, c);
    }
}

void DisplayCollisionObjectPairs(CollisionManager* collisionManager, int area)
{
    DrawLineToggler toggler;

    if (collisionManager)
    {
        if (area == -1)
        {
            for (int i=0; i<collisionManager->NbArea(); i++)
            {
                SubDisplayCollisionObjectPairs(collisionManager, i);
            }
        }
        else
        {
            SubDisplayCollisionObjectPairs(collisionManager, area);
        }
    }
}



//
//
//

bool TestPointOnSurf(CollisionVolume* volume, rmt::Vector r, bool rel)
{
    float smalldist = volume->mSphereRadius * 0.02f;
    
    if (rel == false)
        r.Sub(volume->mPosition);
    
    switch (volume->Type())
    {
    case OBBoxVolumeType:
        {
            OBBoxVolume* e = (OBBoxVolume*) volume;
            float f[3];
            int i;
            for (i=0; i<3; i++) 
                f[i] = rmt::Fabs(e->mAxis[i].DotProduct(r));
            for (i=0; i<3; i++)
            {
                if (rmt::Fabs(f[i] - e->mLength[i]) < smalldist)
                {
                    int ni = NXTI(i);
                    if (f[ni] < e->mLength[ni] + smalldist)
                    {
                        ni = NXTI(i);
                        if (f[ni] < e->mLength[ni] + smalldist)
                            return true;
                    }
                }
            }
            return false;
            break;
        }
        
    case CylinderVolumeType:
        {
            CylinderVolume* e = (CylinderVolume*) volume;
            float f1, f2;
            rmt::Vector v1, v2;
            
            f1 = e->mAxis.DotProduct(r); // longit dist
            float absf1 = rmt::Fabs(f1);
            v1 = f1 * e->mAxis;

            if (absf1 < e->mLength + smalldist)
            {
                v2 = r - v1;
                f2 = v2.Magnitude();
                if (rmt::Fabs(f1 - e->mLength) < smalldist && e->mFlatEnd)
                {
                    if (f2 < e->mCylinderRadius + smalldist)
                        return true; // point on the flat end of cylinder
                }
                if (rmt::Fabs(f2 - e->mCylinderRadius ) < smalldist)
                {
                    if (f1 < e->mLength + smalldist)
                        return true; // point on the side
                }
            }
            else if (!e->mFlatEnd)
            {
                f1 = f1 > 0 ? e->mLength : -e->mLength;
                v2 = f1 * e->mAxis;
                v1 = r - v2;
                f2 = v1.Magnitude();
                if (rmt::Fabs(f2 - e->mCylinderRadius ) < smalldist)
                    return true; // point on the half sphere
            }
            return false;
            break;
        }
    case SphereVolumeType:
        {
            float f = r.Magnitude();
            if (rmt::Fabs(f-volume->mSphereRadius) < smalldist) 
                return true;
            return false;
            break;
        }
    case WallVolumeType:
        {
            WallVolume* e = (WallVolume*) volume;
            float f1 = e->mNormal.DotProduct(r);
            if (rmt::Fabs(f1) < smalldist) 
                return true;
            return false;
            break;
        }
    default:
        return true;
        
    }
}
          
            
//----------------------------------------------------------------
// Some new drawing functions
//----------------------------------------------------------------

void DrawOBBoxShape(OBBoxVolume* el)
{
    tShader *defaultShader = new tShader;
    defaultShader->AddRef();
    
    rmt::Vector minCorner;
    rmt::Vector maxCorner;
    
    minCorner.x = - el->mLength[0];
    minCorner.y = - el->mLength[1];
    minCorner.z = - el->mLength[2];
    
    maxCorner.x = el->mLength[0];
    maxCorner.y = el->mLength[1];
    maxCorner.z = el->mLength[2];
    
    p3d::stack->Push();
    rmt::Matrix positionMatrix;
    positionMatrix.Identity();
    positionMatrix.Row(0) = el->mAxis[0];
    positionMatrix.Row(1) = el->mAxis[1];
    positionMatrix.Row(2) = el->mAxis[2];
    positionMatrix.Row(3) = el->mPosition;
    p3d::stack->Multiply(positionMatrix);
    
    maxCorner.Scale(sDrawVolumeShapeScale);
    minCorner.Scale(sDrawVolumeShapeScale);

    #ifndef RAD_RELEASE
        P3DDrawOrientedBox(minCorner, maxCorner, *defaultShader, el->GetColour());
    #endif
    
    p3d::stack->Pop();
    
    defaultShader->Release();
    
}

void DrawSphereShape(SphereVolume* el)
{
    tShader *defaultShader = new tShader;
    defaultShader->AddRef();
    
    #ifndef RAD_RELEASE
        P3DDrawSphere(el->GetRadius()*sDrawVolumeShapeScale, el->mPosition, *defaultShader, el->GetColour());
    #endif
    
    defaultShader->Release();
}

void DrawCylinderShape(CylinderVolume* el)
{
    
    tShader *defaultShader = new tShader;
    defaultShader->AddRef();
    
    rmt::Vector cylinderEndPoint;
    rmt::Vector cylinderStartPoint;
    rmt::Vector length = el->mAxis;
    length.Scale(el->mLength);
    
    cylinderStartPoint.Sub(el->mPosition, length);
    cylinderEndPoint.Add(el->mPosition, length);
    
    if (el->mFlatEnd)
    {
        #ifndef RAD_RELEASE
        P3DDrawCylinder(el->mCylinderRadius*sDrawVolumeShapeScale, 
            cylinderStartPoint, 
            cylinderEndPoint, 
            *defaultShader, 
            el->GetColour());
        #endif
    }
    else
    {
        #ifndef RAD_RELEASE
        P3DDrawCapsule(el->mCylinderRadius*sDrawVolumeShapeScale, 
            cylinderStartPoint, 
            cylinderEndPoint, 
            *defaultShader, 
            el->GetColour());
        #endif
    }
    
    defaultShader->Release();
}

} // sim
