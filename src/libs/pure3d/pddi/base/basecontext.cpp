/*===========================================================================
    basecontext.cpp
    10-Oct-99 Created by Neall

    Copyright (c)1999, 2000, 2001, 2002  Radical Entertainment Ltd.
    All rights reserved.
===========================================================================*/

#include <pddi/base/basecontext.hpp>
#include <pddi/base/base_font.hpp>
#include <pddi/base/debug.hpp>

#include <stdio.h>
#include <string.h>
#include <float.h>
#include <math.h>

const int STATE_STACK_DEPTH = 4;

//-------------------------------------------------------------------
void pddiRenderState::Sync(pddiRenderContext* context)
{
    context->SetCullMode(cullMode);
    context->SetColourWrite(redWrite, greenWrite, blueWrite, alphaWrite);
    context->EnableZBuffer(zEnabled);
    context->SetZCompare(zCompare);
    context->SetZWrite(zWrite);
    context->SetFillMode(fillMode);
}

void pddiViewState::Sync(pddiRenderContext* context)
{
    context->SetClearColour(clearColour);
    context->SetClearDepth(clearDepth);
    context->SetClearStencil(clearStencil);
    context->SetProjectionMode(projectionMode);
    context->SetViewWindow(viewWindow.left, viewWindow.top, viewWindow.right, viewWindow.bottom);
    context->SetCamera(camera.nearPlane, camera.farPlane, camera.fov, camera.aspect);
    context->SetZBias(zBias);
    context->SetZRange(zRange[0], zRange[1]);
    context->SetScissor(&scissor);
}

void pddiLightingState::Sync(pddiRenderContext* context)
{
    context->SetAmbientLight(ambient);
    for(int i=0; i < PDDI_MAX_LIGHTS; i++)
    {
        context->EnableLight(i, light[i].enabled);
        context->SetLightType(i, light[i].type);
        context->SetLightColour(i, light[i].colour);
        context->SetLightDirection(i, &light[i].worldDirection);
        context->SetLightPosition(i, &light[i].worldPosition);
        context->SetLightRange(i, light[i].range);
        context->SetLightAttenuation(i, light[i].attenA, light[i].attenB, light[i].attenC);
        context->SetLightCone(i, light[i].phi, light[i].theta, light[i].falloff);
    }
}

void pddiFogState::Sync(pddiRenderContext* context)
{
    context->EnableFog(enabled);
    context->SetFog(colour, start, end);
}

void pddiStencilState::Sync(pddiRenderContext* context)
{
    context->EnableStencilBuffer(enabled);
    context->SetStencilCompare(compare);
    context->SetStencilRef(ref);
    context->SetStencilMask(mask);
    context->SetStencilWriteMask(mask);
    context->SetStencilOp(failOp, zFailOp, zPassOp);
}

//-------------------------------------------------------------------
pddiBaseContext::pddiBaseContext(pddiDisplay* disp, pddiDevice* dev)
{
    display = disp;
    display->AddRef();
    device = dev;
    device->AddRef();
    memset(&state, 0, sizeof(pddiContextState));
    
    BuildMatrixStacks();
    BuildStateStacks();

    font = new pddiBaseFont(this); 
    displayStats = false;
    supressPrimTally = false;

    stats[PDDI_STAT_BUFFERED_COUNT] = 0.0f;
    stats[PDDI_STAT_BUFFERED_ALLOC] = 0.0f;
    stats[PDDI_STAT_TEXTURE_ALLOCATED] = 0.0f;
    stats[PDDI_STAT_TEXTURE_SLOP] = 0.0f;
    stats[PDDI_STAT_TEXTURE_COUNT_4BIT] = 0.0f;
    stats[PDDI_STAT_TEXTURE_ALLOC_4BIT] = 0.0f;
    stats[PDDI_STAT_TEXTURE_COUNT_8BIT] = 0.0f;
    stats[PDDI_STAT_TEXTURE_ALLOC_8BIT] = 0.0f;
    stats[PDDI_STAT_TEXTURE_COUNT_16BIT] = 0.0f;
    stats[PDDI_STAT_TEXTURE_ALLOC_16BIT] = 0.0f;
    stats[PDDI_STAT_TEXTURE_COUNT_32BIT] = 0.0f;
    stats[PDDI_STAT_TEXTURE_ALLOC_32BIT] = 0.0f;
    stats[PDDI_STAT_TEXTURE_COUNT_DXTN] = 0.0f;
    stats[PDDI_STAT_TEXTURE_ALLOC_DXTN] = 0.0f;
    ResetStats();
}

pddiBaseContext::~pddiBaseContext()
{
    device->Release();
    display->Release();

    for(int i=0; i < PDDI_MAX_MATRIX_STACKS; i++)
    {
        delete state.matrixStack[i];
    }

    delete state.viewStateStack;
    delete state.renderStateStack;
    delete state.lightingStateStack;
    delete state.fogStateStack;
    delete state.stencilStateStack;

    delete font;
    
}

