//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _CONTEXT_HPP
#define _CONTEXT_HPP

#include <p3d/p3dtypes.hpp>
#include <p3d/platform.hpp>
#include <p3d/loadmanager.hpp>

class tView;
class tInventory;
class tLoadManager;
class tMatrixStack;
class pddiRenderContext;
class pddiDevice;
class pddiDisplay;
class pddiExtHardwareSkinning;
class pddiExtVertexProgram;

#define MATRIXSTACK (20) // size of matrix stack
#define MAX_PORT_LIGHTS 8

enum p3dMatrixType { P3D_MATRIX_MODELVIEW };

class tContext
{
public:
    // Frame synchronisztion
    void BeginFrame(void);
    void EndFrame(bool swapBuffers = true);
    bool InFrame(void) { return inFrame; } // are we currently in a frame?

    // Swaps front and back buffer
    // Users should not call this function directly unless they have already called
    // EndFrame(false);
    void SwapBuffers(void);

    tInventory*        GetInventory(void)   { return Inventory;}
    tLoadManager*      GetLoadManager(void) { return loadManager;}
    pddiRenderContext* GetContext(void)     { return RenderContext;}
    pddiDevice*        GetDevice(void)      { return RenderDevice;}
    pddiDisplay*       GetDisplay(void)     { return RenderDisplay;}

    // Clear the current output buffer at BeginFrame
    void     SetClearColour(tColour c)  { clearColour = c; }
    tColour  GetClearColour(void)       { return clearColour; }
    void     SetClearDepth(float depth) { clearDepth = depth; }
    float    GetClearDepth(void)        { return clearDepth; }
    void     SetClearStencil(unsigned stencil) {clearStencil = stencil;}
    unsigned GetClearStencil(void)             {return clearStencil;}
    void     SetClearMask(unsigned mask) {clearMask = mask;}
    unsigned GetClearMask(void)          {return clearMask;}
 

    // Current view
    tView* GetView();

    void ObjectToWorld(const rmt::Vector& object, rmt::Vector* world);
    void WorldToObject(const rmt::Vector& world, rmt::Vector* object);

    void ScreenToDevice(const rmt::Vector& screen, rmt::Vector* device);
    void DeviceToScreen(const rmt::Vector& device, rmt::Vector* screen);

    void ObjectToView(const rmt::Vector& object, rmt::Vector* view);
    void ObjectToDevice(const rmt::Vector& object, rmt::Vector* device);
    void WorldToView(const rmt::Vector& world, rmt::Vector* view);
    void WorldToDevice(const rmt::Vector& world, rmt::Vector* device);

    // matrix stack control
    tMatrixStack* GetMatrixStack(p3dMatrixType type = P3D_MATRIX_MODELVIEW);

    rmt::Matrix* GetViewMatrix();
    rmt::Matrix* GetWorldMatrix();

    void RebuildCTM();
    rmt::Matrix* GetInverseViewMatrix() { return &CVMI; }

    // functions users shouldn't neet to call (automatically called by tView::BeginRender)
    void SetView(tView *);   
    void LoadViewMatrix(const rmt::Matrix& matrix, const rmt::Matrix& inverseMatrix); 

    pddiExtHardwareSkinning* GetHardwareSkinning(void);
    pddiExtVertexProgram* GetVertexProgram(void);

private:
    friend class tPlatform;
    tContext(pddiDevice*, pddiDisplay*, pddiRenderContext*);
    ~tContext();

    void Setup(void);
    void Shutdown(void);

    // The inventory used for resolving load dependancies
    tInventory* Inventory;

    // Resposible for managing loading
    tLoadManager* loadManager;

    // Frame count increased each time you call BeginFrame.
    unsigned FrameCount;

    tMatrixStack* stack;
    tView*        View;  // Current used tView object
    
    rmt::Matrix CVM;   // Current View Matrix
    rmt::Matrix CVMI;  // Current View Matrix Inverse
    rmt::Matrix CWM;   // current world matrix

    tColour  clearColour;
    float    clearDepth;
    unsigned clearStencil;
    unsigned clearMask;

    int nLights;

    pddiDevice*        RenderDevice;
    pddiRenderContext* RenderContext;
    pddiDisplay*       RenderDisplay;
    pddiExtHardwareSkinning* skinning;
    pddiExtVertexProgram* vertexProgram;

    bool inFrame;
};

#endif

