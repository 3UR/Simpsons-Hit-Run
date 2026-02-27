/*===========================================================================
    debugdraw.hpp
    25-Jun-01 Created by Liberty 
    Some basic wireframe drawing functions

    Copyright (c)2001 Radical Entertainment, Inc.
    All rights reserved.
===========================================================================*/
#ifndef _DEBUGDRAW_HPP_
#define _DEBUGDRAW_HPP_

#ifndef RAD_RELEASE

#include <p3d/shader.hpp>
#include <radmath/radmath.hpp>

//---------------------------------------------------------------------------
// Debug drawing functions
//---------------------------------------------------------------------------
void P3DDrawOrientedBox
(
    const rmt::Vector minCorner,
    const rmt::Vector maxCorner,
    tShader& shader,
    tColour lineColor = tColour(255,0,0),
    bool drawDiagonals = true
);

void P3DDrawSphere
(
    float radius,
    const rmt::Vector position,
    tShader& shader,
    tColour lineColor = tColour(255,0,0),
    float wireResolution = 4, 
    float wireContours = 4
);

void P3DDrawCylinder
(
    float radius,
    rmt::Vector position,
    rmt::Vector position2,
    tShader& shader,
    tColour lineColor = tColour(255,0,0),
    float wireResolution = 4, 
    float wireContours = 4
);

void P3DDrawCone
(
    float radius,
    rmt::Vector position,
    float radius2,
    rmt::Vector position2,
    tShader& shader,
    tColour lineColor = tColour(255,0,0),
    float wireResolution = 4, 
    float wireContours = 4
);

void P3DDrawCapsule
( 
    float radius,
    rmt::Vector position,
    rmt::Vector position2,
    tShader& shader,
    tColour lineColor = tColour(255,0,0),
    float wireResolution = 4, 
    float wireContours = 4
);

#endif //_DEBUGDRAW_HPP_

#endif