// initial state of the hardware
void pddiBaseContext::DefaultState()
{
    // viewState
    state.viewState->clearColour.Set(0,0,0);
    state.viewState->clearDepth = 1.0f;
    state.viewState->clearStencil = 0;
    state.viewState->projectionMode = PDDI_PROJECTION_PERSPECTIVE;
    state.viewState->viewWindow.Set(0.0f, 0.0f, 1.0f, 1.0f);
    state.viewState->camera.Set(1.0f, 1000.0f, 90.0f, 1.333f);
    state.viewState->zBias = 0.0f;
    state.viewState->zRange[0] = 0.0f;
    state.viewState->zRange[1] = 1.0f;   
    state.viewState->scissor.Set(0, 0, display->GetWidth(), display->GetHeight());
    memset(state.viewState->clipPlane, 0, sizeof(pddiPlane) * PDDI_MAX_CLIP_PLANES);
    
    // renderState
    state.renderState->zEnabled = true;
    state.renderState->redWrite = true;
    state.renderState->greenWrite = true;
    state.renderState->blueWrite = true;
    state.renderState->alphaWrite = true;
    state.renderState->cullMode = PDDI_CULL_NORMAL;
    state.renderState->zCompare = PDDI_COMPARE_LESSEQUAL;
    state.renderState->zWrite = true;
    state.renderState->fillMode = PDDI_FILL_SOLID;

    // lightingState
    state.lightingState->ambient.Set(16,16,16);
    for(int i=0; i < PDDI_MAX_LIGHTS; i++)
    {
        state.lightingState->light[i].enabled = false;
        state.lightingState->light[i].type = PDDI_LIGHT_DIRECTIONAL;
        state.lightingState->light[i].colour.Set(255,255,255);
        state.lightingState->light[i].worldPosition.Set(0.0f, 0.0f, 1.0f);
        state.lightingState->light[i].viewPosition.Set(0.0f, 0.0f, 0.0f);
        state.lightingState->light[i].worldDirection.Set(0.0f, 0.0f, 1.0f);
        state.lightingState->light[i].viewDirection.Set(0.0f, 0.0f, 0.0f);
        state.lightingState->light[i].range = 1000000.0f;
        state.lightingState->light[i].attenA = 1.0f;
        state.lightingState->light[i].attenB = 0.0f;
        state.lightingState->light[i].attenC = 0.0f;
        state.lightingState->light[i].phi    = 0.0f;
        state.lightingState->light[i].theta  = 0.0f;
        state.lightingState->light[i].falloff= 0.0f;
    }

    // fogState
    state.fogState->enabled = false;
    state.fogState->colour.Set(255,255,255);
    state.fogState->start = 0.0f;
    state.fogState->end = 1000.0f;

    // stencilState
    state.stencilState->enabled = false;
    state.stencilState->compare = PDDI_COMPARE_ALWAYS;
    state.stencilState->ref = 0;
    state.stencilState->mask = 0xffffffff;
    state.stencilState->writeMask = 0xffffffff;
    state.stencilState->failOp = PDDI_STENCIL_KEEP;
    state.stencilState->zFailOp = PDDI_STENCIL_KEEP;
    state.stencilState->zPassOp = PDDI_STENCIL_KEEP;

    SyncState(PDDI_STATE_ALL);
}

// sync the hardware with the base context state
// this call is incredibly expensive
void pddiBaseContext::SyncState(unsigned mask)
{
    if(mask & PDDI_STATE_VIEW)     state.viewState->Sync(this);
    if(mask & PDDI_STATE_RENDER)   state.renderState->Sync(this);
    if(mask & PDDI_STATE_LIGHTING) state.lightingState->Sync(this);
    if(mask & PDDI_STATE_FOG)      state.fogState->Sync(this);
    if(mask & PDDI_STATE_STENCIL)  state.stencilState->Sync(this);
}

void pddiBaseContext::BuildStateStacks()
{
    state.viewStateStack = new pddiStack<pddiViewState>(STATE_STACK_DEPTH);
    state.viewState = state.viewStateStack->Top();
    state.renderStateStack = new pddiStack<pddiRenderState>(STATE_STACK_DEPTH);
    state.renderState = state.renderStateStack->Top();
    state.lightingStateStack = new pddiStack<pddiLightingState>(STATE_STACK_DEPTH);
    state.lightingState = state.lightingStateStack->Top();
    state.fogStateStack = new pddiStack<pddiFogState>(STATE_STACK_DEPTH);
    state.fogState = state.fogStateStack->Top();
    state.stencilStateStack = new pddiStack<pddiStencilState>(STATE_STACK_DEPTH);
    state.stencilState = state.stencilStateStack->Top();
}

