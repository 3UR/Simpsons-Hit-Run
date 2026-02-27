//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include <p3d/view.hpp>
#include <p3d/utility.hpp>
#include <p3d/camera.hpp>
#include <p3d/light.hpp>
#include <p3d/error.hpp>
#include <p3d/matrixstack.hpp>

//-------------------------------------------------------------------
tView::tView()
{
    t = l = 0.0f;
    r = b = 1.0f;

    camera = NULL;
    clearColour.Set(0,0,0);
    clearDepth = 1.0f;
    clearStencil = 0;

    ambientColour.Set(16,16,16);

    for(int i = 0; i< MAX_VIEW_LIGHTS; i++)
    {
        activeLightList[i] = NULL;
    }

    clearMask = PDDI_BUFFER_COLOUR | PDDI_BUFFER_DEPTH;

    fogEnabled = false;
    fogColour.Set(255,255,255);
    fogNear = fogFar = 0;

    cameraTransform.Identity();
    lightTransform.Identity();
}

tView::~tView()
{
    if(camera)
        camera->Release();

    RemoveAllLights();
}

//-------------------------------------------------------------------
void tView::SetCamera(tCamera* newCamera)
{
    tRefCounted::Assign(camera, newCamera);
}

//-------------------------------------------------------------------
void tView::SetWindow(float left, float top, float right, float bottom)
{
    P3DASSERT((left >= 0.0f) && (left <= 1.0f));
    P3DASSERT((top >= 0.0f) && (top <= 1.0f));
    P3DASSERT((right >= 0.0f) && (right <= 1.0f));
    P3DASSERT((bottom >= 0.0f) && (bottom <= 1.0f));
    P3DASSERT(right > left);
    P3DASSERT(bottom > top);

    l = left;
    t = top;
    r = right;
    b = bottom;
}

void tView::GetWindow(float* left, float* top, float* right, float* bottom)
{
    *left = l;
    *top = t;
    *right = r;
    *bottom = b;
}

//-------------------------------------------------------------------
void tView::SetAmbientLight(tColour colour)
{
    ambientColour = colour;
}

unsigned tView::AddLight(tLight* light)
{
    rAssert( light != NULL );
    for(int i = 0; i < MAX_VIEW_LIGHTS; i++)
    {
        if(!activeLightList[i])
        {
            light->AddRef();
            activeLightList[i] = light;
            return i;
        }
    }
    return 0xffffffff;
}

void tView::RemoveLight(unsigned i)
{
    tRefCounted::Release(activeLightList[i]);
}

void tView::RemoveAllLights()
{
    for(int i = 0; i < MAX_VIEW_LIGHTS; i++)
    {
        tRefCounted::Release(activeLightList[i]);
    }
}

tLight* tView::GetLight(unsigned l)
{
    P3DASSERT(l < MAX_VIEW_LIGHTS);
    return activeLightList[l];
}

//-------------------------------------------------------------------
void tView::BeginRender()
{
    P3DASSERT(camera);
    pddiRect viewRect(int(l * p3d::display->GetWidth()), 
                            int(t * p3d::display->GetHeight()), 
                            int(r * p3d::display->GetWidth()), 
                            int(b * p3d::display->GetHeight()));
    p3d::context->SetView(this);

    camera->SetState();

    p3d::pddi->SetViewWindow(l, t, r, b);
    p3d::pddi->SetScissor(&viewRect);

    // ambient light
    p3d::pddi->SetAmbientLight(ambientColour);

    // camera matrix setup
    rmt::Matrix newCW;
    rmt::Matrix newWC;

    newCW.Mult(camera->GetCameraToWorldMatrix(), cameraTransform);
    newWC.Invert(newCW);

    p3d::context->LoadViewMatrix(newWC, newCW);
    
    // setup active lights
    rmt::Matrix lightMat;

    lightMat.Mult(camera->GetCameraToWorldMatrix(), lightTransform);
    lightMat.Invert();

    p3d::stack->PushLoad(lightMat);

    int lightHandle = 0;
    for(int i=0; i < MAX_VIEW_LIGHTS; i++)
    {
        if(activeLightList[i])
            activeLightList[i]->Activate(lightHandle++);
    }

    p3d::stack->Pop();

    if(fogEnabled)
    {
        p3d::pddi->EnableFog(true);
        p3d::pddi->SetFog(fogColour,fogNear,fogFar);
    }
    else
    {
        p3d::pddi->EnableFog(false);
    }

    if(clearMask)
    {
        p3d::pddi->SetClearColour(clearColour);
        p3d::pddi->SetClearDepth(clearDepth);
        p3d::pddi->SetClearStencil(clearStencil);
        p3d::pddi->Clear(clearMask);
    }
}

void tView::EndRender()
{
    p3d::context->SetView(NULL);

    for(int i=0; i < MAX_VIEW_LIGHTS; i++)
    {
        if(activeLightList[i])
            activeLightList[i]->Deactivate();
    }
}

void tView::ViewToScreen(const rmt::Vector& view, rmt::Vector* screen)
{
    float aspect_ratio;
    if (camera)
    {
        float fov;
        camera->GetFOV(&fov, &aspect_ratio);
    }
    else
    {
        aspect_ratio = 1.0f;
    }

    screen->x = l - 0.5f + ((view.x + 0.5f) * (r - l));
    screen->y = 0.5f - t + (((view.y * aspect_ratio) - 0.5f) * (b - t));
    screen->z = view.z;
}

void tView::ScreenToView(const rmt::Vector& screen, rmt::Vector* view)
{
    float aspect_ratio;
    if (camera)
    {
        float fov;
        camera->GetFOV(&fov, &aspect_ratio);
    }
    else
    {
        aspect_ratio = 1.0f;
    }

    view->x = ((screen.x + 0.5f - l) / (r - l)) - 0.5f;
    view->y = (0.5f - ((0.5f - screen.y - t) / (b - t))) / aspect_ratio;
    view->z = screen.z;
}

