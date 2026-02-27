//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include <p3d/spotlight.hpp>
#include <p3d/context.hpp>
#include <p3d/utility.hpp>
#include <radmath/radmath.hpp>

tSpotLight::tSpotLight() :
    direction(0.0f, 0.0f, 1.0f),
    a(1.0f),
    b(0.0f),
    c(0.0f),
    range(0.0f),
    phi(rmt::PI_BY2),
    theta(rmt::PI_BY4),
    falloff(1.0f)
{
}

tSpotLight::tSpotLight(tSpotLight *sptLight) :
    tLight(sptLight),
    direction(sptLight->direction),
    a(sptLight->a),
    b(sptLight->b),
    c(sptLight->c),
    range(sptLight->range),
    phi(sptLight->phi),
    theta(sptLight->theta),
    falloff(sptLight->falloff)
{
}

void tSpotLight::SetPosition(float x, float y, float z)
{
    position.x = x;
    position.y = y;
    position.z = z;
    Update();
}

void tSpotLight::SetDirection(float x, float y, float z)
{
    direction.x = x;
    direction.y = y;
    direction.z = z;
    Update();
}

void tSpotLight::GetDirection(float* x, float* y, float* z)
{
    *x = direction.x;
    *y = direction.y;
    *z = direction.z;
}

void tSpotLight::SetRange(float r)
{
    range = r;
    Update();
}

void tSpotLight::SetAttenuation(float A, float B, float C)
{
    a = A;
    b = B;
    c = C;
    Update();
}

void tSpotLight::GetAttenuation(float* A, float* B, float* C)
{
    *A = a;
    *B = b;
    *C = c;
}

void tSpotLight::SetCone(float Phi, float Theta, float Falloff)
{
    phi = Phi;
    theta = Theta;
    falloff = Falloff;
    Update();
}

void tSpotLight::GetCone(float* Phi, float* Theta, float* Falloff)
{
    *Phi = phi;
    *Theta = theta;
    *Falloff = falloff;
}

void tSpotLight::Update()
{
    if(!active)
        return;

    pddiLightDesc desc(enabled);
    desc.SetSpotLight(colour, (pddiVector*)&position, (pddiVector*)&direction, phi, theta, falloff, a, b, c);
    p3d::pddi->SetLight(slot, &desc);
}
    


