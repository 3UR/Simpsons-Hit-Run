//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include <p3d/vectorcamera.hpp>

tVectorCamera::tVectorCamera() : 
    position(0.0f,  0.0f,  0.0f), 
    direction(0.0f,  0.0f,  1.0f),
    up(0.0f,  1.0f,  0.0f)
{
}

tVectorCamera::tVectorCamera(tVectorCamera* other) : tCamera(other)
{
    position = other->GetPosition();
    direction = other->GetDirection();
    up = other->GetUpVector();
    updated = false;
}

tVectorCamera::~tVectorCamera()
{
    //
}

void tVectorCamera::SetDirection(const rmt::Vector& dir)
{
    direction = dir;
    updated = false;
}

void tVectorCamera::SetUpVector(const rmt::Vector& vup)
{
    up= vup;
    updated = false;
}

void tVectorCamera::SetPosition(const rmt::Vector& pos)
{
    position = pos;
    updated = false;
}

void tVectorCamera::Update()
{
    rmt::Vector nheading;

    direction.Normalize();
    up.Normalize();
    cameraToWorld.FillHeading(direction, up);
    cameraToWorld.Row(3) = position;
    worldToCamera = cameraToWorld;
    worldToCamera.InvertOrtho();

    updated = true;
}