//-------------------------------------------------------------------
void pddiBaseContext::ResetStats()
{
 //  stats[PDDI_STAT_FRAME_TIME] = 0.0f;
    stats[PDDI_STAT_BUFFERED_PRIM] = 0.0f;
    stats[PDDI_STAT_BUFFERED_PRIM_VERT] = 0.0f;
    stats[PDDI_STAT_BUFFERED_INDEXED_PRIM] = 0.0f;
    stats[PDDI_STAT_BUFFERED_INDEXED_PRIM_VERT] = 0.0f;
    stats[PDDI_STAT_BUFFERED_PRIM_CALLS] = 0.0f;
    stats[PDDI_STAT_BUFFERED_PRIM_AVG] = 0.0f;
    stats[PDDI_STAT_STREAMED_PRIM] = 0.0f;
    stats[PDDI_STAT_STREAMED_PRIM_VERT] = 0.0f;
    stats[PDDI_STAT_STREAMED_PRIM_CALLS] = 0.0f;
    stats[PDDI_STAT_STREAMED_PRIM_AVG] = 0.0f;
    stats[PDDI_STAT_SKINNED_BONES] = 0.0f;
    stats[PDDI_STAT_SKINNED_XFORM_VERT] = 0.0f;
    stats[PDDI_STAT_SKINNED_XFORM_MS] = 0.0f;
    stats[PDDI_STAT_SKINNED_WAIT_MS] = 0.0f;
    stats[PDDI_STAT_MATRIX_OPS] = 0.0f;
    stats[PDDI_STAT_LIGHT_OPS] = 0.0f;
    stats[PDDI_STAT_MATERIAL_OPS] = 0.0f;
    stats[PDDI_STAT_TEXTURE_HITS] = 0.0f;
    stats[PDDI_STAT_TEXTURE_MISSES] = 0.0f;
    stats[PDDI_STAT_TEXTURE_UPLOADED] = 0.0f;
}

void pddiBaseContext::ComputeFrameStats()
{
    stats[PDDI_STAT_BUFFERED_PRIM_AVG] = 
        stats[PDDI_STAT_BUFFERED_PRIM_CALLS] > 0.0f ? 
        (stats[PDDI_STAT_BUFFERED_PRIM] + stats[PDDI_STAT_BUFFERED_INDEXED_PRIM]) / stats[PDDI_STAT_BUFFERED_PRIM_CALLS] : 0.0f;
    
    stats[PDDI_STAT_STREAMED_PRIM_AVG] = 
        stats[PDDI_STAT_STREAMED_PRIM_CALLS] > 0.0f ? 
        stats[PDDI_STAT_STREAMED_PRIM] / stats[PDDI_STAT_STREAMED_PRIM_CALLS] : 0.0f;
}

void pddiBaseContext::BuildMatrixStacks()
{
    state.matrixStack[PDDI_MATRIX_MODELVIEW] = new pddiMatrixStack(32);
    state.matrixStack[PDDI_MATRIX_TEXTURE0] = new pddiMatrixStack(2);
    state.matrixStack[PDDI_MATRIX_TEXTURE1] = new pddiMatrixStack(2);
    state.matrixStack[PDDI_MATRIX_TEXTURE2] = new pddiMatrixStack(2);
    state.matrixStack[PDDI_MATRIX_TEXTURE3] = new pddiMatrixStack(2);
}

