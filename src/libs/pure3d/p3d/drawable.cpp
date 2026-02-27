//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include <p3d/drawable.hpp>
#include <p3d/shader.hpp>
#include <p3d/utility.hpp>

static rmt::Box3D dummyBox;
static rmt::Sphere dummySphere;

void tDrawable::GetBoundingBox(rmt::Box3D* box)
{
    static bool isFirst = true;
    if(isFirst)
    {
        dummyBox.low.Set(0,0,0);
        dummyBox.high.Set(1,1,1);
        isFirst = false;
    }
    *box = dummyBox;
}

void tDrawable::GetBoundingSphere(rmt::Sphere* sphere)
{
    static bool isFirst = true;
    if(isFirst)
    {
        dummySphere.centre.Set(0,0,0);
        dummySphere.radius = 1;
        isFirst = false;
    }
    *sphere = dummySphere;
}

#ifndef RAD_RELEASE
void tDrawable::DisplayBoundingBox(tColour colour)
{
    rmt::Box3D b;
    GetBoundingBox(&b);
    rmt::Box3D* box = &b;

    pddiPrimStream* stream = p3d::pddi->BeginPrims(NULL, PDDI_PRIM_LINESTRIP, PDDI_V_C, 5);
    stream->Colour(colour);
    stream->Coord(box->low.x, box->low.y, box->low.z);
    stream->Colour(colour);
    stream->Coord(box->high.x, box->low.y, box->low.z);
    stream->Colour(colour);
    stream->Coord(box->high.x, box->high.y, box->low.z);
    stream->Colour(colour);
    stream->Coord(box->low.x, box->high.y, box->low.z);
    stream->Colour(colour);
    stream->Coord(box->low.x, box->low.y, box->low.z);
    p3d::pddi->EndPrims(stream);

    stream = p3d::pddi->BeginPrims(NULL, PDDI_PRIM_LINESTRIP, PDDI_V_C, 5);
    stream->Colour(colour);
    stream->Coord(box->high.x, box->high.y, box->high.z);
    stream->Colour(colour);
    stream->Coord(box->low.x, box->high.y, box->high.z);
    stream->Colour(colour);
    stream->Coord(box->low.x, box->low.y, box->high.z);
    stream->Colour(colour);
    stream->Coord(box->high.x, box->low.y, box->high.z);
    stream->Colour(colour);
    stream->Coord(box->high.x, box->high.y, box->high.z);
    p3d::pddi->EndPrims(stream);

    stream = p3d::pddi->BeginPrims(NULL, PDDI_PRIM_LINESTRIP, PDDI_V_C, 5);
    stream->Colour(colour);
    stream->Coord(box->high.x, box->high.y, box->high.z);
    stream->Colour(colour);
    stream->Coord(box->high.x, box->low.y, box->high.z);
    stream->Colour(colour);
    stream->Coord(box->high.x, box->low.y, box->low.z);
    stream->Colour(colour);
    stream->Coord(box->high.x, box->high.y, box->low.z);
    stream->Colour(colour);
    stream->Coord(box->high.x, box->high.y, box->high.z);
    p3d::pddi->EndPrims(stream);

    stream = p3d::pddi->BeginPrims(NULL, PDDI_PRIM_LINESTRIP, PDDI_V_C, 5);
    stream->Colour(colour);
    stream->Coord(box->low.x, box->high.y, box->high.z);
    stream->Colour(colour);
    stream->Coord(box->low.x, box->low.y, box->high.z);
    stream->Colour(colour);
    stream->Coord(box->low.x, box->low.y, box->low.z);
    stream->Colour(colour);
    stream->Coord(box->low.x, box->high.y, box->low.z);
    stream->Colour(colour);
    stream->Coord(box->low.x, box->high.y, box->high.z);
    p3d::pddi->EndPrims(stream);
}

void tDrawable::DisplayBoundingSphere(tColour colour)
{
    // nothing yet
    P3DASSERT(0);
}
#endif

tShader* tShaderIntBroadcast::Process(tShader* s)
{
    s->SetInt(param, val);
    return s;
}


tShader* tShaderFloatBroadcast::Process(tShader* s)
{
    s->SetFloat(param, val);
    return s;
}


tShader* tShaderColourBroadcast::Process(tShader* s)
{
    s->SetColour(param, val);
    return s;
}


tShader* tShaderTextureBroadcast::Process(tShader* s)
{
    s->SetTexture(param, val);
    return s;
}


tShader* tShaderVectorBroadcast::Process(tShader* s)
{
    s->SetVector(param, *((pddiVector*)&val));
    return s;
}


tShader* tShaderMatrixBroadcast::Process(tShader* s)
{
    s->SetMatrix(param, *((pddiMatrix*)&val));
    return s;
}

