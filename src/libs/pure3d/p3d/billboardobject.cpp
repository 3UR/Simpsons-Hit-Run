/*===========================================================================
    billboardQuad.cpp

    Copyright (c)2000 Radical Entertainment, Inc.
    All rights reserved.
===========================================================================*/
#include <constants/chunkids.hpp>
#include <p3d/chunkfile.hpp>
#include <p3d/utility.hpp>
#include <p3d/shader.hpp>
#include <p3d/view.hpp>
#include <p3d/camera.hpp>
#include <p3d/matrixstack.hpp>
#include <p3d/billboardobject.hpp>
#include <pddi/pddiext.hpp>

//for memcpy
#include <stdio.h>
#include <string.h>

static const int BILLBOARD_QUAD_VERSION = 2;
static const int BILLBOARD_QUAD_GROUP_VERSION = 0;
static const int BILLBOARD_DISPLAY_INFO_VERSION = 0;
static const int BILLBOARD_PERSPECTIVE_INFO_VERSION = 0;

using namespace RadicalMathLibrary;

//*****************************************************************************
// NOTE:
//  If i ever need to have camera angle and quad angle working together such
//  that the quad will fade off based camera location and camera rotation then
//  i need to do the following.
//  
//  - add two other parameters that would represent the camera source range
//  and camera edge range.  
//  
//  - compute the initial intensity based on the angle between the vector from 
//  the camera to the quad (both in world position) and the quad normal.  
//  compare the angle to the source and edge range.
//
//  - compute the final intensity by calculating the angle between the camera direction
//  and the vector from the camera to the quad (both in world position).  use the 
//  angle with the camera source and edge range to determine the fraction of the
//  initial intensity to use (ie if camera direction is equal to the vector from camera
//  to quad then the final intensity = initial intensity, otherwise final intensity = some
//  factor based on angle * initial intensity)
//*****************************************************************************

#ifdef RAD_XBOX
    static const float visibilityTestSize = 0.01f;
    static const float visibilityTestScale = 0.8f;
    unsigned int tBillboardQuadGroup::maxVisibilityTests = 0;
    tBillboardQuadGroup::VisibilityTests tBillboardQuadGroup::oldVisibilityTests;
    tBillboardQuadGroup::VisibilityTests tBillboardQuadGroup::currVisibilityTests;
#endif

    //<HACK>
    BillboardQuadManager* BillboardQuadManager::spInstance = NULL;
    bool BillboardQuadManager::sCollectPass = false;
    bool BillboardQuadManager::sEnabled = false;

//*****************************************************************************
//
// Class tBillboardQuad
//
//*****************************************************************************   
tBillboardQuad::tBillboardQuad():
    flip(false),
    billboardMode(p3dBillboardConstants::BillboardMode::ALL_AXIS),
    visible(true),
    perspective(true),
    intensity(1.0f),
    width(1.0f),
    height(1.0f),
    distance(0.0f),
    colour(255, 255, 255, 255),
    uvOffset(0.0f,0.0f),
    cutOffMode(p3dBillboardConstants::CutOffMode::NONE),
    uvOffsetRange(0.0f,0.0f),
    sourceRange(0.0f),
    edgeRange(0.0f)
{
    boundingBox.high.Set(0.0f,0.0f,0.0f);
    boundingBox.low.Set(0.0f,0.0f,0.0f);

    transform.Identity();

    //bottom left
    uv[0].u = 0.0f;
    uv[0].v = 0.0f;
    //bottom right
    uv[1].u = 0.0f;
    uv[1].v = 1.0f;
    //top right
    uv[2].u = 1.0f;
    uv[2].v = 1.0f;
    //top left
    uv[3].u = 1.0f;
    uv[3].v = 0.0f;
    //texture size
    uv[3].u = 1.0f;
    uv[3].v = 1.0f;
}

tBillboardQuad::~tBillboardQuad()
{
}