void pddiBaseContext::DisplayStats()
{
// NOTE: Be careful about commas for the second sprintf arg
#ifdef PDDI_TRACK_STATS
    float bonesPerVertex = 0.0F;
    if (stats[PDDI_STAT_SKINNED_XFORM_VERT]) 
    {
        bonesPerVertex = stats[PDDI_STAT_SKINNED_BONES] / stats[PDDI_STAT_SKINNED_XFORM_VERT];
    }

    char buf[512];
    sprintf(buf,
        "frame: %.0f\n"
        "time: %.2fms (%2.0ffps)\n"
        "buf prims: %.0f, idx %.0f\n"
        "buf verts: %.0f, idx %.0f\n"
        "buf calls: %.0f (%.1f)\n"
        "buf alloc: %.0f (%.1f KB)\n"
        "str prims: %.0f\n"
        "str verts: %.0f\n"
        "str calls: %.0f (%.1f)\n"
        "state changes: %.0f\n"
        "matrix ops: %.0f\n"
        "light ops: %.0f\n"
        "tex alloc: %.0f KB (%.0f KB)\n"
        "textures : 4 bit : %d (%.0f KB), 8 bit : %d (%.0f KB)\n"
        "textures : 16 bit : %d (%.0f KB),32 bit : %d (%.0f KB)\n"
        "textures : dxtn : %d (%.0f KB)\n"
        "tex cache: %.0f hit, %.0f miss (%.1f KB)\n"
        "Skin XformVtx: %d  BPV: %.2f  XForm: %.3fms"
        "\n", 

        stats[PDDI_STAT_CURRENT_FRAME],
        stats[PDDI_STAT_FRAME_TIME], 1000.0f / stats[PDDI_STAT_FRAME_TIME],

        stats[PDDI_STAT_BUFFERED_PRIM],
        stats[PDDI_STAT_BUFFERED_INDEXED_PRIM],
        stats[PDDI_STAT_BUFFERED_PRIM_VERT],
        stats[PDDI_STAT_BUFFERED_INDEXED_PRIM_VERT],
        stats[PDDI_STAT_BUFFERED_PRIM_CALLS],
        stats[PDDI_STAT_BUFFERED_PRIM_AVG],
        stats[PDDI_STAT_BUFFERED_COUNT],
        stats[PDDI_STAT_BUFFERED_ALLOC],
        stats[PDDI_STAT_STREAMED_PRIM],
        stats[PDDI_STAT_STREAMED_PRIM_VERT],
        stats[PDDI_STAT_STREAMED_PRIM_CALLS],
        stats[PDDI_STAT_STREAMED_PRIM_AVG],
        stats[PDDI_STAT_MATERIAL_OPS],
        stats[PDDI_STAT_MATRIX_OPS],
        stats[PDDI_STAT_LIGHT_OPS],
        stats[PDDI_STAT_TEXTURE_ALLOCATED], 
        stats[PDDI_STAT_TEXTURE_SLOP], 
        (int)stats[PDDI_STAT_TEXTURE_COUNT_4BIT], 
        stats[PDDI_STAT_TEXTURE_ALLOC_4BIT], 
        (int)stats[PDDI_STAT_TEXTURE_COUNT_8BIT], 
        stats[PDDI_STAT_TEXTURE_ALLOC_8BIT], 
        (int)stats[PDDI_STAT_TEXTURE_COUNT_16BIT], 
        stats[PDDI_STAT_TEXTURE_ALLOC_16BIT], 
        (int)stats[PDDI_STAT_TEXTURE_COUNT_32BIT], 
        stats[PDDI_STAT_TEXTURE_ALLOC_32BIT], 
        (int)stats[PDDI_STAT_TEXTURE_COUNT_DXTN], 
        stats[PDDI_STAT_TEXTURE_ALLOC_DXTN], 
        stats[PDDI_STAT_TEXTURE_HITS], 
        stats[PDDI_STAT_TEXTURE_MISSES], 
        stats[PDDI_STAT_TEXTURE_UPLOADED],
        (int)stats[PDDI_STAT_SKINNED_XFORM_VERT],
        bonesPerVertex,
        stats[PDDI_STAT_SKINNED_XFORM_MS]
        );

        DrawString(buf, 10, 25, pddiColour(20, 20, 20));
        DrawString(buf, 9, 24, pddiColour(150, 150, 150));
#else
    static int cur = 0;
    static float runningTotal = 0.0f;
    static float last = stats[PDDI_STAT_FRAME_TIME];
    static float low = stats[PDDI_STAT_FRAME_TIME];
    static float high = stats[PDDI_STAT_FRAME_TIME];
    static float lastLow = stats[PDDI_STAT_FRAME_TIME];
    static float lastHigh = stats[PDDI_STAT_FRAME_TIME];
    static int frame = 0;
    const int num = 128;

    if(stats[PDDI_STAT_FRAME_TIME] < low)
    {
        low = stats[PDDI_STAT_FRAME_TIME];
    }

    if(stats[PDDI_STAT_FRAME_TIME] > high)
    {
        high = stats[PDDI_STAT_FRAME_TIME];
    }

    if((frame++ % num) == 0)
    {
        last = runningTotal / float(num);
        lastLow = low; 
        lastHigh = high; 
        low = stats[PDDI_STAT_FRAME_TIME];
        high = stats[PDDI_STAT_FRAME_TIME];
        runningTotal = 0.0f;
    }

    runningTotal += stats[PDDI_STAT_FRAME_TIME];

    char buf[128];
    sprintf(buf, "%.2fms (%2.0ffps), %.2f -> %.2f",
        last, 1000.0f / last, lastLow, lastHigh );

        DrawString(buf, 30, 35, pddiColour(20, 20, 20));
        DrawString(buf, 29, 34, pddiColour(150, 150, 150));
#endif
}

//-------------------------------------------------------------------
// pddiRenderContext interface implementation
//-------------------------------------------------------------------

void pddiBaseContext::BeginFrame()
{
    PDDIASSERT(!state.inFrame);
    state.inFrame = true;

    state.currentFrame++;
    stats[PDDI_STAT_CURRENT_FRAME] = (float)state.currentFrame;
    stats[PDDI_STAT_FRAME_TIME] = EndTiming();
    BeginTiming();
#ifdef PDDI_TRACK_STATS
    ResetStats();
#endif
}

void pddiBaseContext::EndFrame()
{

    PDDIASSERT(state.inFrame);
    if(displayStats)
    {
        ComputeFrameStats();
        DisplayStats();
    }
    state.inFrame = false;
}

void pddiBaseContext::SetClearColour(pddiColour colour)
{
    state.viewState->clearColour = colour;
}

pddiColour pddiBaseContext::GetClearColour(void)
{
    return state.viewState->clearColour;
}

void pddiBaseContext::SetClearDepth(float depth)
{
    state.viewState->clearDepth = depth;
}

float pddiBaseContext::GetClearDepth(void)
{
    return state.viewState->clearDepth;
}

