//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include <p3d/pointlight.hpp>
#include <p3d/utility.hpp>

#include <float.h>

tPointLight::tPointLight() :
    a(1.0f),
    b(0.0f),
    c(0.0f),
    range(0.0f)
{
}

tPointLight::tPointLight(tPointLight *pntLight) :
    tLight(pntLight),
    a(pntLight->a),
    b(pntLight->b),
    c(pntLight->c),
    range(pntLight->range)
{
}

void tPointLight::SetPosition(float x, float y, float z)
{
    position.x = x;
    position.y = y;
    position.z = z;
    Update();
}

void tPointLight::SetRange(float r)
{
    range = r;
    Update();
}

void tPointLight::SetAttenuation(float A, float B, float C)
{
    a = A;
    b = B;
    c = C;
    Update();
}

void tPointLight::GetAttenuation(float* A, float* B, float* C)
{
    *A = a;
    *B = b;
    *C = c;
    Update();
}

void tPointLight::Update()
{
    if(!active)
        return;

    pddiLightDesc desc(enabled);
    desc.SetPointLight(colour, (pddiVector*)&position, a, b, c);
    p3d::pddi->SetLight(slot, &desc);
}


