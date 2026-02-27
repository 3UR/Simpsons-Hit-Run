//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _VIEW_HPP
#define _VIEW_HPP

class tView;

#include <p3d/refcounted.hpp>
#include <p3d/p3dtypes.hpp>
#include <pddi/pddienum.hpp>

class tCamera;
class tLight;

#define MAX_VIEW_LIGHTS (32)

class tView : public tRefCounted
{
public:
    tView();

    // set view window, 
    // all coordiantes must be in 0->1, 0,0 == upper left corner of screen
    // default = (0,0,1,1) 
    void SetWindow(float l,  float t,  float r,  float b);
    void GetWindow(float* l, float* t, float* r, float* b);
    
    // set and retreive current camera
    // setting the camera withing Begin/EndRender has no effect
    void SetCamera(tCamera *);
    tCamera* GetCamera(void) { return camera; }

    void SetAmbientLight(tColour colour);
    tColour GetAmbientLight(void) { return ambientColour; }

    // Set automatic clearing, if clear mask is non-zero
    // specified buffers will be cleared at BeginRender time
    void     SetClearColour(tColour c)  { clearColour = c; }
    tColour  GetClearColour(void)       { return clearColour; }
    void     SetClearDepth(float depth) { clearDepth = depth; }
    float    GetClearDepth(void)        { return clearDepth; }
    void     SetClearStencil(unsigned stencil) {clearStencil = stencil;}
    unsigned GetClearStencil(void)             {return clearStencil;}
    void     SetClearMask(unsigned mask) {clearMask = mask;} // some combination of PDDI_BUFFER_* flags
    unsigned GetClearMask(void)          {return clearMask;}

    void     SetBackgroundColour(tColour colour) {SetClearColour(colour);}
    tColour  GetBackgroundColour(void)           { return GetClearColour(); }

    void EnableFog(bool b)       { fogEnabled = b; }
    bool IsFogEnabled(void)      { return fogEnabled; }

    void SetFogColour(tColour c) { fogColour = c; }
    tColour GetFogColour(void)   { return fogColour; }
    
    void  SetFogPlanes(float fnear, float ffar) { fogNear = fnear; fogFar = ffar; }
    void  GetFogPlanes(float* fnear, float* ffar)   { *fnear = fogNear; *ffar = fogFar; }

    unsigned AddLight(tLight *);
    tLight*  GetLight(unsigned);
    void     RemoveLight(unsigned);
    void     RemoveAllLights(void);

    // change preconcatenated transforms for camera and lights, this allows use of cameras and lights that are 
    // defined in different spaces (such as from a scenegraph/NIS that has been positioned in the world)
    void SetCameraTransform(const rmt::Matrix& t) { cameraTransform = t; }
    void SetLightTransform(const rmt::Matrix& t) { lightTransform = t; }


    void BeginRender();
    void EndRender();

    void ViewToScreen(const rmt::Vector& view, rmt::Vector* screen);
    void ScreenToView(const rmt::Vector& screen, rmt::Vector* view);

protected:
    virtual ~tView();

    float     l,t,r,b;   // Top left
    tCamera*  camera;
    tColour ambientColour;

    tColour  clearColour;
    float    clearDepth;
    unsigned clearStencil;
    unsigned clearMask;

    tColour fogColour;
    float fogNear, fogFar;

    tLight* activeLightList[MAX_VIEW_LIGHTS];
    bool fogEnabled;

    rmt::Matrix cameraTransform;
    rmt::Matrix lightTransform;
};

#endif