void pddiBaseContext::SetClearStencil(unsigned stencil)
{
    state.viewState->clearStencil = stencil;
}

unsigned pddiBaseContext::GetClearStencil(void)
{
    return state.viewState->clearStencil;
}

void pddiBaseContext::Clear(unsigned bufferMask)
{
    // for warning
    (void)bufferMask;
}

void pddiBaseContext::IdentityMatrix(pddiMatrixType id)
{
    state.matrixStack[id]->Identity();
    LoadHardwareMatrix(id);
}

void pddiBaseContext::LoadMatrix(pddiMatrixType id, pddiMatrix* matrix)
{
    state.matrixStack[id]->Load(matrix);
    LoadHardwareMatrix(id);
}

void pddiBaseContext::PushMatrix(pddiMatrixType id)
{
    state.matrixStack[id]->Push();
}

void pddiBaseContext::PopMatrix(pddiMatrixType id)
{
    state.matrixStack[id]->Pop();
    LoadHardwareMatrix(id);
}

void pddiBaseContext::MultMatrix(pddiMatrixType id, pddiMatrix* matrix)
{
    state.matrixStack[id]->Mult(matrix);
    LoadHardwareMatrix(id);
}

void pddiBaseContext::ScaleMatrix(pddiMatrixType id, float x, float y, float z)
{
    pddiMatrix m;

    m.Identity();

    m.m[0][0] = x;
    m.m[1][1] = y;
    m.m[2][2] = z;
 
    state.matrixStack[id]->Mult(&m);
    LoadHardwareMatrix(id);
}

pddiMatrix* pddiBaseContext::GetMatrix(pddiMatrixType id)
{
    return state.matrixStack[id]->Top();
}

void pddiBaseContext::PushIdentityMatrix(pddiMatrixType id)
{
    state.matrixStack[id]->Push();
    state.matrixStack[id]->Identity();
    LoadHardwareMatrix(id);
}

void pddiBaseContext::PushLoadMatrix(pddiMatrixType id, pddiMatrix* matrix)
{
    state.matrixStack[id]->Push();
    state.matrixStack[id]->Load(matrix);
    LoadHardwareMatrix(id);
}

void pddiBaseContext::PushMultMatrix(pddiMatrixType id, pddiMatrix* matrix)
{
    state.matrixStack[id]->Push();
    state.matrixStack[id]->Mult(matrix);
    LoadHardwareMatrix(id);
}

void pddiBaseContext::SetProjectionMode(pddiProjectionMode mode)
{
    state.viewState->projectionMode = mode;
    SetupHardwareProjection();
}

pddiProjectionMode pddiBaseContext::GetProjectionMode()
{
    return state.viewState->projectionMode;
}

void pddiBaseContext::SetViewWindow(float left, float top, float right, float bottom)
{
    state.viewState->viewWindow.Set(left, top, right, bottom);
    SetupHardwareProjection();
}

void pddiBaseContext::GetViewWindow(float* left, float* top, float* right, float* bottom)
{
    *left = state.viewState->viewWindow.left;
    *top = state.viewState->viewWindow.top;
    *right = state.viewState->viewWindow.right;
    *bottom = state.viewState->viewWindow.bottom;
}

void pddiBaseContext::SetCamera(float n, float f, float fv, float ar)
{
    state.viewState->camera.Set(n, f, fv, ar);
    SetupHardwareProjection();
}

void pddiBaseContext::GetCamera(float* n, float* f, float* fov, float* aspect)
{
    *n = state.viewState->camera.nearPlane;
    *f = state.viewState->camera.farPlane;
    *fov = state.viewState->camera.fov;
    *aspect = state.viewState->camera.aspect;
}

void pddiBaseContext::SetScissor(pddiRect* rect)
{
    state.viewState->scissor = *rect;
}
 
void pddiBaseContext::GetScissor(pddiRect* rect)
{
    *rect = state.viewState->scissor;
}

pddiPrimStream* pddiBaseContext::BeginPrims(pddiShader* shader, pddiPrimType primType, unsigned vertexType, int vertexCount, unsigned pass )
{
    PDDIASSERT(shader);
    return 0;
}

void pddiBaseContext::EndPrims(pddiPrimStream* stream)
{
    PDDIASSERT(stream);
}


void pddiBaseContext::DrawPrimBuffer(pddiShader* material, pddiPrimBuffer* buffer)
{
    PDDIASSERT(0);
}

void pddiBaseContext::DrawString(const char* s, int x, int y, pddiColour colour)
{
#ifndef RAD_RELEASE
    supressPrimTally = true;

    pddiRect oldScissor;
    pddiProjectionMode proj = GetProjectionMode();
    GetScissor(&oldScissor);

    pddiRect scissor(0,0,display->GetWidth(), display->GetHeight());
    SetScissor(&scissor);
    SetProjectionMode(PDDI_PROJECTION_DEVICE);

    pddiMatrix m;
    m.Identity();

    PushIdentityMatrix(PDDI_MATRIX_MODELVIEW);
    font->Print(s, x, y, state.viewState->camera.nearPlane + 0.0001f, colour);
    PopMatrix(PDDI_MATRIX_MODELVIEW);

    SetScissor(&oldScissor);
    SetProjectionMode(proj);

    supressPrimTally = false;
#endif
}

