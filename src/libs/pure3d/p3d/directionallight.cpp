//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include <p3d/directionallight.hpp>
#include <p3d/context.hpp>
#include <p3d/utility.hpp>

tDirectionalLight::tDirectionalLight() :
    direction(0.0f, 0.0f, 1.0f)
{
}

tDirectionalLight::tDirectionalLight(tDirectionalLight *dirLight) :
    tLight(dirLight),
    direction(dirLight->direction)
{
}

void tDirectionalLight::SetDirection(float x, float y, float z)
{
    direction.x = x;
    direction.y = y;
    direction.z = z;

    direction.Normalize();

    Update();
}

void tDirectionalLight::GetDirection(float* x, float* y, float* z)
{
    *x = direction.x;
    *y = direction.y;
    *z = direction.z;
}

void tDirectionalLight::Update()
{
    if(!active)
        return;

    pddiLightDesc desc(enabled);
    desc.SetDirectionalLight(colour, (pddiVector*)&direction);
    p3d::pddi->SetLight(slot, &desc);
}
    

