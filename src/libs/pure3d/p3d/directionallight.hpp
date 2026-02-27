//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _DIRECTIONALLIGHT_HPP
#define _DIRECTIONALLIGHT_HPP

#include <p3d/light.hpp>

class tDirectionalLight : public tLight
{
public:
    tDirectionalLight();
    tDirectionalLight(tDirectionalLight *dirLight);

    void SetDirection(float x, float y, float z);
    void GetDirection(float* x, float* y, float* z);

    void SetDirection(const rmt::Vector& d) { SetDirection(d.x, d.y, d.z); }
    const rmt::Vector& GetDirection(void) {return direction;}

protected:
    virtual void Update();
    virtual ~tDirectionalLight() { /* destructor */ }
    rmt::Vector direction;
};
#endif//_TDLIGHT_HPP