int pddiBaseContext::GetMaxLights()
{
    return PDDI_MAX_LIGHTS;
}

void pddiBaseContext::SetAmbientLight(pddiColour colour)
{
    state.lightingState->ambient = colour;
}

pddiColour pddiBaseContext::GetAmbientLight()
{
    return state.lightingState->ambient;
}

void pddiBaseContext::SetLight(int l, pddiLightDesc* desc)
{
    if(l < PDDI_MAX_LIGHTS)
    {
        pddiLight* light = &state.lightingState->light[l];
        light->enabled = desc->enabled;
        light->type = desc->type;
        light->colour = desc->colour;
        light->red = (float)light->colour.Red();
        light->green = (float)light->colour.Green();
        light->blue = (float)light->colour.Blue();
        light->intensity = (float)light->colour.Intensity();
        light->worldPosition = desc->position;
        light->worldDirection = desc->direction;
        light->attenA = desc->attenuationA;
        light->attenB = desc->attenuationB;
        light->attenC = desc->attenuationC;
        light->phi = desc->phi;
        light->theta = desc->theta;
        light->falloff = desc->falloff;
        SetupHardwareLight(l);
    };
}

void pddiBaseContext::EnableLight(int light, bool enable)
{
    if(light < PDDI_MAX_LIGHTS)
    {
        state.lightingState->light[light].enabled = enable;
        SetupHardwareLight(light);
    }
}

bool pddiBaseContext::IsLightEnabled(int light)
{
    if(light < PDDI_MAX_LIGHTS)
    {
        return state.lightingState->light[light].enabled;
    }
    else
    {
        return false;
    }
}

void pddiBaseContext::SetLightType(int light, pddiLightType type)
{
    if(light < PDDI_MAX_LIGHTS)
    {
        state.lightingState->light[light].type = type;
        SetupHardwareLight(light);
    }
}

pddiLightType pddiBaseContext::GetLightType(int light)
{
    if(light < PDDI_MAX_LIGHTS)
    {
        return state.lightingState->light[light].type;
    }
    else
    {
        return PDDI_LIGHT_DIRECTIONAL;
    }
}

void pddiBaseContext::SetLightColour(int light, pddiColour colour)
{
    if(light < PDDI_MAX_LIGHTS)
    {
        state.lightingState->light[light].colour = colour;
        state.lightingState->light[light].red = (float)colour.Red();
        state.lightingState->light[light].green = (float)colour.Green();
        state.lightingState->light[light].blue = (float)colour.Blue();
        state.lightingState->light[light].intensity = (float)colour.Intensity();
        SetupHardwareLight(light);
    }
}

pddiColour pddiBaseContext::GetLightColour(int light)
{
    if(light < PDDI_MAX_LIGHTS)
    {
        return state.lightingState->light[light].colour;
    }
    else
    {
        return pddiColour(0);
    }
}

void pddiBaseContext::SetLightPosition(int light, pddiVector* vector)
{
    if(light < PDDI_MAX_LIGHTS)
    {
        state.lightingState->light[light].worldPosition= *vector;
        SetupHardwareLight(light);
    }
}

void pddiBaseContext::GetLightPosition(int light, pddiVector* vector)
{
    if(light < PDDI_MAX_LIGHTS)
    {
        *vector = state.lightingState->light[light].worldPosition;
    }
    else
    {
        vector->x = vector->y = vector->z = 0.0f;
    }
}

void pddiBaseContext::SetLightDirection(int light, pddiVector* vector)
{
    if(light < PDDI_MAX_LIGHTS)
    {
        state.lightingState->light[light].worldDirection= *vector;
        SetupHardwareLight(light);
    }
}

void pddiBaseContext::GetLightDirection(int light, pddiVector* vector)
{
    if(light < PDDI_MAX_LIGHTS)
    {
        *vector = state.lightingState->light[light].worldDirection;
    }
    else
    {
        vector->x = vector->y = vector->z = 0.0f;
    }
}

void pddiBaseContext::SetLightRange(int light, float range)
{
    if(light < PDDI_MAX_LIGHTS)
    {
        state.lightingState->light[light].range = range;
        SetupHardwareLight(light);
    }
}

float pddiBaseContext::GetLightRange(int light)
{
    if(light < PDDI_MAX_LIGHTS)
    {
        return state.lightingState->light[light].range;
    }
    else
    {
        return 0.0f;
    }
}

void pddiBaseContext::SetLightAttenuation(int light, float a, float b, float c)
{
    if(light < PDDI_MAX_LIGHTS)
    {
        state.lightingState->light[light].attenA = a;
        state.lightingState->light[light].attenB = b;
        state.lightingState->light[light].attenC = c;
        SetupHardwareLight(light);
    }
}

