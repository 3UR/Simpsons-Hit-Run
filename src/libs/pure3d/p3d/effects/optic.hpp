/*===========================================================================
    optic.hpp
    18-Sep-00 Created by Bryan

    Copyright (c)2000 Radical Entertainment, Inc.
    All rights reserved.
===========================================================================*/

#ifndef _OPTIC_HPP
#define _OPTIC_HPP

#include <radmath/radmath.hpp>
#include <p3d/loadmanager.hpp>
#include <p3d/drawable.hpp>

class tCamera;

//*****************************************************************************
//
// Class tOpticEffect - base parent class for optic effects
//
//*****************************************************************************
class tOpticEffect: public tDrawable
{
public:
    tOpticEffect();

    // enable ztest
    void SetZTest(bool enable)                                      { zTest = enable; }
    bool GetZTest() const                                           { return zTest; }

    // enable zwrite
    void SetZWrite(bool enable)                                     { zWrite = enable; }
    bool GetZWrite() const                                          { return zWrite; }

    // enable fog
    void SetFog(bool enable)                                        { fog = enable; }
    bool GetFog() const                                             { return fog; }

    virtual void Display();

protected:
    virtual ~tOpticEffect();
    virtual void DisplayEffect() = 0;

    bool zTest;
    bool zWrite;
    bool fog;
};

#endif /* _OPTIC_HPP */

