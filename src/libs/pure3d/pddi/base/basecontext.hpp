/*===========================================================================
    basecontext.hpp
    10-Oct-99 Created by Neall

    Copyright (c)1999  Radical Entertainment, Inc.
    All rights reserved.
===========================================================================*/

#ifndef _BASECONTEXT_HPP
#define _BASECONTEXT_HPP

#include <pddi/pddi.hpp>
#include <pddi/pddiext.hpp>
#include <pddi/base/basetype.hpp>
#include <pddi/base/debug.hpp>
#include <pddi/base/base_font.hpp>

#define PDDI_MAX_UV_CHANNELS 4
#define PDDI_MAX_CLIP_PLANES 6
#define PDDI_MAX_LIGHTS 8
#define PDDI_MAX_STATS 40
#define PDDI_MAX_MATRIX_STACKS 5

//-------------------------------------------------------------------
class pddiFloatRect
{
public:
    pddiFloatRect() {}
    pddiFloatRect(float l, float t, float r, float b) { left = l; top = t; right = r; bottom = b; }
    float left;
    float top;
    float right;
    float bottom;

    void Set(float l, float t, float r, float b) { left = l; top = t; right = r; bottom = b; }
};

class pddiCamera
{
public:
    float nearPlane;
    float farPlane;
    float fov;
    float aspect;

    void Set(float n, float f, float fv, float ar) { nearPlane = n; farPlane = f; fov = fv; aspect = ar; }
};

//-------------------------------------------------------------------
// this is pretty mickey-mouse - have to clean this up at some point
class pddiMatrixStack : public pddiStack<pddiMatrix>
{
public:
    pddiMatrixStack(int d) : pddiStack<pddiMatrix>(d) 
    { 
        for(int i=0; i < depth; i++) stack[i].Identity(); 
    }
    void Identity()          { stack[top].Identity(); }
    void Load(pddiMatrix* m) { stack[top] = *m; }
    void Mult(pddiMatrix* m) { pddiMatrix r;  r.Mult(*m, stack[top]); stack[top] = r; }
};

//-------------------------------------------------------------------
class pddiPlane
{
public:
    bool enabled;
    float coeff[4];
};

class pddiLight
{
public:
    void Sync();

    bool enabled;
    pddiVector worldPosition;
    pddiVector viewPosition;
    pddiVector worldDirection;
    pddiVector viewDirection;
    pddiLightType type;
    pddiColour colour;
    float red;  // 0.0f - 255.0f
    float green;
    float blue;
    float intensity;
    float range;
    float attenA;
    float attenB;
    float attenC;
    float phi;
    float theta;
    float falloff;
};

//-------------------------------------------------------------------
class pddiState
{
public:
    virtual void Sync(pddiRenderContext* context) = 0;
};

class pddiRenderState : public pddiState
{
public:
    virtual void Sync(pddiRenderContext* context);

    pddiCullMode    cullMode;
    bool            zEnabled;
    pddiCompareMode zCompare;
    bool            zWrite;
    pddiFillMode    fillMode;
    bool            redWrite;
    bool            greenWrite;
    bool            blueWrite;
    bool            alphaWrite;
};

class pddiViewState : public pddiState
{
public:
    virtual void Sync(pddiRenderContext* context);

    pddiColour    clearColour;
    float         clearDepth;
    unsigned      clearStencil;
    pddiProjectionMode projectionMode;
    pddiFloatRect viewWindow;
    pddiCamera    camera;
    pddiPlane     clipPlane[PDDI_MAX_CLIP_PLANES];
    float         zBias;
    float         zRange[2];
    pddiRect      scissor;
};

class pddiLightingState : public pddiState
{
public:
    virtual void Sync(pddiRenderContext* context);

    pddiColour        ambient; 
    int               pad;
    pddiLight         light[PDDI_MAX_LIGHTS];
};

class pddiFogState : public pddiState
{
public:
    virtual void Sync(pddiRenderContext* context);

    bool        enabled;
    pddiColour  colour;
    float       start;
    float       end;
};

class pddiStencilState : public pddiState
{
public:
    virtual void Sync(pddiRenderContext* context);

    bool            enabled;
    pddiCompareMode compare;
    int             ref;
    unsigned        mask;
    unsigned        writeMask;
    pddiStencilOp   failOp;
    pddiStencilOp   zFailOp;
    pddiStencilOp   zPassOp;
};

//-------------------------------------------------------------------
// this class holds the entire rendering state of pddi
class pddiContextState
{
public:
    unsigned currentFrame;
    unsigned inFrame;
    unsigned contextFlags;