void pddiBaseContext::GetLightAttenuation(int light, float* a, float* b, float* c)
{
    if(light < PDDI_MAX_LIGHTS)
    {
        *a = state.lightingState->light[light].attenA;
        *b = state.lightingState->light[light].attenB;
        *c = state.lightingState->light[light].attenC;
    }
    else
    {
        *a = *b = *c = 0.0f;
    }
}

void pddiBaseContext::SetLightCone(int light, float a, float b, float c)
{
    if(light < PDDI_MAX_LIGHTS)
    {
        state.lightingState->light[light].phi = a;
        state.lightingState->light[light].theta = b;
        state.lightingState->light[light].falloff = c;
        SetupHardwareLight(light);
    }
}

void pddiBaseContext::GetLightCone(int light, float* a, float* b, float* c)
{
    if(light < PDDI_MAX_LIGHTS)
    {
        *a = state.lightingState->light[light].phi;
        *b = state.lightingState->light[light].theta;
        *c = state.lightingState->light[light].falloff;
    }
    else
    {
        *a = *b = *c = 0.0f;
    }
}


void pddiBaseContext::SetCullMode(pddiCullMode mode)
{
    state.renderState->cullMode = mode;
}

pddiCullMode pddiBaseContext::GetCullMode()
{
    return state.renderState->cullMode;
}

void pddiBaseContext::SetColourWrite(bool red, bool green, bool blue, bool alpha)
{
    state.renderState->redWrite = red;
    state.renderState->greenWrite = green;
    state.renderState->blueWrite = blue;
    state.renderState->alphaWrite = alpha;
}

void pddiBaseContext::GetColourWrite(bool* red, bool* green, bool* blue, bool* alpha)
{
    *red = state.renderState->redWrite;
    *green = state.renderState->greenWrite;
    *blue = state.renderState->blueWrite;
    *alpha = state.renderState->alphaWrite;
}

void pddiBaseContext::EnableZBuffer(bool enable)
{
    state.renderState->zEnabled = enable;
}

bool pddiBaseContext::IsZBufferEnabled()
{
    return state.renderState->zEnabled;
}

void pddiBaseContext::SetZCompare(pddiCompareMode zCompare)
{
    state.renderState->zCompare = zCompare;
}

pddiCompareMode pddiBaseContext::GetZCompare()
{
    return state.renderState->zCompare;
}

void pddiBaseContext::SetZWrite(bool zWrite)
{
    state.renderState->zWrite = zWrite;
}

bool pddiBaseContext::GetZWrite()
{
    return state.renderState->zWrite;
}

void pddiBaseContext::SetZBias(float bias)
{
    state.viewState->zBias = bias;
}

float pddiBaseContext::GetZBias()
{
    return state.viewState->zBias;
}

void pddiBaseContext::SetZRange(float n, float f)
{
    state.viewState->zRange[0] = n;
    state.viewState->zRange[1] = f;
}

void pddiBaseContext::GetZRange(float* n, float* f)
{
    *n = state.viewState->zRange[0];
    *f = state.viewState->zRange[1];
}

void pddiBaseContext::EnableStencilBuffer(bool enable)
{
    state.stencilState->enabled = enable;
}

bool pddiBaseContext::IsStencilBufferEnabled()
{
    return state.stencilState->enabled;
}

void pddiBaseContext::SetStencilCompare(pddiCompareMode compare)
{
    state.stencilState->compare = compare;
}

pddiCompareMode pddiBaseContext::GetStencilCompare()
{
    return state.stencilState->compare;
}

void pddiBaseContext::SetStencilRef(int ref)
{
    state.stencilState->ref = ref;
}

int pddiBaseContext::GetStencilRef()
{
    return state.stencilState->ref;
}

void pddiBaseContext::SetStencilMask(unsigned mask)
{
    state.stencilState->mask = mask;
}

unsigned pddiBaseContext::GetStencilMask()
{
    return state.stencilState->mask;
}

void pddiBaseContext::SetStencilWriteMask(unsigned mask)
{
    state.stencilState->writeMask = mask;
}

unsigned pddiBaseContext::GetStencilWriteMask()
{
    return state.stencilState->writeMask;
}

void pddiBaseContext::SetStencilOp(pddiStencilOp failOp, pddiStencilOp zFailOp, pddiStencilOp zPassOp)
{
    state.stencilState->failOp = failOp;
    state.stencilState->zFailOp = zFailOp;
    state.stencilState->zPassOp = zPassOp;
}

void pddiBaseContext::GetStencilOp(pddiStencilOp* failOp, pddiStencilOp* zFailOp, pddiStencilOp* zPassOp)
{
    *failOp = state.stencilState->failOp;
    *zFailOp = state.stencilState->zFailOp;
    *zPassOp = state.stencilState->zPassOp;
}

void pddiBaseContext::SetFillMode(pddiFillMode mode)
{
    state.renderState->fillMode = mode;
}

pddiFillMode pddiBaseContext::GetFillMode()
{
    return state.renderState->fillMode;
}