void 
tBillboardQuad::Display(pddiPrimStream* stream, const Matrix& world, const Matrix& camera, const Matrix& worldToCamera, float intensityBias)
{
    if (!visible)
    {
        return;
    }

    //Vector  v[4];
    //Vector2 t[4];
    //tColour displayColour;

    if (billboardMode == p3dBillboardConstants::BillboardMode::NO_AXIS)
    {        
        Matrix objectToWorld;
        objectToWorld.Mult(transform, world);

        v[0].Set(-width,-height,0.0f);
        v[1].Set(width,-height,0.0f);
        v[2].Set(width,height,0.0f);
        v[3].Set(-width,height,0.0f);

        v[0].Transform(objectToWorld);
        v[1].Transform(objectToWorld);
        v[2].Transform(objectToWorld);
        v[3].Transform(objectToWorld);

        if (distance != 0.0f)
        {
            Vector extrudeDir(0.0f,0.0f,-distance);
            extrudeDir.Rotate(objectToWorld);

            v[0].Add(extrudeDir);
            v[1].Add(extrudeDir);
            v[2].Add(extrudeDir);
            v[3].Add(extrudeDir);
        }

        v[0].Transform(worldToCamera);
        v[1].Transform(worldToCamera);
        v[2].Transform(worldToCamera);
        v[3].Transform(worldToCamera);
    }
    else if (billboardMode == p3dBillboardConstants::BillboardMode::ALL_AXIS)
    {        
        Vector cameraPos = camera.Row(3);
        Vector worldPos = transform.Row(3);
        worldPos.Transform(world);

        if (distance != 0.0f)
        {
            Vector extrudeDir;
            extrudeDir.Sub(cameraPos,worldPos);
            extrudeDir.NormalizeSafe();
            extrudeDir.Scale(distance);
            worldPos.Add(extrudeDir);
        }

        Vector currPos;
        currPos.x = worldToCamera.m[0][0]*worldPos.x + worldToCamera.m[1][0]*worldPos.y + worldToCamera.m[2][0]*worldPos.z + worldToCamera.m[3][0];
        currPos.y = worldToCamera.m[0][1]*worldPos.x + worldToCamera.m[1][1]*worldPos.y + worldToCamera.m[2][1]*worldPos.z + worldToCamera.m[3][1];
        currPos.z = worldToCamera.m[0][2]*worldPos.x + worldToCamera.m[1][2]*worldPos.y + worldToCamera.m[2][2]*worldPos.z + worldToCamera.m[3][2];

        float x = width;
        float y = height;

        if (!perspective)
        {
            x *= currPos.z;
            y *= currPos.z;
        }

        v[0].Set(currPos.x-x, currPos.y-y, currPos.z);
        v[1].Set(currPos.x+x, currPos.y-y, currPos.z);
        v[2].Set(currPos.x+x, currPos.y+y, currPos.z);
        v[3].Set(currPos.x-x, currPos.y+y, currPos.z);
    }
    else
    {       
        Vector cameraPos = camera.Row(3);
        Vector worldPos = transform.Row(3);
        Vector heading;
        Matrix objectToWorld;
        
        objectToWorld.Identity();

        switch (billboardMode)
        {
            case p3dBillboardConstants::BillboardMode::X_AXIS:
            {
                worldPos.Transform(world);
                heading.x = -cameraPos.x + worldPos.x;
                heading.y = -cameraPos.y + worldPos.y;
                heading.z = -cameraPos.z + worldPos.z;
                objectToWorld.FillHeadingYZ(heading);
                break;
            }
            case p3dBillboardConstants::BillboardMode::Y_AXIS:
            {
                worldPos.Transform(world);
                heading.x = -cameraPos.x + worldPos.x;
                heading.y = -cameraPos.y + worldPos.y;
                heading.z = -cameraPos.z + worldPos.z;
                objectToWorld.FillHeadingXZ(heading);
                break;
            }
            case p3dBillboardConstants::BillboardMode::LOCAL_X_AXIS:
            {
                Vector up(0.0f,1.0f,0.0f);
                Vector normal(0.0f,0.0f,-1.0f);

                objectToWorld.Mult(transform,world);                
                up.Rotate(objectToWorld);
                normal.Rotate(objectToWorld);

                worldPos = objectToWorld.Row(3);
                heading.x = cameraPos.x - worldPos.x;
                heading.y = cameraPos.y - worldPos.y;
                heading.z = cameraPos.z - worldPos.z;

                Vector projectedUp;
                Vector projectedNormal;
                Vector projectedHeading;

                projectedUp.Scale(heading.Dot(up)/up.Dot(up),up);
                projectedNormal.Scale(heading.Dot(normal)/normal.Dot(normal),normal);
                projectedHeading.Add(projectedUp,projectedNormal);
                projectedHeading.Normalize();

                objectToWorld.Identity();
                if (up.Dot(projectedHeading) > 0.0f)
                {
                    objectToWorld.FillRotateX(ACos(normal.Dot(projectedHeading)));
                }
                else
                {
                    objectToWorld.FillRotateX(-ACos(normal.Dot(projectedHeading)));
                }                
                objectToWorld.Mult(transform);
                objectToWorld.Mult(world);
                break;
            }
            case p3dBillboardConstants::BillboardMode::LOCAL_Y_AXIS:
            {
                Vector right(1.0f,0.0f,0.0f);
                Vector normal(0.0f,0.0f,-1.0f);

                objectToWorld.Mult(transform,world);                
                right.Rotate(objectToWorld);
                normal.Rotate(objectToWorld);

                worldPos = objectToWorld.Row(3);
                heading.x = cameraPos.x - worldPos.x;
                heading.y = cameraPos.y - worldPos.y;
                heading.z = cameraPos.z - worldPos.z;

                Vector projectedRight;
                Vector projectedNormal;
                Vector projectedHeading;

                projectedRight.Scale(heading.Dot(right)/right.Dot(right),right);
                projectedNormal.Scale(heading.Dot(normal)/normal.Dot(normal),normal);
                projectedHeading.Add(projectedRight,projectedNormal);
                projectedHeading.Normalize();

                normal.Normalize();
                objectToWorld.Identity();
                if (right.Dot(projectedHeading) > 0.0f)
                {
                    objectToWorld.FillRotateY(-ACos(normal.Dot(projectedHeading)));
                }
                else
                {
                    objectToWorld.FillRotateY(ACos(normal.Dot(projectedHeading)));
                }                
                objectToWorld.Mult(transform);
                objectToWorld.Mult(world);
                break;
            }
            default:
            {
                return;
                break;
            }
        }

        v[0].Set(-width,-height,0.0f);
        v[1].Set(width,-height,0.0f);
        v[2].Set(width,height,0.0f);
        v[3].Set(-width,height,0.0f);

        v[0].Rotate(objectToWorld);
        v[1].Rotate(objectToWorld);
        v[2].Rotate(objectToWorld);
        v[3].Rotate(objectToWorld);

        v[0].Add(worldPos);
        v[1].Add(worldPos);
        v[2].Add(worldPos);
        v[3].Add(worldPos);
        
        if (distance != 0.0f)
        {
            Vector extrudeDir(0.0f,0.0f,-distance);
            extrudeDir.Rotate(objectToWorld);

            v[0].Add(extrudeDir);
            v[1].Add(extrudeDir);
            v[2].Add(extrudeDir);
            v[3].Add(extrudeDir);
        }

        v[0].Transform(worldToCamera);
        v[1].Transform(worldToCamera);
        v[2].Transform(worldToCamera);
        v[3].Transform(worldToCamera);
    }

    float tempIntensity = intensity * intensityBias;
    if (tempIntensity!=1.0f)
    {
        displayColour.SetRed(FtoL(LtoF(colour.Red())*tempIntensity));
        displayColour.SetGreen(FtoL(LtoF(colour.Green())*tempIntensity));
        displayColour.SetBlue(FtoL(LtoF(colour.Blue())*tempIntensity));
        displayColour.SetAlpha(FtoL(LtoF(colour.Alpha())*tempIntensity));
    }
    else
    {
        displayColour = colour;
    }

    t[0].u = uv[0].u+uvOffset.u;
    t[0].v = uv[0].v+uvOffset.v;
    t[1].u = uv[1].u+uvOffset.u;
    t[1].v = uv[1].v+uvOffset.v;
    t[2].u = uv[2].u+uvOffset.u;
    t[2].v = uv[2].v+uvOffset.v;
    t[3].u = uv[3].u+uvOffset.u;
    t[3].v = uv[3].v+uvOffset.v;
        
    if( BillboardQuadManager::sEnabled )
        return;

    if ((!flip)||(billboardMode != p3dBillboardConstants::BillboardMode::NO_AXIS))
    {
        //bottom left
        stream->Colour(displayColour);
        stream->UV(t[0].u, t[0].v);
        stream->Coord(v[0].x, v[0].y, v[0].z);
    
        //top right
        stream->Colour(displayColour);
        stream->UV(t[2].u, t[2].v);
        stream->Coord(v[2].x, v[2].y, v[2].z);

        //bottom right
        stream->Colour(displayColour);
        stream->UV(t[1].u, t[1].v);
        stream->Coord(v[1].x, v[1].y, v[1].z);
    
        // top left
        stream->Colour(displayColour);
        stream->UV(t[3].u, t[3].v);
        stream->Coord(v[3].x, v[3].y, v[3].z);

        // top right
        stream->Colour(displayColour);
        stream->UV(t[2].u, t[2].v);
        stream->Coord(v[2].x, v[2].y, v[2].z);

        // bottom left
        stream->Colour(displayColour);
        stream->UV(t[0].u, t[0].v);
        stream->Coord(v[0].x, v[0].y, v[0].z);
    }
    else
    {        
        //bottom left
        stream->Colour(displayColour);
        stream->UV(t[0].u, t[0].v);
        stream->Coord(v[0].x, v[0].y, v[0].z);
    
        //bottom right
        stream->Colour(displayColour);
        stream->UV(t[1].u, t[1].v);
        stream->Coord(v[1].x, v[1].y, v[1].z);

        //top right
        stream->Colour(displayColour);
        stream->UV(t[2].u, t[2].v);
        stream->Coord(v[2].x, v[2].y, v[2].z);
    
        // bottom left
        stream->Colour(displayColour);
        stream->UV(t[0].u, t[0].v);
        stream->Coord(v[0].x, v[0].y, v[0].z);

        // top right
        stream->Colour(displayColour);
        stream->UV(t[2].u, t[2].v);
        stream->Coord(v[2].x, v[2].y, v[2].z);

        // top left
        stream->Colour(displayColour);
        stream->UV(t[3].u, t[3].v);
        stream->Coord(v[3].x, v[3].y, v[3].z);
    }
}