    // PDDI_MATRIX_MODELVIEW, PDDI_MATRIX_TEXTURE0-3
    pddiMatrixStack*  matrixStack[PDDI_MAX_MATRIX_STACKS];
    pddiStack<pddiViewState>*     viewStateStack;
    pddiStack<pddiRenderState>*   renderStateStack;
    pddiStack<pddiLightingState>* lightingStateStack;
    pddiStack<pddiFogState>*      fogStateStack;
    pddiStack<pddiStencilState>*  stencilStateStack;

    // this pointer is set to the current top of the corresponding state stack
    pddiViewState*     viewState;
    pddiRenderState*   renderState;
    pddiLightingState* lightingState;
    pddiFogState*      fogState;
    pddiStencilState*  stencilState;
};

//-------------------------------------------------------------------

class pddiBaseContext : public pddiRenderContext
{
public:
    pddiBaseContext(pddiDisplay* display, pddiDevice* device);

    // frame synchronisation
    virtual void BeginFrame();
    virtual void EndFrame();

    // clearing
    virtual void SetClearColour(pddiColour colour);
    virtual pddiColour GetClearColour(void);
    virtual void SetClearDepth(float depth);
    virtual float GetClearDepth(void);
    virtual void SetClearStencil(unsigned stencil);
    virtual unsigned GetClearStencil(void);
    virtual void Clear(unsigned bufferMask);

    // matrix stack
    virtual void LoadMatrix(pddiMatrixType id, pddiMatrix* matrix);
    virtual void IdentityMatrix(pddiMatrixType id);
    virtual void PushMatrix(pddiMatrixType id);
    virtual void PopMatrix(pddiMatrixType id);
    virtual void MultMatrix(pddiMatrixType id, pddiMatrix* matrix);
    virtual void ScaleMatrix(pddiMatrixType id, float x, float y, float z);
    virtual pddiMatrix* GetMatrix(pddiMatrixType id);
    
    virtual void PushIdentityMatrix(pddiMatrixType id);
    virtual void PushLoadMatrix(pddiMatrixType id, pddiMatrix* matrix);
    virtual void PushMultMatrix(pddiMatrixType id, pddiMatrix* matrix);

    // camera, view
    virtual void SetProjectionMode(pddiProjectionMode mode);
    virtual pddiProjectionMode GetProjectionMode();

    virtual void SetViewWindow(float left, float top, float right, float bottom);
    virtual void GetViewWindow(float* left, float* top, float* right, float* bottom);
    
    virtual void SetCamera(float nearPlane, float farPlane, float fov, float aspect);
    virtual void GetCamera(float* nearPlane, float* farPlane, float* fov, float* aspect);

    // viewport clipping
    virtual void SetScissor(pddiRect* rect);
    virtual void GetScissor(pddiRect* rect);

    // immediate mode prim rendering
    virtual pddiPrimStream* BeginPrims(pddiShader* material, pddiPrimType primType, unsigned vertexType, int vertexCount = 0, unsigned pass = 0 );
    virtual void EndPrims(pddiPrimStream* stream);

    // retained mode prim rendering
    virtual void DrawPrimBuffer(pddiShader* material, pddiPrimBuffer* buffer);

    // debug text
    virtual void DrawString(const char* s, int x, int y, pddiColour colour = pddiColour(255,255,255));

    // lighting
    virtual int GetMaxLights();

    virtual void SetAmbientLight(pddiColour colour);
    virtual pddiColour GetAmbientLight();

    virtual void SetLight(int handle, pddiLightDesc* desc);

    virtual void EnableLight(int handle, bool active);
    virtual bool IsLightEnabled(int handle);
    virtual void SetLightType(int handle, pddiLightType type);
    virtual pddiLightType GetLightType(int handle);

    virtual void SetLightColour(int handle, pddiColour colour);
    virtual pddiColour GetLightColour(int handle);

    virtual void SetLightPosition(int handle, pddiVector* dir);
    virtual void GetLightPosition(int handle, pddiVector* dir);

    virtual void SetLightDirection(int handle, pddiVector* dir);
    virtual void GetLightDirection(int handle, pddiVector* dir);

    virtual void SetLightRange(int handle, float range);
    virtual float GetLightRange(int handle);

    virtual void SetLightAttenuation(int handle, float a, float b, float c);
    virtual void GetLightAttenuation(int handle, float* a, float* b, float* c);