void pddiBaseContext::EnableFog(bool enable)
{
    state.fogState->enabled = enable;
}

bool pddiBaseContext::IsFogEnabled()
{
    return state.fogState->enabled;
}

void pddiBaseContext::SetFog(pddiColour colour, float start, float end)
{
    state.fogState->start = start;
    state.fogState->end = end;
    state.fogState->colour = colour;
}

void pddiBaseContext::GetFog(pddiColour* colour, float* start, float* end)
{
    *colour = state.fogState->colour;
    *start = state.fogState->start;
    *end = state.fogState->end;
}

int pddiBaseContext::GetIntStat(pddiStatType stat)
{
    PDDIASSERT(stat < PDDI_MAX_STATS);
    return (int)stats[stat];
}

float pddiBaseContext::GetFloatStat(pddiStatType stat)
{
    PDDIASSERT(stat < PDDI_MAX_STATS);
    return stats[stat];
}

void pddiBaseContext::EnableStatsOverlay(bool enable)
{
    displayStats = enable;
}

bool pddiBaseContext::IsStatsOverlayEnabled()
{
    return displayStats;
}

//-------------------------------------------------------------------
void pddiBaseContext::PushState(pddiStateMask mask)
{
    if(mask & PDDI_STATE_RENDER)
    {
        state.renderStateStack->Push();
        state.renderState = state.renderStateStack->Top();
    }

    if(mask & PDDI_STATE_VIEW)
    {
        state.viewStateStack->Push();
        state.viewState = state.viewStateStack->Top();
    }

    if(mask & PDDI_STATE_LIGHTING)
    {
        state.lightingStateStack->Push();
        state.lightingState = state.lightingStateStack->Top();
    }

    if(mask & PDDI_STATE_FOG)
    {
        state.fogStateStack->Push();
        state.fogState = state.fogStateStack->Top();
    }

    if(mask & PDDI_STATE_STENCIL)
    {
        state.stencilStateStack->Push();
        state.stencilState = state.stencilStateStack->Top();
    }
}

void pddiBaseContext::PopState(pddiStateMask mask)
{
    if(mask & PDDI_STATE_RENDER)
    {
        state.renderStateStack->Pop();
        state.renderState = state.renderStateStack->Top();
        state.renderState->Sync(this);
    }

    if(mask & PDDI_STATE_VIEW)
    {
        state.viewStateStack->Pop();
        state.viewState = state.viewStateStack->Top();
        state.viewState->Sync(this);
    }

    if(mask & PDDI_STATE_LIGHTING)
    {
        state.lightingStateStack->Pop();
        state.lightingState = state.lightingStateStack->Top();
        state.lightingState->Sync(this);
    }

    if(mask & PDDI_STATE_FOG)
    {
        state.fogStateStack->Pop();
        state.fogState = state.fogStateStack->Top();
        state.fogState->Sync(this);
    }

    if(mask & PDDI_STATE_STENCIL)
    {
        state.stencilStateStack->Pop();
        state.stencilState = state.stencilStateStack->Top();
        state.stencilState->Sync(this);
    }
}


static pddiExtMemRegistration::CallBack* memCallback = NULL;

class _pddiExtMemRegistration : public pddiExtMemRegistration 
{
public:
    void Register(CallBack* c)
    {
        memCallback = c;
    }
    
    void Release(void)
    {
        // ...
    }

protected:

};

static _pddiExtMemRegistration memExt;

void pddiMemRegAlloc(void* d, unsigned s)
{
    if(memCallback)
    {
        memCallback->Alloc(d,s);
    }
}

void pddiMemRegFree(void* d)
{
    if(memCallback)
    {
        memCallback->Free(d);
    }
}

// extension for routing PDDI debug messages
static pddiDevice::MessageCallback* pddiErrorHandler;

void pddiDevice::SetMessageCallback(pddiDevice::MessageCallback* c)
{
    pddiErrorHandler = c;
};

// This function does the real work of asserting
// it is called by the PDDIASSERT() macro
bool pddiAssertFailed(const char *file, int line, const char *cond, const char *msg, const char *module)
{
    char msgbuf[1024];
    if(!msg && !module)
    {      
        sprintf(msgbuf, "PDDI: (%s)", cond);
    }
    else
    {
        sprintf(msgbuf, "PDDI: '%s'\n%s\n\nModule: %s", msg, cond, module);
    }

    if(pddiErrorHandler)
    {
        return pddiErrorHandler->Fatal(msgbuf, file, line);
    }
    else
    {
        printf("%s", msgbuf);
    }
    return true;
}

//-------------------------------------------------------------------
pddiExtension* pddiBaseContext::GetExtension(unsigned id)
{
    switch(id)
    {
    case PDDI_EXT_MEM_REGISTRATION :
        return &memExt;
    }
    return 0;
}

bool pddiBaseContext::VerifyExtension(unsigned id)
{
    switch(id)
    {
        case PDDI_EXT_MEM_REGISTRATION :
            return true;
    }
    return false;
}