void BakedBillboardQuad::Display(pddiPrimStream* stream)
{
    //displayColour.SetAlpha(255);
    //displayColour.SetRed(255);
    //displayColour.SetGreen(255);
    //displayColour.SetBlue(255);
    if ((!flip)||(billboardMode != p3dBillboardConstants::BillboardMode::NO_AXIS))
    {
        //bottom left
        stream->Colour(displayColour);
        stream->UV(t[0].u, t[0].v);
        stream->Coord(v[0].x, v[0].y, v[0].z);

        //top right
        stream->Colour(displayColour);
        stream->UV(t[2].u, t[2].v);
        stream->Coord(v[2].x, v[2].y, v[2].z);

        //bottom right
        stream->Colour(displayColour);
        stream->UV(t[1].u, t[1].v);
        stream->Coord(v[1].x, v[1].y, v[1].z);

        // top left
        stream->Colour(displayColour);
        stream->UV(t[3].u, t[3].v);
        stream->Coord(v[3].x, v[3].y, v[3].z);

        // top right
        stream->Colour(displayColour);
        stream->UV(t[2].u, t[2].v);
        stream->Coord(v[2].x, v[2].y, v[2].z);

        // bottom left
        stream->Colour(displayColour);
        stream->UV(t[0].u, t[0].v);
        stream->Coord(v[0].x, v[0].y, v[0].z);
    }
    else
    {        
        //bottom left
        stream->Colour(displayColour);
        stream->UV(t[0].u, t[0].v);
        stream->Coord(v[0].x, v[0].y, v[0].z);

        //bottom right
        stream->Colour(displayColour);
        stream->UV(t[1].u, t[1].v);
        stream->Coord(v[1].x, v[1].y, v[1].z);

        //top right
        stream->Colour(displayColour);
        stream->UV(t[2].u, t[2].v);
        stream->Coord(v[2].x, v[2].y, v[2].z);

        // bottom left
        stream->Colour(displayColour);
        stream->UV(t[0].u, t[0].v);
        stream->Coord(v[0].x, v[0].y, v[0].z);

        // top right
        stream->Colour(displayColour);
        stream->UV(t[2].u, t[2].v);
        stream->Coord(v[2].x, v[2].y, v[2].z);

        // top left
        stream->Colour(displayColour);
        stream->UV(t[3].u, t[3].v);
        stream->Coord(v[3].x, v[3].y, v[3].z);
    }
}
//THIS FUNCTION MUST BE OPTIMIZED!!!
void 
tBillboardQuad::Calculate()
{
    tCamera* camera = p3d::context->GetView()->GetCamera();
    if (!camera)
    {
        intensity = 0.0f;
        return;
    }
 
    if (cutOffMode == p3dBillboardConstants::CutOffMode::NONE)
    {
        intensity = 1.0f;
        return;
    }

    // calculate normal of quad and the direction from the quad to the camera
    Matrix worldMatrix = *(p3d::context->GetWorldMatrix());
    Matrix cameraMatrix = p3d::context->GetView()->GetCamera()->GetCameraToWorldMatrix();
    Matrix worldToCamera = p3d::context->GetView()->GetCamera()->GetWorldToCameraMatrix();
    Matrix objectToWorld;

    Vector objectPos;
    Vector cameraPos;
    Vector objectNormal;
    Vector cameraNormal;
    Vector objectDirection;
    Vector cameraDirection;
    float  dotProduct;
    float  angle;

    objectToWorld.Mult(transform,worldMatrix);
    cameraNormal.Set(0.0f,0.0f,1.0f);
    objectNormal.Set(0.0f,0.0f,-1.0f);
    objectPos = objectToWorld.Row(3);
    cameraPos = cameraMatrix.Row(3);
    objectNormal.Rotate(objectToWorld);
    cameraNormal.Rotate(cameraMatrix);
    objectDirection.Sub(cameraPos,objectPos);
    cameraDirection.Set(-objectDirection.x,-objectDirection.y,-objectDirection.z);

    // determine if the billboard should be flipped
    dotProduct = objectDirection.Dot(objectNormal);
    if (dotProduct <= 0.0f)
    {
        flip = true;
        if (cutOffMode == p3dBillboardConstants::CutOffMode::SINGLE_SIDED)
        {
            intensity = 0.0f;
            return;
        }        
    }
    else
    {
        flip = false;
    }
  
    // project the normal and direction along the axis that matter
    switch (billboardMode)
    {
        case p3dBillboardConstants::BillboardMode::ALL_AXIS:
        case p3dBillboardConstants::BillboardMode::NO_AXIS:
        {
            break;
        }
        case p3dBillboardConstants::BillboardMode::X_AXIS:
        case p3dBillboardConstants::BillboardMode::LOCAL_X_AXIS:
        {
            objectNormal.x = 0.0f;
            objectDirection.x = 0.0f;
            cameraNormal.x = 0.0f;
            cameraDirection.x = 0.0f;
            break;
        }
        case p3dBillboardConstants::BillboardMode::Y_AXIS:
        case p3dBillboardConstants::BillboardMode::LOCAL_Y_AXIS:
        {
            objectNormal.y = 0.0f;
            objectDirection.y = 0.0f;
            cameraNormal.y = 0.0f;
            cameraDirection.y = 0.0f;
            break;
        }
        default:
        {
            intensity = 0.0f;
            return;
            break;
        }
    }

    // calculate the colour intensity based on source and edge ranges
    objectNormal.Normalize();
    objectDirection.Normalize();
    cameraNormal.Normalize();
    cameraDirection.Normalize();

    if (cameraNormal.Equals(cameraDirection))
    {
        if (objectNormal.Equals(objectDirection))
        {
            intensity = 1.0f;
        }
        else
        {
            dotProduct = objectDirection.Dot(objectNormal);
            angle = Fabs(ACos(Fabs(dotProduct)));
            if (angle < sourceRange)
            {
                intensity = 1.0f;
            }
            else
            {
                angle -= sourceRange;
                if (angle < edgeRange)
                {
                    intensity = 1.0f - (angle/edgeRange);                    
                }
                else
                {
                    intensity = 0.0f;
                }
            }
        }
    }
    else
    {
        if ((sourceRange+edgeRange) > 0.0f)
        {
            float cameraIntensity = 0.0f;
            float objectIntensity = 0.0f;

            dotProduct = objectDirection.Dot(objectNormal);
            angle = Fabs(ACos(Fabs(dotProduct)));
            if (angle < sourceRange)
            {
                objectIntensity = 1.0f;
            }
            else
            {
                angle -= sourceRange;
                if (angle < edgeRange)
                {
                    objectIntensity = 1.0f - (angle/edgeRange);                    
                }
                else
                {
                    objectIntensity = 0.0f;
                }
            }

            dotProduct = cameraDirection.Dot(cameraNormal);
            angle = Fabs(ACos(Fabs(dotProduct)));
            if (angle < sourceRange)
            {
                cameraIntensity = 1.0f;
            }
            else
            {
                angle -= sourceRange;
                if (angle < edgeRange)
                {
                    cameraIntensity = 1.0f - (angle/edgeRange);                    
                }
                else
                {
                    cameraIntensity = 0.0f;
                }
            }

            intensity = objectIntensity * cameraIntensity;
        }
        else
        {
            intensity = 1.0f;
        }
    }
}

//*****************************************************************************
//
// Class tCamRelativeBillboardQuad
//
//*****************************************************************************

tCamRelativeBillboardQuad::tCamRelativeBillboardQuad():
maxSize( 1.0f ),
minSize( 1.0f ),
nearDist( 1.0f )
{
    
}

tCamRelativeBillboardQuad::~tCamRelativeBillboardQuad()
{

}

void 
tCamRelativeBillboardQuad::Set( float in_minSize, float in_maxSize, float in_nearDist, float in_farDist )
{
    maxSize = in_maxSize;
    minSize = in_minSize;
    nearDist = in_nearDist;
    if ( rmt::Fabs( in_farDist - nearDist ) < 0.01f )
        return;

    m = ( maxSize - minSize ) / ( in_farDist - nearDist );    
}

