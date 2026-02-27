//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _POINTCAMERA_HPP__
#define _POINTCAMERA_HPP__

#include <p3d/camera.hpp>

// class tPointCamera responsibilities
//      Position, target point and twist angle of this camera type
//      are defined in the world coordinate system.
//   A camera that is defined by a position for the camera, a target point (what the camera 
// is looking at) and a twist (a rotation around the axis it is looking along).
// Limits:
// Can exhibit odd behavior if the position and target are directly above/below each other, 
// due to the difficulty of finding a correct up vector to base the twist value off of.
//
class tPointCamera: public tCamera
{
public:
    tPointCamera();
    tPointCamera(tPointCamera*);

    void  SetTarget(const rmt::Vector&); // world space
    void  SetPosition(const rmt::Vector&); // world space
    void  SetTwist(float);
    void  SetUpVector(const rmt::Vector&);

    const rmt::Vector& GetUpVector(void) { return vup;}
    const rmt::Vector& GetTarget(void) { return target;}
    const rmt::Vector& GetPosition(void) { return position;}
    float GetTwist() { return TwistAngle; }

  // old forms of funcitons, 
  // not sure why we did it this way in the first place (returning const refs makes more sense)
  // but we might as well leave 'em to avoid breaking code (nbrooke 11/2001)
    void GetTarget(rmt::Vector* OutTarget) { *OutTarget = target; }
    void GetPosition(rmt::Vector *OutPos) { *OutPos = position; }
    void GetUpVector(rmt::Vector *OutVUp) { *OutVUp = vup; }
    void GetVUp(rmt::Vector * v) { GetUpVector(v);}
    void SetVUp(const rmt::Vector& v) { SetUpVector(v); }

protected:
    virtual ~tPointCamera();

    void Update();

    rmt::Vector position; //  The location of the camera in world space
    rmt::Vector target;   //  The location of the camera's target in world space
    rmt::Vector vup;      //  The "up" vector
    float TwistAngle;     //  The rotation of the camera around the axis it is looking along
};

#endif

