/*===========================================================================
    optic.cpp
    18-Sep-00 Created by Bryan

    Copyright (c)2000 Radical Entertainment, Inc.
    All rights reserved.
===========================================================================*/

#include <p3d/error.hpp>
#include <p3d/utility.hpp>
#include <p3d/camera.hpp>
#include <p3d/matrixstack.hpp>
#include <p3d/effects/opticconstants.hpp>
#include <p3d/effects/optic.hpp>

using namespace RadicalMathLibrary;

//*****************************************************************************
//
// Class tOpticEffect
//
//*****************************************************************************
tOpticEffect::tOpticEffect()
{
}

tOpticEffect::~tOpticEffect()
{
}

void 
tOpticEffect::Display()
{
    // turn off fog
    bool origFog = p3d::pddi->IsFogEnabled();
    p3d::pddi->EnableFog(fog&origFog);

    // turn z-writing off
    bool origZWrite = p3d::pddi->GetZWrite();
    p3d::pddi->SetZWrite(zWrite);

    // turn z compare to always
    pddiCompareMode origZCompare = p3d::pddi->GetZCompare();
    if (!zTest)
    {
        p3d::pddi->SetZCompare(PDDI_COMPARE_ALWAYS);
    }

    DisplayEffect();

    // restore original z-compare mode
    if (!zTest)
    {
        p3d::pddi->SetZCompare(origZCompare);
    }

    // restore z-writing (if previously enabled)
    p3d::pddi->SetZWrite(origZWrite);

    // restore fog (if previously enabled)
    p3d::pddi->EnableFog(origFog);
}
