//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
// File:          radmath.hpp
// Subsystem:     Radical Math Services
// Description:   This file contains all definitions and functions prototypes
//                required to use Radical Math functions
// Author:        Nigel Brooke (based on Radlib34 and Pure3D math library)
//
// History:
//                Jan21/2002 vmar - Added Sphere, Box, Plane, Triangle
//                Jan24/2002 nbrooke - seperated into multiple headers for clarity
//=============================================================================

#ifndef _RADMATH_HPP
#define _RADMATH_HPP

// all the types defined by this library
namespace RadicalMathLibrary
{
    class Vector;     // A standard 3-compnent vector
    class Vector4;    // A 4-element homogenous vector
    class Vector2;    // A 2 element vector
    class Matrix;     // A 4x4 homogenous matrix
    class Quaternion; 
    class Sphere;
    class Box3D;
    class Plane;
    class Triangle;
    class Randomizer;
    class Spline;
    class SplineCurve;
}

#include <radmath/buildconfig.hpp>
#include <radmath/util.hpp>
#include <radmath/trig.hpp>
#include <radmath/quaternion.hpp>
#include <radmath/vector.hpp>
#include <radmath/matrix.hpp>
#include <radmath/geometry.hpp>
#include <radmath/random.hpp>
#include <radmath/spline.hpp>

namespace rmt = RadicalMathLibrary;

int radMathInitialize();

#endif  //RADMATH_HPP

