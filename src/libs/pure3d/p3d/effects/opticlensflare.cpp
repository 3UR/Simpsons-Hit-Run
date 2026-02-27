/*===========================================================================
    opticlensflare.cpp

    Copyright (c)2000 Radical Entertainment, Inc.
    All rights reserved.
===========================================================================*/
#include <p3d/utility.hpp>
#include <p3d/error.hpp>
#include <p3d/shader.hpp>
#include <p3d/view.hpp>
#include <p3d/camera.hpp>
#include <p3d/matrixstack.hpp>
#include <p3d/effects/opticconstants.hpp>
#include <p3d/effects/opticlensflare.hpp>

using namespace RadicalMathLibrary;

//*****************************************************************************
//
// Class tLensFlareGroup
//
//*****************************************************************************
tLensFlareGroup::tLensFlareGroup() :
    tOpticEffect(), 
    shader(NULL),
    sourceRadius(0.25f),
    edgeRadius(0.0f),
    numFlares(0)
{
}

tLensFlareGroup::~tLensFlareGroup()
{
    tRefCounted::Release(shader);
    for (int i = 0; i < numFlares; i++)
    {
        tRefCounted::Release(flares[i]);
    }
}

void
tLensFlareGroup::SetShader(tShader* shad)
{
    tRefCounted::Assign(shader,shad);
}

tLensFlare* 
tLensFlareGroup::GetLensFlareByIndex(int index) const
{   
    if ((index<numFlares)&&(index>=0))
    {
        return flares[index];
    }
    return NULL;
}

tLensFlare* 
tLensFlareGroup::GetLensFlareByName(const char* n) const
{   
    return GetLensFlareByUID(tEntity::MakeUID(n));
}

tLensFlare* 
tLensFlareGroup::GetLensFlareByUID(tUID uid) const
{
    for (int i = 0; i < numFlares; i++)
    {
        if (uid == flares[i]->GetUID())
        {
            return flares[i];
        }
    }
    return NULL;
}

void 
tLensFlareGroup::ProcessShaders(ShaderCallback& callback)
{
    for(int i = 0; i < numFlares; i++)
    {
        tShader* newShader = callback.Process(shader);
        if(newShader != shader)
        {
            tRefCounted::Assign(shader, newShader);
        }
    }
}

void 
tLensFlareGroup::DisplayEffect()
{
    if (numFlares==0)
        return;

    tCamera* camera = p3d::context->GetView()->GetCamera();
    if (!camera)
        return;

    Vector sourceCameraPos;
    Vector sourceWorldPos;
    Vector sourceViewPos;
    sourceWorldPos = p3d::context->GetWorldMatrix()->Row(3);
    camera->WorldToCamera(sourceWorldPos, &sourceCameraPos);
    camera->CameraToView(sourceCameraPos, &sourceViewPos);

    float intensity = 1.0f;
    float totalRadius = sourceRadius + edgeRadius;

    if (camera->SphereVisibleCamera(sourceCameraPos, totalRadius))
    {
        // calculate screen distance from view centre to source centre
        float distance = Sqrt((sourceViewPos.x * sourceViewPos.x) + (sourceViewPos.y * sourceViewPos.y));

        if (distance > totalRadius)
        {
            intensity = 0.0f;
        }
        else if (distance > sourceRadius)
        {
            intensity = 1.0f - ((distance-sourceRadius)/edgeRadius);
        }
        else
        {
            intensity = 1.0f;
        }
    }
    else
    {
        intensity = 0.0f;
    }

    p3d::stack->PushIdentity();
    pddiPrimStream* stream = p3d::pddi->BeginPrims(shader->GetShader(), PDDI_PRIM_TRIANGLES, PDDI_V_CT, 6 * numFlares);

    for (int i = 0; i < numFlares; i++)
    {
        flares[i]->DisplayFlare(stream,sourceCameraPos,intensity);
    }

    p3d::pddi->EndPrims(stream);
    p3d::stack->Pop();
}

//*****************************************************************************
//
// Class tLensFlare
//
//*****************************************************************************
tLensFlare::tLensFlare() :
    width(0.5f),
    height(0.5f),
    distance(0.0f)
{
    colour.Set(255,255,255,255);
    
    //bottom left
    uv[0].u = 0.0f;
    uv[0].v = 0.0f;
    //bottom right
    uv[1].u = 0.0f;
    uv[1].v = 1.0f;
    //top right
    uv[2].u = 1.0f;
    uv[2].v = 1.0f;
    //top left
    uv[3].u = 1.0f;
    uv[3].v = 0.0f;
    //uv offset
    uvOffset.u = 0.0f;
    uvOffset.v = 0.0f;
}

tLensFlare::~tLensFlare()
{
}

void 
tLensFlare::DisplayFlare(pddiPrimStream* stream, Vector position, float intensity)
{
    if (!stream)
        return;
    pddiColour modulatedColour;
    modulatedColour.SetRed(FtoL(intensity * (float)colour.Red()));
    modulatedColour.SetGreen(FtoL(intensity * (float)colour.Green()));
    modulatedColour.SetBlue(FtoL(intensity * (float)colour.Blue()));
    modulatedColour.SetAlpha(FtoL(intensity * (float)colour.Alpha()));

    // calculate vertex positions
    float x = width * position.z;
    float y = height * position.z;

    rmt::Vector flareCentre = position;
    flareCentre.x -= position.x * distance;
    flareCentre.y -= position.y * distance;

    pddiVector vertex0(flareCentre.x-x,flareCentre.y-y,flareCentre.z);
    pddiVector vertex1(flareCentre.x+x,flareCentre.y-y,flareCentre.z);
    pddiVector vertex2(flareCentre.x+x,flareCentre.y+y,flareCentre.z);
    pddiVector vertex3(flareCentre.x-x,flareCentre.y+y,flareCentre.z);
    pddiVector2 uv0(uv[0].u+uvOffset.u,uv[0].v+uvOffset.v);
    pddiVector2 uv1(uv[1].u+uvOffset.u,uv[1].v+uvOffset.v);
    pddiVector2 uv2(uv[2].u+uvOffset.u,uv[2].v+uvOffset.v);
    pddiVector2 uv3(uv[3].u+uvOffset.u,uv[3].v+uvOffset.v);

    stream->Vertex(&vertex0, modulatedColour, &uv0);
    stream->Vertex(&vertex2, modulatedColour, &uv2);
    stream->Vertex(&vertex1, modulatedColour, &uv1);
    stream->Vertex(&vertex0, modulatedColour, &uv0);
    stream->Vertex(&vertex3, modulatedColour, &uv3);
    stream->Vertex(&vertex2, modulatedColour, &uv2);
}


