//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _POINTLIGHT_HPP
#define _POINTLIGHT_HPP

#include <p3d/light.hpp>

class tPointLight : public tLight
{
public:
    tPointLight();
    tPointLight(tPointLight *pntLight);

    void SetPosition(const rmt::Vector& d) { SetPosition(d.x, d.y, d.z); }
    void SetPosition(float x, float y, float z);

    void SetRange(float range);
    float GetRange() { return range; }

    void SetAttenuation(float a, float b, float c);
    void GetAttenuation(float* a, float* b, float *c);

protected:
    virtual void Update();
    virtual ~tPointLight() { /* destructor */ }

    float a; // attenuation = 1 / a + d*b + d^2*c
    float b;
    float c;

    float range;
};

#endif//_POINTLIGHT_HPP