void
tCamRelativeBillboardQuad::Display(pddiPrimStream* stream, const rmt::Matrix& world, const rmt::Matrix& camera, const rmt::Matrix& worldToCamera, float intensityBias)
{

    if (!visible)
    {
        return;
    }

    //Vector  v[4];
    //Vector2 t[4];
    //tColour displayColour;

    if (billboardMode == p3dBillboardConstants::BillboardMode::NO_AXIS)
    {        
        Matrix objectToWorld;
        objectToWorld.Mult(transform, world);

        v[0].Set(-width,-height,0.0f);
        v[1].Set(width,-height,0.0f);
        v[2].Set(width,height,0.0f);
        v[3].Set(-width,height,0.0f);

        v[0].Transform(objectToWorld);
        v[1].Transform(objectToWorld);
        v[2].Transform(objectToWorld);
        v[3].Transform(objectToWorld);

        if (distance != 0.0f)
        {
            Vector extrudeDir(0.0f,0.0f,-distance);
            extrudeDir.Rotate(objectToWorld);

            v[0].Add(extrudeDir);
            v[1].Add(extrudeDir);
            v[2].Add(extrudeDir);
            v[3].Add(extrudeDir);
        }

        v[0].Transform(worldToCamera);
        v[1].Transform(worldToCamera);
        v[2].Transform(worldToCamera);
        v[3].Transform(worldToCamera);
    }
    else if (billboardMode == p3dBillboardConstants::BillboardMode::ALL_AXIS)
    {        
        Vector worldPos = transform.Row(3);
        float billboardScale = CalcScale( camera, world );

        float x = width * billboardScale;
        float y = height * billboardScale;

        // Lets move the billboard up to accommodate the expansion
        worldPos.y += y * 0.5f;


        Vector currPos;
        currPos.x = worldToCamera.m[0][0]*worldPos.x + worldToCamera.m[1][0]*worldPos.y + worldToCamera.m[2][0]*worldPos.z + worldToCamera.m[3][0];
        currPos.y = worldToCamera.m[0][1]*worldPos.x + worldToCamera.m[1][1]*worldPos.y + worldToCamera.m[2][1]*worldPos.z + worldToCamera.m[3][1];
        currPos.z = worldToCamera.m[0][2]*worldPos.x + worldToCamera.m[1][2]*worldPos.y + worldToCamera.m[2][2]*worldPos.z + worldToCamera.m[3][2];





        if (!perspective)
        {
            x *= currPos.z;
            y *= currPos.z;
        }

        v[0].Set(currPos.x-x, currPos.y-y, currPos.z);
        v[1].Set(currPos.x+x, currPos.y-y, currPos.z);
        v[2].Set(currPos.x+x, currPos.y+y, currPos.z);
        v[3].Set(currPos.x-x, currPos.y+y, currPos.z);
    }
    else
    {       
        Vector cameraPos = camera.Row(3);
        Vector worldPos = transform.Row(3);
        Vector heading;
        Matrix objectToWorld;

        objectToWorld.Identity();

        switch (billboardMode)
        {
        case p3dBillboardConstants::BillboardMode::X_AXIS:
            {
                worldPos.Transform(world);
                heading.x = -cameraPos.x + worldPos.x;
                heading.y = -cameraPos.y + worldPos.y;
                heading.z = -cameraPos.z + worldPos.z;
                objectToWorld.FillHeadingYZ(heading);
                break;
            }
        case p3dBillboardConstants::BillboardMode::Y_AXIS:
            {
                worldPos.Transform(world);
                heading.x = -cameraPos.x + worldPos.x;
                heading.y = -cameraPos.y + worldPos.y;
                heading.z = -cameraPos.z + worldPos.z;
                objectToWorld.FillHeadingXZ(heading);
                break;
            }
        case p3dBillboardConstants::BillboardMode::LOCAL_X_AXIS:
            {
                Vector up(0.0f,1.0f,0.0f);
                Vector normal(0.0f,0.0f,-1.0f);

                objectToWorld.Mult(transform,world);                
                up.Rotate(objectToWorld);
                normal.Rotate(objectToWorld);

                worldPos = objectToWorld.Row(3);
                heading.x = cameraPos.x - worldPos.x;
                heading.y = cameraPos.y - worldPos.y;
                heading.z = cameraPos.z - worldPos.z;

                Vector projectedUp;
                Vector projectedNormal;
                Vector projectedHeading;

                projectedUp.Scale(heading.Dot(up)/up.Dot(up),up);
                projectedNormal.Scale(heading.Dot(normal)/normal.Dot(normal),normal);
                projectedHeading.Add(projectedUp,projectedNormal);
                projectedHeading.Normalize();

                objectToWorld.Identity();
                if (up.Dot(projectedHeading) > 0.0f)
                {
                    objectToWorld.FillRotateX(ACos(normal.Dot(projectedHeading)));
                }
                else
                {
                    objectToWorld.FillRotateX(-ACos(normal.Dot(projectedHeading)));
                }                
                objectToWorld.Mult(transform);
                objectToWorld.Mult(world);
                break;
            }
        case p3dBillboardConstants::BillboardMode::LOCAL_Y_AXIS:
            {
                Vector right(1.0f,0.0f,0.0f);
                Vector normal(0.0f,0.0f,-1.0f);

                objectToWorld.Mult(transform,world);                
                right.Rotate(objectToWorld);
                normal.Rotate(objectToWorld);

                worldPos = objectToWorld.Row(3);
                heading.x = cameraPos.x - worldPos.x;
                heading.y = cameraPos.y - worldPos.y;
                heading.z = cameraPos.z - worldPos.z;

                Vector projectedRight;
                Vector projectedNormal;
                Vector projectedHeading;

                projectedRight.Scale(heading.Dot(right)/right.Dot(right),right);
                projectedNormal.Scale(heading.Dot(normal)/normal.Dot(normal),normal);
                projectedHeading.Add(projectedRight,projectedNormal);
                projectedHeading.Normalize();

                objectToWorld.Identity();
                if (right.Dot(projectedHeading) > 0.0f)
                {
                    objectToWorld.FillRotateY(-ACos(normal.Dot(projectedHeading)));
                }
                else
                {
                    objectToWorld.FillRotateY(ACos(normal.Dot(projectedHeading)));
                }                
                objectToWorld.Mult(transform);
                objectToWorld.Mult(world);
                break;
            }
        default:
            {
                return;
                break;
            }
        }
        // Apply billboard scaling
        float billboardScale = CalcScale( camera, world );

        float scaledWidth = width * billboardScale;
        float scaledHeight= height * billboardScale;

        v[0].Set(-scaledWidth,-scaledHeight,0.0f);
        v[1].Set(scaledWidth,-scaledHeight,0.0f);
        v[2].Set(scaledWidth,scaledHeight,0.0f);
        v[3].Set(-scaledWidth,scaledHeight,0.0f);

        // Lets move the billboard up to accommodate the expansion
        float halfHeight = height * 0.5f;
        // What is the halfheight of this thing when scaled?
        float halfHeightScaled = halfHeight * billboardScale;
        // Move it up
        worldPos.y += scaledHeight * 0.5f;
        
        v[0].Rotate(objectToWorld);
        v[1].Rotate(objectToWorld);
        v[2].Rotate(objectToWorld);
        v[3].Rotate(objectToWorld);

        v[0].Add(worldPos);
        v[1].Add(worldPos);
        v[2].Add(worldPos);
        v[3].Add(worldPos);

        if (distance != 0.0f)
        {
            Vector extrudeDir(0.0f,0.0f,-distance);
            extrudeDir.Rotate(objectToWorld);

            v[0].Add(extrudeDir);
            v[1].Add(extrudeDir);
            v[2].Add(extrudeDir);
            v[3].Add(extrudeDir);
        }

        v[0].Transform(worldToCamera);
        v[1].Transform(worldToCamera);
        v[2].Transform(worldToCamera);
        v[3].Transform(worldToCamera);
    }

    float tempIntensity = intensity * intensityBias;
    if (tempIntensity!=1.0f)
    {
        displayColour.SetRed(FtoL(LtoF(colour.Red())*tempIntensity));
        displayColour.SetGreen(FtoL(LtoF(colour.Green())*tempIntensity));
        displayColour.SetBlue(FtoL(LtoF(colour.Blue())*tempIntensity));
        displayColour.SetAlpha(FtoL(LtoF(colour.Alpha())*tempIntensity));
    }
    else
    {
        displayColour = colour;
    }

    t[0].u = uv[0].u+uvOffset.u;
    t[0].v = uv[0].v+uvOffset.v;
    t[1].u = uv[1].u+uvOffset.u;
    t[1].v = uv[1].v+uvOffset.v;
    t[2].u = uv[2].u+uvOffset.u;
    t[2].v = uv[2].v+uvOffset.v;
    t[3].u = uv[3].u+uvOffset.u;
    t[3].v = uv[3].v+uvOffset.v;

    if( BillboardQuadManager::sEnabled )
        return;

    if ((!flip)||(billboardMode != p3dBillboardConstants::BillboardMode::NO_AXIS))
    {
        //bottom left
        stream->Colour(displayColour);
        stream->UV(t[0].u, t[0].v);
        stream->Coord(v[0].x, v[0].y, v[0].z);

        //top right
        stream->Colour(displayColour);
        stream->UV(t[2].u, t[2].v);
        stream->Coord(v[2].x, v[2].y, v[2].z);

        //bottom right
        stream->Colour(displayColour);
        stream->UV(t[1].u, t[1].v);
        stream->Coord(v[1].x, v[1].y, v[1].z);

        // top left
        stream->Colour(displayColour);
        stream->UV(t[3].u, t[3].v);
        stream->Coord(v[3].x, v[3].y, v[3].z);

        // top right
        stream->Colour(displayColour);
        stream->UV(t[2].u, t[2].v);
        stream->Coord(v[2].x, v[2].y, v[2].z);

        // bottom left
        stream->Colour(displayColour);
        stream->UV(t[0].u, t[0].v);
        stream->Coord(v[0].x, v[0].y, v[0].z);
    }
    else
    {        
        //bottom left
        stream->Colour(displayColour);
        stream->UV(t[0].u, t[0].v);
        stream->Coord(v[0].x, v[0].y, v[0].z);

        //bottom right
        stream->Colour(displayColour);
        stream->UV(t[1].u, t[1].v);
        stream->Coord(v[1].x, v[1].y, v[1].z);

        //top right
        stream->Colour(displayColour);
        stream->UV(t[2].u, t[2].v);
        stream->Coord(v[2].x, v[2].y, v[2].z);

        // bottom left
        stream->Colour(displayColour);
        stream->UV(t[0].u, t[0].v);
        stream->Coord(v[0].x, v[0].y, v[0].z);

        // top right
        stream->Colour(displayColour);
        stream->UV(t[2].u, t[2].v);
        stream->Coord(v[2].x, v[2].y, v[2].z);

        // top left
        stream->Colour(displayColour);
        stream->UV(t[3].u, t[3].v);
        stream->Coord(v[3].x, v[3].y, v[3].z);
    }
}


