//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================

/*
#include <p3d/shadow.hpp>

#include <radmath/radmath.hpp>
#include <p3d/shader.hpp>
#include <p3d/utility.hpp>
#include <p3d/vertexlist.hpp>
#include <p3d/primgroup.hpp>
#include <p3d/matrixstack.hpp>
#include <p3d/light.hpp>
#include <p3d/anim/polyskin.hpp>
#include <p3d/anim/skeleton.hpp>
#include <p3d/view.hpp>
#include <p3d/camera.hpp>
#include <p3d/chunkfile.hpp>
#include <constants/chunkids.hpp>

#define CARMACK_REVERSE // use inverted z-buffer mode 
#define DRAW_CAPS // draw caps on volumes

tShadowGeneratorImpl::tShadowGeneratorImpl()
{
    // create the shader for the shadow volumes 
    volumeShader = new tShader();
    volumeShader->AddRef();

    volumeShader->SetInt(PDDI_SP_SHADEMODE, PDDI_SHADE_FLAT);
    volumeShader->SetInt(PDDI_SP_ALPHATEST , false);
    volumeShader->SetInt(PDDI_SP_BLENDMODE, PDDI_BLEND_ALPHA);

    //Create the shadow wash shader
    washShader = new tShader();
    washShader->AddRef();

    washShader->SetInt(PDDI_SP_ISLIT, 0);
    washShader->SetInt(PDDI_SP_SHADEMODE, PDDI_SHADE_FLAT);
    washShader->SetInt(PDDI_SP_ALPHATEST, false );
    washShader->SetInt(PDDI_SP_BLENDMODE, PDDI_BLEND_MODULATE);
}

tShadowGeneratorImpl::~tShadowGeneratorImpl()
{
}

void tShadowGeneratorImpl::SetWashColour(tColour colour)
{
    washColour = colour;
}

void tShadowGeneratorImpl::Begin()
{
    pddiRenderContext* pddi = p3d::pddi;

    pddi->SetZWrite(false);

    //Save some rendering states
    oldCull = p3d::pddi->GetCullMode();

    #ifdef CARMACK_REVERSE
        pddi->SetZCompare(PDDI_COMPARE_GREATEREQUAL);
    #endif

     // Set up stencil compare fuction and masks.
     // Stencil test passes if ((ref & mask) cmpfn (stencil & mask)) is true.
    pddi->EnableStencilBuffer(true);
    pddi->SetStencilCompare(PDDI_COMPARE_ALWAYS);
    pddi->SetStencilWriteMask(0xffffffff);
    pddi->SetStencilMask(0xffffffff);

    pddi->SetColourWrite(false, false, false, false); // disable writes to colour buffer

    s_volumeShader = volumeShader;
}


void tShadowGeneratorImpl::End()
{
    s_volumeShader = NULL;

    pddiRenderContext* pddi = p3d::pddi;

    // restore the old cull mode
    p3d::pddi->SetCullMode(oldCull);

    pddi->SetColourWrite(true, true, true, true); // enable writes to colour buffer

    // Draws a big gray polygon over scene according to the mask in the
    // stencil buffer. (Any pixel with stencil==1 is in the shadow.)

    // Set renderstates (disable z-buffering, enable stencil
    // turn on alphablending on material)
    pddi->EnableZBuffer(false);
    pddi->EnableStencilBuffer(true);

     // This zero's the stencil for free when wash is applied
     pddi->SetStencilOp(PDDI_STENCIL_ZERO, PDDI_STENCIL_ZERO, PDDI_STENCIL_ZERO);


    // The stencilbuffer values indicates # of shadows that overlap each pixel.
     // We only want to draw pixels that are in shadow, which was set up in
     // RenderShadow() such that StencilBufferValue >= 1. In the Direct3D API, 
     // the stencil test is pseudo coded as:
     // StencilRef CompFunc StencilBufferValue
     // so we set our renderstates with StencilRef = 1 and CompFunc = LESSEQUAL.

    pddi->SetStencilCompare(PDDI_COMPARE_LESS);
    pddi->SetStencilRef(0);


    p3d::stack->PushIdentity();
    p3d::stack->Scale((float)p3d::display->GetWidth(),(float)p3d::display->GetHeight(),1);
    pddi->SetProjectionMode(PDDI_PROJECTION_DEVICE);

    // Draw the wash
    pddiPrimStream* stream = pddi->BeginPrims(washShader->GetShader(),PDDI_PRIM_TRISTRIP, PDDI_V_C, 4);

    float z = p3d::context->GetView()->GetCamera()->GetNearPlane();
    stream->Colour(washColour);stream->Coord(0,0,z);
    stream->Colour(washColour);stream->Coord(1,0,z);
    stream->Colour(washColour);stream->Coord(0,1,z);
    stream->Colour(washColour);stream->Coord(1,1,z);
    
    pddi->EndPrims(stream);

    p3d::stack->Pop();

    // Restore the render state
    pddi->SetProjectionMode(PDDI_PROJECTION_PERSPECTIVE);
    pddi->EnableZBuffer(true);
    pddi->SetZWrite(true);
    pddi->SetZCompare(PDDI_COMPARE_LESSEQUAL);
    pddi->EnableStencilBuffer(false);
}

void tShadowGeneratorImpl::PreRender()
{

}
*/