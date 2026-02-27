//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _VECTORCAMERA_HPP_
#define _VECTORCAMERA_HPP_

#include <p3d/camera.hpp>

// A camera constructed from a position in world space, a vector that
// the camera is looking along and a direction that serves as up for the
// generated camera space.
class tVectorCamera: public tCamera
{
public:
  tVectorCamera();
  tVectorCamera(tVectorCamera*);

  void SetDirection(const rmt::Vector& Direction); // New 'look along' vector in world space
  void SetUpVector(const rmt::Vector& Up);         // world space
  void SetPosition(const rmt::Vector& Position);   // world space

  const rmt::Vector& GetDirection(void) { return direction; }
  const rmt::Vector& GetUpVector(void)  { return up; }
  const rmt::Vector& GetPosition(void)  { return position; }

  // old forms of funcitons, 
  // not sure why we did it this way in the first place (returning const refs makes more sense)
  // but we might as well leave 'em to avoid breaking code (nbrooke 11/2001)
    void GetDirection(rmt::Vector* dir) { *dir = direction; }
    void GetUpVector(rmt::Vector* vup) { *vup = up; }
    void GetPosition(rmt::Vector* pos) { *pos = position; }

protected:
  virtual ~tVectorCamera();

  void Update();

  rmt::Vector position;  // The location of the camera in world space
  rmt::Vector direction; // The direction the camera is looking in
  rmt::Vector up;        // The direction that is up in camera space.
};

#endif