    virtual void SetLightCone(int handle, float phi, float theta, float falloff);
    virtual void GetLightCone(int handle, float* phi, float* theta, float* falloff);

    // backface culling
    virtual void SetCullMode(pddiCullMode mode);
    virtual pddiCullMode GetCullMode();

    // colour buffer control
    virtual void SetColourWrite(bool red, bool green, bool blue, bool alpha);
    virtual void GetColourWrite(bool* red, bool* green, bool* blue, bool* alpha);

    // z-buffer control
    virtual void EnableZBuffer(bool enable);
    virtual bool IsZBufferEnabled();

    virtual void SetZCompare(pddiCompareMode compareMode);
    virtual pddiCompareMode GetZCompare();

    virtual void SetZWrite(bool);
    virtual bool GetZWrite();

    virtual void SetZBias(float bias);
    virtual float GetZBias();

    virtual void SetZRange(float n, float f);
    virtual void GetZRange(float* n, float* f);

    // stencil buffer
    void EnableStencilBuffer(bool enable);
    bool IsStencilBufferEnabled();

    void SetStencilCompare(pddiCompareMode compare);
    pddiCompareMode GetStencilCompare();
    
    void SetStencilRef(int ref);
    int GetStencilRef();
    
    void SetStencilMask(unsigned mask);
    unsigned GetStencilMask();

    void SetStencilWriteMask(unsigned mask);
    unsigned GetStencilWriteMask();

    void SetStencilOp(pddiStencilOp failOp, pddiStencilOp zFailOp, pddiStencilOp zPassOp);
    void GetStencilOp(pddiStencilOp* failOp, pddiStencilOp* zFailOp, pddiStencilOp* zPassOp);

    // polygon fill
    virtual void SetFillMode(pddiFillMode mode);
    virtual pddiFillMode GetFillMode();

    // fog
    virtual void EnableFog(bool enable);
    virtual bool IsFogEnabled();
    virtual void SetFog(pddiColour colour, float start, float end);
    virtual void GetFog(pddiColour* colour, float* start, float* end);

    // utility
    virtual int GetIntStat(pddiStatType stat);
    virtual float GetFloatStat(pddiStatType stat);
    virtual void EnableStatsOverlay(bool);
    virtual bool IsStatsOverlayEnabled();

    // state preservation restoration
    virtual void PushState(pddiStateMask mask);
    virtual void PopState(pddiStateMask mask);

    // extensions
    virtual pddiExtension* GetExtension(unsigned extID);
    virtual bool VerifyExtension(unsigned extID);

    // forces drawing to complete
    virtual void DrawSync() { /**/ }
        
    // amb may7/2001 render to texture
    virtual unsigned SetRenderTarget(pddiTexture* tex) { return 0;/* nop */ }

    // amb nov28/2001
    virtual unsigned Screenshot(pddiColour* buffer, int nBytes) { return 0; }

#ifdef PDDI_TRACK_STATS
    inline void ADD_STAT(pddiStatType s, float v)
    {
        stats[s] += v;
    }

    inline void SET_STAT(pddiStatType s, float v)
    {
        stats[s] = v;
    }
#else
    inline void ADD_STAT(pddiStatType, float) {}
    inline void SET_STAT(pddiStatType, float) {}
#endif

protected:
    virtual ~pddiBaseContext();

    pddiDisplay*     display;
    pddiDevice*      device;
    pddiContextState state;
    float            stats[PDDI_MAX_STATS];
    pddiBaseFont*    font;
    bool             displayStats;
    bool             supressPrimTally;

    virtual void DefaultState();
    virtual void BuildMatrixStacks();
    virtual void BuildStateStacks();
    virtual void SyncState(unsigned mask);
    virtual void ResetStats();
    virtual void ComputeFrameStats();
    void TallyPrimStats(pddiPrimType type, int count);
    void TallyIndexedPrimStats(pddiPrimType type, int count);
    virtual void DisplayStats();
    //virtual void DumpState();

    int GetCurrentFrame() { return (int)stats[PDDI_STAT_CURRENT_FRAME]; }

    virtual void BeginTiming() = 0;
    virtual float EndTiming() = 0;

    virtual void LoadHardwareMatrix(pddiMatrixType id) = 0;
    virtual void SetupHardwareProjection() = 0;
    virtual void SetupHardwareLight(int handle) = 0;
};

void pddiMemRegAlloc(void* d, unsigned s);
void pddiMemRegFree(void* d);

#endif //_BASECONTEXT_HPP