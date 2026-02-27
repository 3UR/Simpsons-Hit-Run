//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include <p3d/pointcamera.hpp>

tPointCamera::tPointCamera()
{
    position.Set(0,0,0);
    target.Set(0,0,0);
    vup.Set(0,1,0);
    TwistAngle = 0;
}

tPointCamera::tPointCamera(tPointCamera* other) : tCamera(other)
{
    position = other->GetPosition();
    target = other->GetTarget();
    vup = other->GetUpVector();
    TwistAngle = other->GetTwist();
    updated = false;
}

tPointCamera::~tPointCamera()
{
    //
}

void tPointCamera::SetTarget(const rmt::Vector& InTarget)
{
    target = InTarget;
    updated = false;
}

void tPointCamera::SetPosition(const rmt::Vector& InPos)
{
    position = InPos;
    updated = false;
}

void tPointCamera::SetUpVector(const rmt::Vector& InVUp)
{
    vup = InVUp;
    updated = false;
}

void tPointCamera::SetTwist(float newtwist)
{
    TwistAngle = newtwist;
    updated = false;
}


void tPointCamera::Update()
{
    rmt::Vector heading;
    rmt::Matrix matrix;
    rmt::Matrix twistm;

    matrix.Identity();
    heading.Sub(target, position);
    
    heading.Normalize();

    // if heading is straight up, use (1,0,0) as vup instead
    if(heading.DotProduct(vup) == 1.0f)
    {   
        if((heading.y != 0.0f) || (heading.z != 0.0f))
        {
            matrix.FillHeading(heading, rmt::Vector(1,0,0));
        } else
        {
            matrix.FillHeading(heading, rmt::Vector(0,1,0));
        }
    }
    else
    {
        matrix.FillHeading(heading, vup);
    }

    twistm.Identity();
    twistm.FillRotateZ(TwistAngle);
    cameraToWorld.Mult(twistm, matrix);

    cameraToWorld.Row(3) = position;
    worldToCamera = cameraToWorld;
    worldToCamera.InvertOrtho();

    updated = true;
}

