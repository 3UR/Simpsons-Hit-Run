//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include <p3d/ambientlight.hpp>
#include <p3d/context.hpp>
#include <p3d/utility.hpp>

tAmbientLight::tAmbientLight()
{
}

tAmbientLight::tAmbientLight(tAmbientLight *ambLight) :
    tLight( ambLight )
{
}

tAmbientLight::~tAmbientLight()
{
}

void tAmbientLight::Update(void)
{
   p3d::pddi->SetAmbientLight(colour);
}