float tCamRelativeBillboardQuad::CalcScale( const rmt::Matrix& camera, const rmt::Matrix& world )const
{
    Vector cameraPos = camera.Row(3);
    Vector worldPos = transform.Row(3);
    worldPos.Transform(world);

    if (distance != 0.0f)
    {
        Vector extrudeDir;
        extrudeDir.Sub(cameraPos,worldPos);
        extrudeDir.NormalizeSafe();
        extrudeDir.Scale(distance);
        worldPos.Add(extrudeDir);
    }
    // Determine the distance to the camera
    rmt::Vector camPosition;
    p3d::context->GetView()->GetCamera()->GetWorldPosition( &camPosition );
    float distToBillboard = (camPosition - worldPos).Magnitude();
    float billboardScale = m * ( distToBillboard - nearDist ) + minSize;
    billboardScale = rmt::Clamp( billboardScale, minSize, maxSize );

    return billboardScale;
}


int tBillboardQuad::DisplayOcclusionSprite(const rmt::Matrix& world, const rmt::Matrix& camera, const rmt::Matrix& worldToCamera)
{
    return 0;
}



//*****************************************************************************
//
// Class tBillboardQuadGroup
//
//*****************************************************************************
tBillboardQuadGroup::tBillboardQuadGroup():
    shader(NULL),
    numQuads(0),
    zTest(false),
    zWrite(false),    
    occlusionMode(0),
    intensityBias(1.0f)
{
#ifdef RAD_XBOX
    numPixelsVisible = 1;
    maxPixelsVisible = 1;
#endif RAD_XBOX 
    boundingBox.high.Set(0.0f,0.0f,0.0f);
    boundingBox.low.Set(0.0f,0.0f,0.0f);
}

tBillboardQuadGroup::~tBillboardQuadGroup()
{
    tRefCounted::Release(shader);
    for (int i = 0; i < numQuads; i++)
    {
        tRefCounted::Release(quads[i]);
    }
}

void 
tBillboardQuadGroup::SetShader(tShader* mat)
{
    tRefCounted::Assign(shader, mat);
    if (shader)
    {
        shader->SetInt(PDDI_SP_UVMODE,PDDI_UV_TILE);
    }
}

tBillboardQuad* 
tBillboardQuadGroup::FindQuadByUID(tUID id)
{
    for (int i = 0; i < numQuads; i++)
    {
        if (quads[i]->GetUID()==id)
        {
            return quads[i];
        }
    }
    return NULL;
}

tBillboardQuad* 
tBillboardQuadGroup::FindQuadByName(const char* name)
{
    return FindQuadByUID(tName::MakeUID(name));
}

