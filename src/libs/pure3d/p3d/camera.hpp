//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef __CAMERA_HPP__
#define __CAMERA_HPP__

#include <p3d/entity.hpp>
#include <p3d/loadmanager.hpp>

// This is the basic camera, it can be used as is as a 
// or extended to support other means of position 
// specification (see tPointCamera and tVectorCamera)
class tCamera: public tEntity
{
public:
    enum ClipCode { CLIP_NEAR = 0x1,  CLIP_FAR = 0x2, 
                         CLIP_TOP = 0x4,   CLIP_BOTTOM = 0x8, 
                         CLIP_LEFT = 0x10, CLIP_RIGHT = 0x20};

    tCamera();
    tCamera(tCamera*);

    // field of view
    virtual void  SetFOV (float  fov, float aspect);   
    virtual void  GetFOV (float* fov, float* aspect);  
    float GetFieldOfView(void) { return fov; }
    float GetAspectRatio(void) { return aspect; }

    // near and far clipping planes
    virtual void SetNearPlane(float);
    virtual float GetNearPlane(void);
    
    virtual void SetFarPlane(float);
    virtual float GetFarPlane(void);

    virtual void GetCameraSpaceClipPlane(ClipCode which, rmt::Plane* plane);
    virtual void GetWorldSpaceClipPlane(ClipCode which, rmt::Plane* plane);

    // utility functions to retrieve information about transformations
    virtual const rmt::Matrix& GetWorldToCameraMatrix(void);
    virtual const rmt::Matrix& GetCameraToWorldMatrix(void);

    virtual void CameraToWorld(const rmt::Vector& cam, rmt::Vector* world);
    virtual void WorldToCamera(const rmt::Vector& world, rmt::Vector* camera);

    virtual bool CameraToView(const rmt::Vector& cam, rmt::Vector* view);
    virtual bool WorldToView(const rmt::Vector& world, rmt::Vector* view);

    void ViewToCamera(const rmt::Vector& view, rmt::Vector* line1, rmt::Vector* line2);
    void ViewToWorld(const rmt::Vector& view, rmt::Vector* line1, rmt::Vector* line2);

    virtual bool PointVisibleCamera(const rmt::Vector& cameraSpacePoint);
    virtual bool SphereVisibleCamera(const rmt::Vector& cameraSpaceCentre, float radius);

    virtual bool PointVisible(const rmt::Vector& worldSpacePoint);
    virtual bool SphereVisible(const rmt::Vector& worldSpaceCentre, float radius);

    virtual unsigned PointClipCode(const rmt::Vector& cameraSpacePoint);

    virtual void GetWorldLookAtDirection(rmt::Vector *v);
    virtual void GetWorldPosition(rmt::Vector *pos);  
  
    // Sets a matrix to be used as the camera transformation matrix. 
    // This can interfere with the operation of derived classes, be cautious
    // if using it in conjunction with them (you might want to overrride it)
    virtual void SetCameraMatrix(rmt::Matrix* CameraMatrix);

    // copy this camera's state into pddi
    virtual void SetState(void);

protected:
    virtual ~tCamera();
    virtual void Update(void);

    float fov;
    float aspect;
    float nearPlane, farPlane;

    float fovScaleX,fovScaleY;
    float leftPlaneX, leftPlaneZ;
    float rightPlaneX, rightPlaneZ;
    float upPlaneY, upPlaneZ;
    float downPlaneY, downPlaneZ;

    rmt::Matrix worldToCamera;
    rmt::Matrix cameraToWorld;

    bool updated;
};

class tCameraLoader : public tSimpleChunkHandler
{
public:
    tCameraLoader();

protected:
    ~tCameraLoader() {}
    tEntity* LoadObject(tChunkFile*, tEntityStore*);
};

#endif

