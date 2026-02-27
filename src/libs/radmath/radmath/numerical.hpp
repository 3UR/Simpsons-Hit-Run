/*===========================================================================
   File:            numerical.hpp

   Description:     A collection of classes implementing various numerical algorithms
                    This file is intended as a repository for various numerical methods that can be shared by the various teams

   Owner:           Jean-Luc Duprat
   Creation:        August 28, 2002.

   Revisions:       August 28, 2002
                    Numerical Integration
                    
   Parts of this code Copyright (c) 2002 Radical Entertainment, Ltd. All rights reserved.
   Other copyrights holders explicitely identified in this file
===========================================================================*/

#ifndef _RADMATH_NUMERICAL_HPP
#define _RADMATH_NUMERICAL_HPP

namespace RadicalMathLibrary
{

// This code is adapted from code released by Magic Software, Inc.
// Copyright (c) 2000-2002 Magic Software, Inc.  All Rights Reserved.
// http://www.magic-software.com/License/free.pdf

class Integrate
{
public:
    Integrate();
    ~Integrate();

    // last parameter is for user-defined data
    typedef float (*Function)(float,void*);

    // Romberg integration
    void SetOrder (unsigned int iOrder);
    float RombergIntegral (float fA, float fB, Function oF, void* pvUserData = 0);

    // Gaussian quadrature
    float GaussianQuadrature (float fA, float fB, Function oF, void* pvUserData = 0);

protected:
    // parameters for Romberg integration
    unsigned int    ms_iOrder;
    float*          ms_apfRom[2];
};

}// end namespace

namespace rmt = RadicalMathLibrary;

#endif // _RADMATH_NUMERICAL_HPP