void 
tBillboardQuadGroup::Display()
{
    if ( !BillboardQuadManager::sEnabled )
    {
        Matrix world = *(p3d::context->GetWorldMatrix());
        Matrix camera = p3d::context->GetView()->GetCamera()->GetCameraToWorldMatrix();
        Matrix worldToCamera = p3d::context->GetView()->GetCamera()->GetWorldToCameraMatrix();

#ifdef RAD_XBOX
        if ( occlusionMode )
        {
            if ( currVisibilityTests.numTests < maxVisibilityTests )
            {
                tRefCounted::Assign( currVisibilityTests.tests[currVisibilityTests.numTests], this );
                currVisibilityTests.numTests++;
                visibilityPosition = world.Row(3);
            }
        }
        if ( numPixelsVisible == 0 )
        {
            return;
        }
        float visIntensityBias = rmt::Clamp( static_cast<float>( numPixelsVisible ) / static_cast<float>( maxPixelsVisible ), 0.0f, 1.0f );
        // Apply the set intensity bias to the calculated one from the visibility test
        intensityBias *= visIntensityBias;
#endif

        // check to see if any of the quads are visible
        int numVisibleQuads = 0;
        int quadIndex = 0;
        for (quadIndex = 0; quadIndex<numQuads; quadIndex++)
        {
            if((quads[quadIndex]->GetVisibility() == true) && 
                (quads[quadIndex]->GetColour().Alpha() != 0) && 
                !((quads[quadIndex]->GetColour().Red() == 0) && 
                  (quads[quadIndex]->GetColour().Green() == 0) &&
                  (quads[quadIndex]->GetColour().Blue()  == 0)) )
            {
                quads[quadIndex]->Calculate();
                numVisibleQuads++;
            }
        }

        if (numVisibleQuads == 0)
        {
            return;
        }

        // turn z-writing off
        bool origZWrite = p3d::pddi->GetZWrite();
        if (origZWrite!=zWrite)
        {
            p3d::pddi->SetZWrite(zWrite);
        }

        // turn z compare to always
        pddiCompareMode origZCompare = p3d::pddi->GetZCompare();
        if ( ( zTest == false ) || ( occlusionMode ) )
        {
            if (origZCompare!=PDDI_COMPARE_ALWAYS)
            {
                p3d::pddi->SetZCompare(PDDI_COMPARE_ALWAYS);
            }
        }

        p3d::stack->PushIdentity();
        pddiPrimStream* stream = p3d::pddi->BeginPrims(shader->GetShader(), PDDI_PRIM_TRIANGLES, PDDI_V_CT, numVisibleQuads*6);

        for (int i = 0; i < numQuads; i++)
        {
            if((quads[i]->GetVisibility() == true) && 
                (quads[i]->GetColour().Alpha() != 0) && 
                !((quads[i]->GetColour().Red() == 0) && 
                  (quads[i]->GetColour().Green() == 0) &&
                  (quads[i]->GetColour().Blue()  == 0)) )

            quads[i]->Display(stream,world,camera,worldToCamera,intensityBias);
        }

        p3d::pddi->EndPrims(stream);   

        p3d::stack->Pop();   

        // restore original z-compare mode
        if ( ( zTest == false ) || ( occlusionMode ) )
        {
            if (origZCompare!=PDDI_COMPARE_ALWAYS)
            {
                p3d::pddi->SetZCompare(origZCompare);
            }
        }

        // restore z-writing (if previously enabled)
        if (origZWrite!=zWrite)
        {
            p3d::pddi->SetZWrite(origZWrite);
        }

        return;
    }

    if(BillboardQuadManager::sCollectPass)
    {
        Matrix world = *(p3d::context->GetWorldMatrix());
        Matrix camera = p3d::context->GetView()->GetCamera()->GetCameraToWorldMatrix();
        Matrix worldToCamera = p3d::context->GetView()->GetCamera()->GetWorldToCameraMatrix();

        if(GetUID()==tName::MakeUID("groundglowShape"))
            return;

#ifdef RAD_XBOX
        if ( occlusionMode )
        {
            if ( currVisibilityTests.numTests < maxVisibilityTests )
            {
                tRefCounted::Assign( currVisibilityTests.tests[currVisibilityTests.numTests], this );
                currVisibilityTests.numTests++;
                visibilityPosition = world.Row(3);
            }
        }
        if ( numPixelsVisible == 0 )
        {
            return;
        }
        float intensityBias = rmt::Clamp( static_cast<float>( numPixelsVisible ) / static_cast<float>( maxPixelsVisible ), 0.0f, 1.0f );
#endif

        // check to see if any of the quads are visible
        int numVisibleQuads = 0;
        int quadIndex = 0;
        for (quadIndex = 0; quadIndex<numQuads; quadIndex++)
        {
            if((quads[quadIndex]->GetVisibility() == true) && 
                (quads[quadIndex]->GetColour().Alpha() != 0) && 
                !((quads[quadIndex]->GetColour().Red() == 0) && 
                  (quads[quadIndex]->GetColour().Green() == 0) &&
                  (quads[quadIndex]->GetColour().Blue()  == 0)) )
            {
                quads[quadIndex]->Calculate();
                numVisibleQuads++;
            }
        }

        if (numVisibleQuads == 0)
        {
            return;
        }

        p3d::stack->PushIdentity();

        //pddiPrimStream* stream = p3d::pddi->BeginPrims(shader->GetShader(), PDDI_PRIM_TRIANGLES, PDDI_V_CT, numVisibleQuads*6);

        for (int i = 0; i < numQuads; i++)
        {
            if((quads[i]->GetVisibility() == true) && 
                (quads[i]->GetColour().Alpha() != 0) && 
                !((quads[i]->GetColour().Red() == 0) && 
                  (quads[i]->GetColour().Green() == 0) &&
                  (quads[i]->GetColour().Blue()  == 0)) )
            {
#ifdef RAD_XBOX
                quads[i]->Display(NULL,world,camera,worldToCamera,intensityBias);
#else
                quads[i]->Display(NULL,world,camera,worldToCamera,1.0f);
#endif
            }
        }

        //p3d::pddi->EndPrims(stream);   

        p3d::stack->Pop();   

        GetBillboardQuadManager()->Add(this);
        return;
    }
    else
    {
        // turn z-writing off
        bool origZWrite = p3d::pddi->GetZWrite();
        if (origZWrite!=zWrite)
        {
            p3d::pddi->SetZWrite(zWrite);
        }

        // turn z compare to always
        pddiCompareMode origZCompare = p3d::pddi->GetZCompare();
        if (!zTest)
        {
            if (origZCompare!=PDDI_COMPARE_ALWAYS)
            {
                p3d::pddi->SetZCompare(PDDI_COMPARE_ALWAYS);
            }
        }

        p3d::stack->PushIdentity();

        pddiPrimStream* stream = p3d::pddi->BeginPrims(shader->GetShader(), PDDI_PRIM_TRIANGLES, PDDI_V_CT, mTempQuads.mUseSize*6);

        for (int i = 0; i < mTempQuads.mUseSize; i++)
        {
            mTempQuads[i].Display(stream);//,world,camera,worldToCamera,intensityBias);
            //quads[i]->Display(stream,world,camera,worldToCamera,intensityBias);
        }

        p3d::pddi->EndPrims(stream);   

        p3d::stack->Pop();   

        // restore original z-compare mode
        if (!zTest)
        {
            if (origZCompare!=PDDI_COMPARE_ALWAYS)
            {
                p3d::pddi->SetZCompare(origZCompare);
            }
        }

        // restore z-writing (if previously enabled)
        if (origZWrite!=zWrite)
        {
            p3d::pddi->SetZWrite(origZWrite);
        }
    }
}

void 
tBillboardQuadGroup::ProcessShaders(ShaderCallback& callback)
{
    tRefCounted::Assign(shader, callback.Process(shader));
}

void
tBillboardQuadGroup::SetIntensityBias( float bias )
{
    intensityBias = bias;
}

void
tBillboardQuadGroup::ConvertToCameraRelative(float minSize, float maxSize, float minDist, float maxDist )
{
    // Iterate through the child quads and free them
    // replacing them with tCamRelativeBillboardQuads
    for ( int i = 0 ; i < GetNumQuads() ; i++ )
    {
        tBillboardQuad* oldQuad = GetQuad( i );
        tCamRelativeBillboardQuad* camRelativeQuad = new tCamRelativeBillboardQuad;
        camRelativeQuad->Set( minSize, maxSize, minDist, maxDist );
        
        // Copy statistics over. There should not be any shallow copies in here
 
        memcpy( camRelativeQuad->v, oldQuad->v , sizeof (camRelativeQuad->v) );
        memcpy( camRelativeQuad->t, oldQuad->t , sizeof (camRelativeQuad->t) );

        camRelativeQuad->SetName( oldQuad->GetName() );

        camRelativeQuad->displayColour = oldQuad->displayColour;
        camRelativeQuad->flip = oldQuad->flip;
        camRelativeQuad->billboardMode = oldQuad->billboardMode;

        camRelativeQuad->boundingBox = oldQuad->boundingBox;
        camRelativeQuad->boundingSphere = oldQuad->boundingSphere;

        camRelativeQuad->visible = oldQuad->visible;
        camRelativeQuad->perspective = oldQuad->perspective;
        camRelativeQuad->intensity = oldQuad->intensity;
        camRelativeQuad->transform = oldQuad->transform;
        camRelativeQuad->width = oldQuad->width;
        camRelativeQuad->height = oldQuad->height;
        camRelativeQuad->distance = oldQuad->distance;
        camRelativeQuad->colour = oldQuad->colour;

        memcpy( camRelativeQuad->uv, oldQuad->uv , sizeof (camRelativeQuad->uv) );

        camRelativeQuad->uvOffset = oldQuad->uvOffset;
        camRelativeQuad->cutOffMode = oldQuad->cutOffMode;
        camRelativeQuad->uvOffsetRange = oldQuad->uvOffsetRange;
        camRelativeQuad->sourceRange = oldQuad->sourceRange;
        camRelativeQuad->edgeRange = oldQuad->edgeRange;

     

        quads[i] = camRelativeQuad;
        oldQuad->ReleaseVerified();
    }
}


#ifdef RAD_XBOX

void tBillboardQuadGroup::SetMaxNumVisibilityTests( unsigned int numTests )
{
    if ( numTests != maxVisibilityTests )
    {
        maxVisibilityTests = numTests;
        for ( unsigned int i = 0; i < oldVisibilityTests.numTests; i++ )
        {
            tRefCounted::Release( oldVisibilityTests.tests[i] );
        }
        oldVisibilityTests.numTests = 0;
        oldVisibilityTests.tests.Resize( maxVisibilityTests );
        for ( unsigned int i = 0; i < currVisibilityTests.numTests; i++ )
        {
            tRefCounted::Release( currVisibilityTests.tests[i] );
        }
        currVisibilityTests.numTests = 0;
        currVisibilityTests.tests.Resize( maxVisibilityTests );
    }
}

