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

#include <radmath/numerical.hpp>

namespace RadicalMathLibrary
{

// class Integrate is adapted from code released by Magic Software, Inc.
// Copyright (c) 2000-2002 Magic Software, Inc.  All Rights Reserved.
// http://www.magic-software.com/License/free.pdf

Integrate::Integrate()
{
    ms_iOrder = 0;
}


Integrate::~Integrate()
{
    if(ms_iOrder>0)
    {
        delete[] ms_apfRom[0];
        delete[] ms_apfRom[1];
    }
}


void 
Integrate::SetOrder(unsigned int iOrder)
{
    if(iOrder == ms_iOrder)
        return;

    if(ms_iOrder>0)
    {
        delete[] ms_apfRom[0];
        delete[] ms_apfRom[1];
    }

    ms_iOrder = iOrder;
    ms_apfRom[0] = new float[ms_iOrder];
    ms_apfRom[1] = new float[ms_iOrder];
}


float 
Integrate::RombergIntegral (float fA, float fB, Function oF,
    void* pvUserData)
{
    float fH = fB - fA;

    ms_apfRom[0][0] = 0.5f*fH*(oF(fA,pvUserData)+oF(fB,pvUserData));
    for (unsigned int i0 = 2, iP0 = 1; i0 <= ms_iOrder; i0++, iP0 *= 2, fH *= 0.5f)
    {
        // approximations via the trapezoid rule
        float fSum = 0.0f;
        unsigned int i1;
        for (i1 = 1; i1 <= iP0; i1++)
            fSum += oF(fA + fH*(i1-0.5f),pvUserData);

        // Richardson extrapolation
        ms_apfRom[1][0] = 0.5f*(ms_apfRom[0][0] + fH*fSum);
        for (unsigned int i2 = 1, iP2 = 4; i2 < i0; i2++, iP2 *= 4)
        {
            ms_apfRom[1][i2] =
                (iP2*ms_apfRom[1][i2-1] - ms_apfRom[0][i2-1])/(iP2-1);
        }

        for (i1 = 0; i1 < i0; i1++)
            ms_apfRom[0][i1] = ms_apfRom[1][i1];
    }

    return ms_apfRom[0][ms_iOrder-1];
}


float 
Integrate::GaussianQuadrature (float fA, float fB, Function oF, void* pvUserData)
{
    // Legendre polynomials
    // P_0(x) = 1
    // P_1(x) = x
    // P_2(x) = (3x^2-1)/2
    // P_3(x) = x(5x^2-3)/2
    // P_4(x) = (35x^4-30x^2+3)/8
    // P_5(x) = x(63x^4-70x^2+15)/8

    // generation of polynomials
    //   d/dx[ (1-x^2) dP_n(x)/dx ] + n(n+1) P_n(x) = 0
    //   P_n(x) = sum_{k=0}^{floor(n/2)} c_k x^{n-2k}
    //     c_k = (-1)^k (2n-2k)! / [ 2^n k! (n-k)! (n-2k)! ]
    //   P_n(x) = ((-1)^n/(2^n n!)) d^n/dx^n[ (1-x^2)^n ]
    //   (n+1)P_{n+1}(x) = (2n+1) x P_n(x) - n P_{n-1}(x)
    //   (1-x^2) dP_n(x)/dx = -n x P_n(x) + n P_{n-1}(x)

    // roots of the Legendre polynomial of specified degree
    const int iDegree = 5;
    static const float s_afRoot[iDegree] =
    {
        -0.9061798459f,
        -0.5384693101f,
         0.0f,
        +0.5384693101f,
        +0.9061798459f
    };
    static const float s_afCoeff[iDegree] =
    {
        0.2369268850f,
        0.4786286705f,
        0.5688888889f,
        0.4786286705f,
        0.2369268850f
    };

    // Need to transform domain [a,b] to [-1,1].  If a <= x <= b and
    // -1 <= t <= 1, then x = ((b-a)*t+(b+a))/2.
    float fRadius = 0.5f*(fB - fA);
    float fCenter = 0.5f*(fB + fA);

    float fResult = 0.0f;
    for (int i = 0; i < iDegree; i++)
        fResult += s_afCoeff[i]*oF(fRadius*s_afRoot[i]+fCenter,pvUserData);
    fResult *= fRadius;

    return fResult;
}

} // end namespace
