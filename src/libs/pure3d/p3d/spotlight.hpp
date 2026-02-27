//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _SPOTLIGHT_HPP_
#define _SPOTLIGHT_HPP_

#include <p3d/light.hpp>

class tSpotLight : public tLight
{
public:
    tSpotLight();
    tSpotLight(tSpotLight *sptLight);

    void SetPosition(const rmt::Vector& d) { SetPosition(d.x, d.y, d.z); }
    void SetPosition(float x, float y, float z);

    void SetDirection(const rmt::Vector& d) { SetDirection(d.x, d.y, d.z); }
    void SetDirection(float x, float y, float z);
    void GetDirection(float* x, float* y, float* z);
    const rmt::Vector& GetDirection(void) { return direction; }

    void SetRange(float range);
    float GetRange() { return range; }

    void SetAttenuation(float a, float b, float c);
    void GetAttenuation(float* a, float* b, float *c);

    void SetCone(float phi, float theta, float falloff);
    void GetCone(float* phi, float* theta, float* falloff);

protected:
    virtual void Update();
    virtual ~tSpotLight() { /* destructor */ }
    rmt::Vector direction;

    float a; // attenuation = 1 / a + d*b + d^2*c
    float b;
    float c;

    float range;

    float phi;
    float theta;
    float falloff;
};

#endif