unsigned int tBillboardQuadGroup::GetMaxNumVisibilityTests( void )
{
    return maxVisibilityTests;
}

void tBillboardQuadGroup::GetVisibilityResults( void )
{
    pddiExtVisibilityTest* extension = (pddiExtVisibilityTest*)p3d::pddi->GetExtension( PDDI_EXT_VISIBILITY_TEST );

    if ( extension != NULL )
    {
        if ( oldVisibilityTests.numTests > 0 )
        {
            for ( unsigned int i = 0; i < oldVisibilityTests.numTests; i++ )
            {
                oldVisibilityTests.tests[i]->numPixelsVisible = extension->Result( i );
                tRefCounted::Release( oldVisibilityTests.tests[i] );
            }
        }
        oldVisibilityTests.numTests = 0;
    }
}

void tBillboardQuadGroup::SubmitVisibilityTests( void )
{
    pddiExtVisibilityTest* visibilityExt = (pddiExtVisibilityTest*)p3d::pddi->GetExtension( PDDI_EXT_VISIBILITY_TEST );

    if ( visibilityExt != NULL )
    {
        if ( currVisibilityTests.numTests > 0 )
        {
            tView* view = p3d::context->GetView();
            P3DASSERT( view != NULL );

            tCamera* camera = view->GetCamera();
            P3DASSERT( camera != NULL );

            int numSamples = 1;
            pddiExtAntialiasControl* antialiasExt = (pddiExtAntialiasControl*)p3d::pddi->GetExtension( PDDI_EXT_ANTIALIAS_CONTROL );
            if ( antialiasExt != NULL )
            {
                switch ( antialiasExt->GetAntiAliasMode() )
                {
                    case pddiDisplayInit::MULTISAMPLE_2_LINEAR:
                    case pddiDisplayInit::MULTISAMPLE_2_QUINCUNX:
                    case pddiDisplayInit::SUPERSAMPLE_2_HORIZONTAL_LINEAR:
                    case pddiDisplayInit::SUPERSAMPLE_2_VERTICAL_LINEAR:
                    {
                        numSamples = 2;
                        break;
                    }
                    case pddiDisplayInit::MULTISAMPLE_4_NEAR:
                    case pddiDisplayInit::MULTISAMPLE_4_GAUSSIAN:
                    case pddiDisplayInit::SUPERSAMPLE_4_LINEAR:
                    case pddiDisplayInit::SUPERSAMPLE_4_GAUSSIAN:
                    {
                        numSamples = 4;
                        break;
                    }
                    case pddiDisplayInit::MULTISAMPLE_9_GAUSSIAN:
                    case pddiDisplayInit::SUPERSAMPLE_9_GAUSSIAN:
                    {
                        numSamples = 9;
                        break;
                    }
                }
            }

            for ( unsigned int i = 0; i < currVisibilityTests.numTests; i++ )
            {
                if ( oldVisibilityTests.numTests < maxVisibilityTests )
                {
                    rmt::Vector position;
                    camera->WorldToCamera( currVisibilityTests.tests[i]->visibilityPosition, &position );

                    float delta = visibilityTestSize * position.z;
                    float x1 = position.x - delta;
                    float x2 = position.x + delta;
                    float y1 = position.y - delta;
                    float y2 = position.y + delta;

                    p3d::stack->PushIdentity();
                    visibilityExt->Begin();
                    pddiPrimStream* stream = p3d::pddi->BeginPrims( NULL, PDDI_PRIM_TRIANGLES, PDDI_V_POSITION, 6 );

                    // bottom left
                    stream->Coord( x1, y1, position.z );

                    // top right
                    stream->Coord( x2, y2, position.z );

                    // bottom right
                    stream->Coord( x2, y1, position.z );

                    // top left
                    stream->Coord( x1, y2, position.z );

                    // top right
                    stream->Coord( x2, y2, position.z );

                    // bottom left
                    stream->Coord( x1, y1, position.z );

                    p3d::pddi->EndPrims( stream );   
                    visibilityExt->End( oldVisibilityTests.numTests );
                    p3d::stack->Pop();

                    rmt::Vector bl( x1, y1, position.z );
                    camera->CameraToWorld( bl, &bl );
                    p3d::context->WorldToDevice( bl, &bl );

                    rmt::Vector tl( x1, y2, position.z );
                    camera->CameraToWorld( tl, &tl );
                    p3d::context->WorldToDevice( tl, &tl );

                    rmt::Vector br( x2, y1, position.z );
                    camera->CameraToWorld( br, &br );
                    p3d::context->WorldToDevice( br, &br );

                    currVisibilityTests.tests[i]->maxPixelsVisible = rmt::FtoL( ( bl.y - tl.y ) * ( br.x - bl.x ) * visibilityTestScale ) * numSamples;
                    if ( currVisibilityTests.tests[i]->maxPixelsVisible <= 0 )
                    {
                        currVisibilityTests.tests[i]->maxPixelsVisible = 1;
                    }
                    oldVisibilityTests.tests[oldVisibilityTests.numTests] = currVisibilityTests.tests[i];
                    currVisibilityTests.tests[i] = NULL;
                    oldVisibilityTests.numTests++;
                }
                else
                {
                    for ( unsigned int j = i; j < currVisibilityTests.numTests; j++ )
                    {
                        tRefCounted::Release( currVisibilityTests.tests[j] );
                    }
                    break;
                }
            }

            currVisibilityTests.numTests = 0;
        }
    }
}

#endif

//*****************************************************************************
//
// Class tBillboardQuadGroupLoader
//
//*****************************************************************************
tBillboardQuadGroupLoader::tBillboardQuadGroupLoader() : tSimpleChunkHandler(Pure3D::BillboardObject::QUAD_GROUP)
{
}

tEntity* 
tBillboardQuadGroupLoader::LoadObject(tChunkFile* f, tEntityStore* store)
{
    int version = f->GetLong();
    P3DASSERT(version == BILLBOARD_QUAD_GROUP_VERSION);
   
    char name[256];
    char shaderName[256];
    f->GetPString(name);

    tBillboardQuadGroup* quadGroup = new tBillboardQuadGroup;
    quadGroup->SetName(name);

    f->GetPString(shaderName);
    tShader* shader = p3d::find<tShader>(store, shaderName);
    if(!shader)
    {
        p3d::printf("warning : shader \"%s\" not found for billboard quad group \"%s\"\n", shaderName, name);
        shader = new tShader("error");
    }

    quadGroup->SetShader(shader);
    quadGroup->SetZTest(f->GetLong()!=0);
    quadGroup->SetZWrite(f->GetLong()!=0);
    quadGroup->SetOcclusion(f->GetInt()); // is "Fog" in shader
    quadGroup->numQuads = f->GetLong();
    quadGroup->quads.SetSize( quadGroup->numQuads );

    int quadNum = 0;
    while( f->ChunksRemaining() )
    {
        switch ( f->BeginChunk() )
        {
            case Pure3D::BillboardObject::QUAD:
            {
                if (quadNum < quadGroup->numQuads)
                {
                    quadGroup->quads[quadNum] = LoadQuad( f, store );

                    P3DASSERT(quadGroup->quads[quadNum]);
                    quadGroup->quads[quadNum]->AddRef();

                    quadNum++;
                }
                break;
            }
            default:
                break;    
        }
        f->EndChunk();
    }

    if (quadGroup->numQuads>0)
    {
        quadGroup->boundingBox.high = quadGroup->quads[0]->boundingBox.high + quadGroup->quads[0]->GetTranslation();
        quadGroup->boundingBox.low = quadGroup->quads[0]->boundingBox.low + quadGroup->quads[0]->GetTranslation();

        for (int i = 1; i < quadGroup->numQuads; i++)
        {
            rmt::Vector translation = quadGroup->quads[i]->GetTranslation();
            quadGroup->boundingBox.high.x = Max(quadGroup->boundingBox.high.x, quadGroup->quads[i]->boundingBox.high.x + translation.x);
            quadGroup->boundingBox.high.y = Max(quadGroup->boundingBox.high.y, quadGroup->quads[i]->boundingBox.high.y + translation.y);
            quadGroup->boundingBox.high.z = Max(quadGroup->boundingBox.high.z, quadGroup->quads[i]->boundingBox.high.z + translation.z);
            quadGroup->boundingBox.low.x = Min(quadGroup->boundingBox.low.x, quadGroup->quads[i]->boundingBox.low.x + translation.x);
            quadGroup->boundingBox.low.y = Min(quadGroup->boundingBox.low.y, quadGroup->quads[i]->boundingBox.low.y + translation.y);
            quadGroup->boundingBox.low.z = Min(quadGroup->boundingBox.low.z, quadGroup->quads[i]->boundingBox.low.z + translation.z);
        }

        Vector v1(quadGroup->boundingBox.high);
        Vector v2(quadGroup->boundingBox.low);

        quadGroup->boundingSphere.centre.Sub(v1,v2);
        quadGroup->boundingSphere.centre.Scale(0.5f);
        quadGroup->boundingSphere.centre.Add(v2);

        v1.Sub(quadGroup->boundingSphere.centre);
        quadGroup->boundingSphere.radius = Abs(v1.Length());
    }
        
    return quadGroup;
}

tBillboardQuad* 
tBillboardQuadGroupLoader::LoadQuad(tChunkFile* f, tEntityStore* store)
{
    int version = f->GetLong();
    P3DASSERT(version == BILLBOARD_QUAD_VERSION);

    Vector translation;    
    Quaternion rotation;

    char buf[256];
    f->GetPString(buf);

    tBillboardQuad* quad = new tBillboardQuad;

    quad->SetName(buf);
    quad->SetBillboardMode(f->GetLong());
    f->GetData(&translation, 3, tFile::DWORD);
    quad->SetTranslation(translation);
    quad->SetColour(tColour(f->GetLong()));
    f->GetData(quad->uv, 8, tFile::DWORD);
    quad->SetWidth(f->GetFloat());
    quad->SetHeight(f->GetFloat());
    quad->SetDistance(f->GetFloat());
    f->GetData(&quad->uvOffset, 2, tFile::DWORD);        

    while( f->ChunksRemaining() )
    {
        switch ( f->BeginChunk() )
        {
            case Pure3D::BillboardObject::DISPLAY_INFO:
            {
                int version = f->GetLong();
              //  P3DASSERT(version == BILLBOARD_DISPLAY_INFO_VERSION);

                f->GetData(&rotation, 4, tFile::DWORD);
                quad->SetRotation(rotation);
                quad->SetCutOffMode(f->GetLong());
                f->GetData(&quad->uvOffsetRange, 2, tFile::DWORD);
                quad->SetSourceRange(f->GetFloat());
                quad->SetEdgeRange(f->GetFloat());               
                break;
            }
            case Pure3D::BillboardObject::PERSPECTIVE_INFO:
            {
                int version = f->GetLong();
              //  P3DASSERT(version == BILLBOARD_PERSPECTIVE_INFO_VERSION);

                quad->SetPerspectiveScale(f->GetLong()!=0);
                break;
            }
            default:
                break;    
        }
        f->EndChunk();
    }

    Vector2 u;
    Vector2 v;

    u.x = Min(quad->uv[0].u,Min(quad->uv[1].u,Min(quad->uv[2].u,quad->uv[3].u)));
    u.y = Max(quad->uv[0].u,Max(quad->uv[1].u,Max(quad->uv[2].u,quad->uv[3].u)));
    v.x = Min(quad->uv[0].v,Min(quad->uv[1].v,Min(quad->uv[2].v,quad->uv[3].v)));
    v.y = Max(quad->uv[0].v,Max(quad->uv[1].v,Max(quad->uv[2].v,quad->uv[3].v)));

    quad->uv[4].u = u.y-u.x;
    quad->uv[4].v = v.y-v.x;

    if (quad->billboardMode == p3dBillboardConstants::BillboardMode::NO_AXIS)
    {
        Vector v[8];
        
        v[0].Set(-quad->width,-quad->height,quad->distance>0.0f ? -quad->distance : 0.0f);
        v[1].Set(quad->width,-quad->height,quad->distance>0.0f ? -quad->distance : 0.0f);
        v[2].Set(quad->width,quad->height,quad->distance>0.0f ? -quad->distance : 0.0f);
        v[3].Set(-quad->width,quad->height,quad->distance>0.0f ? -quad->distance : 0.0f);
        v[4].Set(-quad->width,-quad->height,quad->distance<0.0f ? -quad->distance : 0.0f);
        v[5].Set(quad->width,-quad->height,quad->distance<0.0f ? -quad->distance : 0.0f);
        v[6].Set(quad->width,quad->height,quad->distance<0.0f ? -quad->distance : 0.0f);
        v[7].Set(-quad->width,quad->height,quad->distance<0.0f ? -quad->distance : 0.0f);

        int i;
        for (i = 0; i < 8; i++)
        {
            v[i].Rotate(quad->transform);
        }

        quad->boundingBox.high = quad->boundingBox.low = v[0];

        for (i = 1; i < 8; i++)
        {
            quad->boundingBox.high.x = Max(quad->boundingBox.high.x, v[i].x);
            quad->boundingBox.high.y = Max(quad->boundingBox.high.y, v[i].y);
            quad->boundingBox.high.z = Max(quad->boundingBox.high.z, v[i].z);
            quad->boundingBox.low.x = Min(quad->boundingBox.low.x, v[i].x);
            quad->boundingBox.low.y = Min(quad->boundingBox.low.y, v[i].y);
            quad->boundingBox.low.z = Min(quad->boundingBox.low.z, v[i].z);
        }

        Vector v1(quad->boundingBox.high);
        Vector v2(quad->boundingBox.low);

        quad->boundingSphere.centre.Sub(v1,v2);
        quad->boundingSphere.centre.Scale(0.5f);
        quad->boundingSphere.centre.Add(v2);

        v1.Sub(quad->boundingSphere.centre);
        quad->boundingSphere.radius = Abs(v1.Length());
    }
    else
    {
        Vector v(quad->width,quad->height,quad->distance);
        float len = v.Length();

        quad->boundingSphere.centre.Set(0.0f,0.0f,0.0f);
        quad->boundingSphere.radius = len;

        quad->boundingBox.high.Set(len,len,len);
        quad->boundingBox.low.Set(-len,-len,-len);
    }

    return quad;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
BillboardQuadManager::BillboardQuadManager()
{
    mpBBQGs.Allocate(25);
    mpBBQGs.AddUse(25);
    for(int i=0; i<mpBBQGs.mSize-1;i++)
    {
        mpBBQGs[i] = new tBillboardQuadGroup();
        mpBBQGs[i]->AddRef();
        mpBBQGs[i]->mTempQuads.Allocate(100);
    }
    mpBBQGs.ClearUse();
    sCollectPass = true;
}
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void BillboardQuadManager::DisplayAll()
{
    sCollectPass = false;
    int i=mpBBQGs.mUseSize-1;

    for(; i>-1; i--)
    {
        mpBBQGs[i]->Display();
    }
    Clear();
    sCollectPass = true;
}
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void BillboardQuadManager::Add(tBillboardQuadGroup* ipBBQG)
{
    tUID shaderUID = ipBBQG->GetShader()->GetUID();
    int i=mpBBQGs.mUseSize-1;//, j;

    for(; i>-1; i--)
    {
        if( mpBBQGs[i]->GetShader()->GetUID() == shaderUID )
        {
            mpBBQGs[i]->AddQuads(ipBBQG);
            break;
        }
    }

    if(i==-1) //Didn't find a match, add a new slot
    {
        mpBBQGs.AddUse(1);
        //Shallow copy, by design
        mpBBQGs[mpBBQGs.mUseSize-1]->ShallowCopy(ipBBQG);
    }
}