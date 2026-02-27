//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//-----------------------------------------------------------------------------
// shadow.cpp
//
// Stencil bufffer shadow rendering
//
//-----------------------------------------------------------------------------

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

#if defined (RAD_WIN32) || defined(RAD_UWP)
#define CARMACK_REVERSE // use inverted z-buffer mode 
#define DRAW_CAPS // draw caps on volumes
#endif

const unsigned short NO_NEIGHBOUR = 0xffff;
static const tColour s_VolumeColour(255,0,0,0);

static const unsigned MAX_EDGES     = 2048;
static const unsigned MAX_CAP_POLYS = 2048;

using namespace RadicalMathLibrary;

struct DrawEdge
{
    short a;
    short b;
} s_DrawEdges[MAX_EDGES];

unsigned s_NumDrawEdges;

struct DrawCapPolys
{
    short a;
    short b;
    short c;
} s_DrawCapPolys[MAX_CAP_POLYS];
unsigned s_NumDrawCapPolys;

tShader* s_volumeShader = NULL;

/*
#include<radtime.hpp>
unsigned timeA, timeB, timeC, timeD, timeE, timeF, timeG;
*/

static int VertsToPrims(pddiPrimType type, int verts)
{
    switch(type)
    {
        case PDDI_PRIM_TRIANGLES:
            return verts/3;
        case PDDI_PRIM_TRISTRIP:
            return verts - 2;
        case PDDI_PRIM_LINES:
            return verts / 2;
        case PDDI_PRIM_LINESTRIP:
            return verts - 1;
        case PDDI_PRIM_POINTS:
            return verts;
        default:
            P3DASSERT(0);
            return 0;
    }
    return 0;
}


enum ShadowVolPass
{
    PASS_FRONT,
    PASS_BACK
};

static inline void SetupPass(ShadowVolPass pass, tShader* shader)
{
    p3d::pddi->SetCullMode( (pass == PASS_FRONT) ? PDDI_CULL_SHADOW_BACKFACE : PDDI_CULL_SHADOW_FRONTFACE );
    shader->SetInt( PDDI_SP_BLENDMODE, (pass == PASS_FRONT) ? PDDI_BLEND_ADD : PDDI_BLEND_SUBTRACT);

    #ifdef CARMACK_REVERSE
    p3d::pddi->SetCullMode((pass == PASS_FRONT) ? PDDI_CULL_INVERTED :  PDDI_CULL_NORMAL);
    #else
    p3d::pddi->SetCullMode((pass == PASS_FRONT) ? PDDI_CULL_NORMAL : PDDI_CULL_INVERTED );
    #endif

    p3d::pddi->SetStencilOp(PDDI_STENCIL_KEEP, PDDI_STENCIL_KEEP, (pass == PASS_FRONT) ? PDDI_STENCIL_INCR : PDDI_STENCIL_DECR);
}

tShadowGenerator::tShadowGenerator() :
    washShader(NULL),
    volumeShader(NULL)
{
    volumeShader = new tShader();
    volumeShader->AddRef();
    volumeShader->SetInt(PDDI_SP_SHADEMODE, PDDI_SHADE_FLAT);
    volumeShader->SetInt(PDDI_SP_ALPHATEST , false);
    volumeShader->SetInt(PDDI_SP_BLENDMODE , PDDI_BLEND_ALPHA);

    washShader = new tShader();
    washShader->AddRef();
   
    //Create the shadow wash shader
    washShader->SetInt(PDDI_SP_ISLIT, 0);
    washShader->SetInt(PDDI_SP_SHADEMODE, PDDI_SHADE_FLAT);
    washShader->SetInt(PDDI_SP_ALPHATEST, false );
    washShader->SetInt(PDDI_SP_BLENDMODE, PDDI_BLEND_MODULATE);
}

tShadowGenerator::~tShadowGenerator()
{
    tRefCounted::Release(washShader);
}

void tShadowGenerator::SetWashColour(tColour colour)
{
    washColour = colour;
}

void tShadowGenerator::Begin()
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


void tShadowGenerator::End()
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

    stream->Colour(washColour);stream->Coord(0,0,1);
    stream->Colour(washColour);stream->Coord(1,0,1);
    stream->Colour(washColour);stream->Coord(0,1,1);
    stream->Colour(washColour);stream->Coord(1,1,1);
    
    pddi->EndPrims(stream);

    p3d::stack->Pop();

    // Restore the render state
    pddi->SetProjectionMode(PDDI_PROJECTION_PERSPECTIVE);
    pddi->EnableZBuffer(true);
    pddi->SetZWrite(true);
    pddi->SetZCompare(PDDI_COMPARE_LESSEQUAL);
    pddi->EnableStencilBuffer(false);
}

void tShadowGenerator::PreRender()
{

}

/*****************************************************************************
 * tShadowMesh
 ****************************************************************************/
tShadowMesh::tShadowMesh() :
    topology(NULL),
    vertices(NULL),
    normals(NULL),
    triCulled(NULL),
    volumeLength(0.0f),
    setVolumeLength(0.0f)
{


}

tShadowMesh::~tShadowMesh()
{
    delete[] topology;
    delete[] vertices;
    delete[] normals;
    delete[] triCulled;
}

void tShadowMesh::Display()
{
    P3DASSERT(s_volumeShader);

//    timeA = radTimeGetMicroseconds();
    // transform light to object space
    Matrix w2o = *p3d::context->GetWorldMatrix();
    w2o.Invert();
    if(isPointLight)
    {
        w2o.Transform(worldLight, &light);
    }
    else
    {
        w2o.RotateVector(worldLight, &light);
    }

    GuessVolumeLength();

    //Update the shadow edge
    UpdateEdges();

//    timeB = radTimeGetMicroseconds();
//    timeA = timeB-timeA;

    // draw the front facing pass
    SetupPass(PASS_FRONT, s_volumeShader);

    DrawShadowVolume();

//    timeC = radTimeGetMicroseconds();
//    timeB = timeC-timeB;

    DrawCaps();

//    timeD = radTimeGetMicroseconds();
//    timeC = timeD-timeC;

    // draw the inverted pass
    SetupPass(PASS_BACK, s_volumeShader);

    // Draw back-side of shadow volume in stencil/z only
    // instance->RenderInstance( obj );
    DrawShadowVolume();

//    timeE = radTimeGetMicroseconds();
//    timeD = timeE-timeD;

    DrawCaps();
    // Reset the volume length to be recalculated next
    //frame.
    volumeLength = 0.0f;
//    timeE = radTimeGetMicroseconds() - timeE;
}


void tShadowMesh::UpdateEdges()
{
    P3DASSERT(vertices);
    P3DASSERT(normals);
    unsigned short i;

    //Update back face culling information (from pov of light source)
    if(!isPointLight)
    {
        for(i=0;i<numTriangles;i++)
        {
            triCulled[i] = (normals[i].DotProduct(light) < 0);
        }
    }
    else
    {
        Vector pos = light;
        for(i=0;i<numTriangles;i++)
        {
            Vector cull;
            cull.Sub(pos, vertices[topology[i].v0]);
            triCulled[i] = (normals[i].DotProduct(cull) > 0);
        }
    }

    //Compute edge list
    s_NumDrawCapPolys = 0;
    s_NumDrawEdges = 0;
    for(i=0;i<numTriangles;i++)
    {
        if(!triCulled[i])
        {
            P3DASSERT(s_NumDrawCapPolys < MAX_CAP_POLYS);
            s_DrawCapPolys[s_NumDrawCapPolys].a = topology[i].v0;
            s_DrawCapPolys[s_NumDrawCapPolys].b = topology[i].v1;
            s_DrawCapPolys[s_NumDrawCapPolys].c = topology[i].v2;

            s_NumDrawCapPolys++;

            if((topology[i].n0 == (unsigned short)-1) ||
               (triCulled[topology[i].n0]))
            {
                P3DASSERT(s_NumDrawEdges < MAX_EDGES);
                s_DrawEdges[s_NumDrawEdges].b = topology[i].v0;
                s_DrawEdges[s_NumDrawEdges].a = topology[i].v1;
                s_NumDrawEdges++;
            }
            if((topology[i].n1 == (unsigned short)-1) ||
               (triCulled[topology[i].n1]))
            {
                P3DASSERT(s_NumDrawEdges < MAX_EDGES);
                s_DrawEdges[s_NumDrawEdges].b = topology[i].v1;
                s_DrawEdges[s_NumDrawEdges].a = topology[i].v2;
                s_NumDrawEdges++;
            }
            if((topology[i].n2 == (unsigned short)-1) ||
               (triCulled[topology[i].n2]))
            {
                P3DASSERT(s_NumDrawEdges < MAX_EDGES);
                s_DrawEdges[s_NumDrawEdges].b = topology[i].v2;
                s_DrawEdges[s_NumDrawEdges].a = topology[i].v0;
                s_NumDrawEdges++;
            }
        }
    }

}

// render the caps on the front and rear of the shadow volume
void tShadowMesh::DrawCaps()
{
#ifdef DRAW_CAPS
    P3DASSERT(s_volumeShader);

    unsigned short i;
    Vector shadow = light;
    shadow.Scale(volumeLength);

    pddiPrimStream* stream = p3d::pddi->BeginPrims(s_volumeShader->GetShader(), PDDI_PRIM_TRIANGLES, PDDI_V_C, 6 * s_NumDrawCapPolys);

    if(!isPointLight)
    {
        for(i=0; i<s_NumDrawCapPolys;i++)
        {
            Vector a, b, c, d, e, f;
            a = vertices[s_DrawCapPolys[i].a];
            b = vertices[s_DrawCapPolys[i].b];
            c = vertices[s_DrawCapPolys[i].c];

            // draw front cap poly (winding order is reversed)
            stream->Colour(s_VolumeColour); stream->Coord(c.x, c.y, c.z);
            stream->Colour(s_VolumeColour); stream->Coord(b.x, b.y, b.z);
            stream->Colour(s_VolumeColour); stream->Coord(a.x, a.y, a.z);

            // calculate rear cap position
            a.Add(shadow);
            b.Add(shadow);
            c.Add(shadow);

            // draw rear cap
            stream->Colour(s_VolumeColour); stream->Coord(a.x, a.y, a.z);
            stream->Colour(s_VolumeColour); stream->Coord(b.x, b.y, b.z);
            stream->Colour(s_VolumeColour); stream->Coord(c.x, c.y, c.z);
        }
    }
    else
    {
        for(i=0; i<s_NumDrawCapPolys;i++)
        {
            Vector a, b, c, d, e, f;
            a = vertices[s_DrawCapPolys[i].a];
            b = vertices[s_DrawCapPolys[i].b];
            c = vertices[s_DrawCapPolys[i].c];

            // draw front cap poly (winding order is reversed)
            stream->Colour(s_VolumeColour); stream->Coord(c.x, c.y, c.z);
            stream->Colour(s_VolumeColour); stream->Coord(b.x, b.y, b.z);
            stream->Colour(s_VolumeColour); stream->Coord(a.x, a.y, a.z);

            // calculate rear cap position
            d.Sub(a, light);
            e.Sub(b, light);
            f.Sub(c, light);
            d.Scale(volumeLength);
            e.Scale(volumeLength);
            f.Scale(volumeLength);
            a.Add(d);
            b.Add(e);
            c.Add(f);

            // draw rear cap
            stream->Colour(s_VolumeColour); stream->Coord(a.x, a.y, a.z);
            stream->Colour(s_VolumeColour); stream->Coord(b.x, b.y, b.z);
            stream->Colour(s_VolumeColour); stream->Coord(c.x, c.y, c.z);
        }
    }
    p3d::pddi->EndPrims(stream);
#endif
}


//Render the shadow volume 
void tShadowMesh::DrawShadowVolume()
{
    unsigned short i;
    P3DASSERT(s_volumeShader);

    pddiPrimStream* stream;
    stream = p3d::pddi->BeginPrims(s_volumeShader->GetShader(), PDDI_PRIM_TRIANGLES, PDDI_V_C, 6 * s_NumDrawEdges);

    //Set the shadow volume colour. It would be better to have this as a shader constant,
    //so we wouldn't have to send this data over the bus many times.
    s_volumeShader->SetColour(PDDI_SP_DIFFUSE, s_VolumeColour);

    Vector s;
    if(!isPointLight)
    {
        s = light;
        s.Scale(volumeLength);

        // Draw the shadow
        for(i=0;i<s_NumDrawEdges;i++)
        {
            Vector& va = vertices[s_DrawEdges[i].a];
            Vector& vb = vertices[s_DrawEdges[i].b];

            pddiVector a,b,c,d;
            a.Set(va.x, va.y, va.z);
            b.Set(vb.x, vb.y, vb.z);
            c.Set(va.x+s.x, va.y+s.y, va.z+s.z);
            d.Set(vb.x+s.x, vb.y+s.y, vb.z+s.z);

            //Would be better as a quad. Also could consider sending multiple 
            //sets of tri's to try and get better fill rate(since the triangles would be
            //less ponty). Or could pre create something indexed.
            stream->Vertex(&a, s_VolumeColour);
            stream->Vertex(&c, s_VolumeColour);
            stream->Vertex(&b, s_VolumeColour);

            stream->Vertex(&b, s_VolumeColour);
            stream->Vertex(&c, s_VolumeColour);
            stream->Vertex(&d, s_VolumeColour);
        }
    }
    else
    {
        s = light;

        //Draw the shadow
        for(i=0;i<s_NumDrawEdges;i++)
        {
            Vector& va = vertices[s_DrawEdges[i].a];
            Vector& vb = vertices[s_DrawEdges[i].b];

            pddiVector a,b,c,d;
            a.Set(va.x, va.y, va.z);
            b.Set(vb.x, vb.y, vb.z);
            Vector sa, sb;
            sa.Sub(va, s);sb.Sub(vb, s);
            sa.Normalize();sb.Normalize();
            sa.Scale(5.0f);sb.Scale(5.0f);
        
            c.Set(va.x+sa.x, va.y+sa.y, va.z+sa.z);
            d.Set(vb.x+sb.x, vb.y+sb.y, vb.z+sb.z);

            //Would be better as a quad. Also could consider sending multiple 
            //sets of tri's to try and get better fill rate(since the triangles would be
            //less ponty). Or could pre create something indexed.
            stream->Vertex(&a, s_VolumeColour);
            stream->Vertex(&c, s_VolumeColour);
            stream->Vertex(&b, s_VolumeColour);

            stream->Vertex(&b, s_VolumeColour);
            stream->Vertex(&c, s_VolumeColour);
            stream->Vertex(&d, s_VolumeColour);
        }
    }

    p3d::pddi->EndPrims(stream);
}

void tShadowMesh::DrawNormals(void)
{
    pddiPrimStream* stream = p3d::pddi->BeginPrims(NULL, PDDI_PRIM_LINES, PDDI_V_C, 2 * numTriangles);
    tColour white(255,255,255,255);
    
    for(unsigned i = 0; i < numTriangles; i++)
    {
        rmt::Vector centre;
        centre.Interpolate(vertices[topology[i].v0], vertices[topology[i].v1], 0.5f);
        centre.Interpolate(centre, vertices[topology[i].v2], 0.5f);
        
        stream->Colour(white);
        stream->Coord(centre.x, centre.y, centre.z);

        centre.Add(normals[i]);

        stream->Colour(white);
        stream->Coord(centre.x, centre.y, centre.z);
    }

    p3d::pddi->EndPrims(stream);
}


void tShadowMesh::DrawShell(void)
{
    pddiPrimStream* stream = p3d::pddi->BeginPrims(NULL, PDDI_PRIM_TRIANGLES, PDDI_V_C, 3 * numTriangles);
    tColour blue(0,0,255,255);
    
    for(unsigned i = 0; i < numTriangles; i++)
    {
        stream->Colour(blue);
        stream->Coord(vertices[topology[i].v0].x, vertices[topology[i].v0].y, vertices[topology[i].v0].z);

        stream->Colour(blue);
        stream->Coord(vertices[topology[i].v1].x, vertices[topology[i].v1].y, vertices[topology[i].v1].z);

        stream->Colour(blue);
        stream->Coord(vertices[topology[i].v2].x, vertices[topology[i].v2].y, vertices[topology[i].v2].z);
    }

    p3d::pddi->EndPrims(stream);
}

void tShadowMesh::GuessVolumeLength(void)
{
    // if someone has set a volume length, use it
    if(setVolumeLength != 0.0f)
    {
        volumeLength = setVolumeLength;
        return;
    }
    // If the volume lenght has been precalculated
    //earlier in the display process we can leave.
    if(volumeLength != 0.0f)
    {
        return;
    }
    
    // otherwise try to guess
    // note : this code is not even remotley correct, it works sometimes, 
    // but needs dramatic improvment
    tCamera* camera = p3d::context->GetView()->GetCamera();
    Vector cameraLight;
    Sphere tmpSphere = boundingSphere;
    p3d::context->ObjectToWorld(boundingSphere.centre, &tmpSphere.centre);
    camera->WorldToCamera(tmpSphere.centre, &tmpSphere.centre);

    if(!isPointLight)
    {
        camera->GetWorldToCameraMatrix().RotateVector(worldLight, &cameraLight);
    }
    else
    {
        Vector tmp;
        camera->WorldToCamera(worldLight, &tmp);
        cameraLight = tmpSphere.centre;
        cameraLight.Sub(tmp);
        cameraLight.Normalize();
    }

    unsigned i;

    for(i = 1; i < 100; ++i)
    {
        tmpSphere.centre.Add(cameraLight);
        if(!camera->SphereVisibleCamera(tmpSphere.centre, tmpSphere.radius))
        {
            break;
        }
    }

    volumeLength = (float)i;
}

//
// tShadowSkin
// 
tShadowSkin::tShadowSkin() :
    skeleton(NULL),
    boneMatrix(NULL),
    m_pSourceVertexOne(NULL),
    m_pSourceVertexTwo(NULL),
    m_pSourceVertexThree(NULL),
    numVertexOne(0),
    numVertexTwo(0),
    numVertexThree(0),
    pose(NULL)
{
}

tShadowSkin::~tShadowSkin()
{
    tRefCounted::Release(skeleton);
    tRefCounted::Release(pose);
    delete[] boneMatrix;
    delete[] m_pSourceVertexOne;
    delete[] m_pSourceVertexTwo;
    delete[] m_pSourceVertexThree;
}

void tShadowSkin::Display() 
{ 
    if(!pose) 
    {
        tPose* p = skeleton->NewPose();
        tRefCounted::Assign(pose, p);
    }

    Display(pose);
}

void tShadowSkin::Display(tPose* p)
{
    if(!p) 
    {
        return;
    }

    unsigned int i;

//    timeF = radTimeGetMicroseconds();
    // transform the pose joints into world space
    p->Evaluate();

    bool poseChanged = false;

    // The skin vertices are stored in rest-pose relative world space.
    // The below matrix will transform from this space, into bone-local
    // space where the animation is applied, and then back into camera space.
    unsigned numJoints = rmt::Min( skeleton->GetNumJoint(), p->GetNumJoint() );
    for(i = 0; i < numJoints; ++i)
    {
        Matrix tmp;

        tmp.Mult(skeleton->GetJoint(i)->inverseWorldMatrix, p->GetJoint(i)->worldMatrix);
        if(!poseChanged)
        {
            for(unsigned j = 0; j < 4; j++)
            {
                for(unsigned k = 0; k < 4; k++)
                {
                    if(boneMatrix[i].m[j][k] != tmp.m[j][k])
                    {
                        poseChanged = true;
                        goto poseHasChanged;
                    }
                }
            }
poseHasChanged: ;
        }

        boneMatrix[i] = tmp;
    }

    if(poseChanged)
    {
        P3DASSERT(numVertices == numVertexOne + numVertexTwo + numVertexThree);
        rmt::Vector* pOut = vertices;
        Vector vertex;    //Vertex accumulator
        Vector* sv;
        float weight;
        Matrix* m;
        for(i=0; i<numVertexThree;i++)
        {
            //Fetch source vertex information
            VertexThree* pVertex = &(m_pSourceVertexThree[i]);
            sv = &pVertex->m_Position;

            // compute weighted skin co-ordinate
            weight = pVertex->m_Weights[0];
            m = &boneMatrix[pVertex->m_Index[0]];
            vertex.x = (m->m[0][0]*sv->x + m->m[1][0]*sv->y + m->m[2][0]*sv->z + m->m[3][0]) * weight;
            vertex.y = (m->m[0][1]*sv->x + m->m[1][1]*sv->y + m->m[2][1]*sv->z + m->m[3][1]) * weight;
            vertex.z = (m->m[0][2]*sv->x + m->m[1][2]*sv->y + m->m[2][2]*sv->z + m->m[3][2]) * weight;

            weight = pVertex->m_Weights[1];
            m = &boneMatrix[pVertex->m_Index[1]];
            vertex.x += (m->m[0][0]*sv->x + m->m[1][0]*sv->y + m->m[2][0]*sv->z + m->m[3][0]) * weight;
            vertex.y += (m->m[0][1]*sv->x + m->m[1][1]*sv->y + m->m[2][1]*sv->z + m->m[3][1]) * weight;
            vertex.z += (m->m[0][2]*sv->x + m->m[1][2]*sv->y + m->m[2][2]*sv->z + m->m[3][2]) * weight;

            weight = pVertex->m_Weights[2];
            m = &boneMatrix[pVertex->m_Index[2]];
            vertex.x += (m->m[0][0]*sv->x + m->m[1][0]*sv->y + m->m[2][0]*sv->z + m->m[3][0]) * weight;
            vertex.y += (m->m[0][1]*sv->x + m->m[1][1]*sv->y + m->m[2][1]*sv->z + m->m[3][1]) * weight;
            vertex.z += (m->m[0][2]*sv->x + m->m[1][2]*sv->y + m->m[2][2]*sv->z + m->m[3][2]) * weight;

            // output transformed vertex
            pOut[pVertex->vertexIndex] = vertex;
        }
        for(i=0; i<numVertexTwo;i++)
        {
            //Fetch source vertex information
            VertexTwo* pVertex = &(m_pSourceVertexTwo[i]);
            sv = &pVertex->m_Position;

            // compute weighted skin co-ordinate
            weight = pVertex->m_Weights[0];
            m = &boneMatrix[pVertex->m_Index[0]];
            vertex.x = (m->m[0][0]*sv->x + m->m[1][0]*sv->y + m->m[2][0]*sv->z + m->m[3][0]) * weight;
            vertex.y = (m->m[0][1]*sv->x + m->m[1][1]*sv->y + m->m[2][1]*sv->z + m->m[3][1]) * weight;
            vertex.z = (m->m[0][2]*sv->x + m->m[1][2]*sv->y + m->m[2][2]*sv->z + m->m[3][2]) * weight;

            weight = pVertex->m_Weights[1];
            m = &boneMatrix[pVertex->m_Index[1]];
            vertex.x += (m->m[0][0]*sv->x + m->m[1][0]*sv->y + m->m[2][0]*sv->z + m->m[3][0]) * weight;
            vertex.y += (m->m[0][1]*sv->x + m->m[1][1]*sv->y + m->m[2][1]*sv->z + m->m[3][1]) * weight;
            vertex.z += (m->m[0][2]*sv->x + m->m[1][2]*sv->y + m->m[2][2]*sv->z + m->m[3][2]) * weight;

            // output transformed vertex
            pOut[pVertex->vertexIndex] = vertex;
        }
        for(i=0; i<numVertexOne;i++)
        {
            //Fetch source vertex information
            VertexOne* pVertex = &(m_pSourceVertexOne[i]);
            m = &boneMatrix[pVertex->m_Index[0]];

            // compute weighted skin co-ordinate
            sv = &pVertex->m_Position;
            vertex.x = (m->m[0][0]*sv->x + m->m[1][0]*sv->y + m->m[2][0]*sv->z + m->m[3][0]);
            vertex.y = (m->m[0][1]*sv->x + m->m[1][1]*sv->y + m->m[2][1]*sv->z + m->m[3][1]);
            vertex.z = (m->m[0][2]*sv->x + m->m[1][2]*sv->y + m->m[2][2]*sv->z + m->m[3][2]);

            // output transformed vertex
            pOut[pVertex->vertexIndex] = vertex;
        }


//        timeG = radTimeGetMicroseconds();
//        timeF = timeG-timeF;

       //Update per triangle normal
        Vector vAB, vBC;
        for(i=0; i<numTriangles;i++)
        {
            Vector &a = vertices[topology[i].v0];
            Vector &b = vertices[topology[i].v1];
            Vector &c = vertices[topology[i].v2];

            //Optimization fodder
            vAB.Sub(b, a);    
            vBC.Sub(c, b);
            normals[i].CrossProduct(vAB, vBC);
        }
    }
    else
    {
//        timeG = radTimeGetMicroseconds();
//        timeF = timeG-timeF;
    }

//    timeG = radTimeGetMicroseconds() - timeG;
    GuessVolumeLength( p );
    tShadowMesh::Display();
}

void tShadowSkin::GuessVolumeLength( tPose* Pose )
{
    // if someone has set a volume length, use it
    if( setVolumeLength != 0.0f )
    {
        volumeLength = setVolumeLength;
        return;
    }
    // The length has been precalculated somewhere else.
    if( ( volumeLength != 0.0f ) || ( Pose == 0 ) )
    {
        return;
    }

    // otherwise try to guess
    // note : this code is perfect... :)
    tCamera* camera = p3d::context->GetView()->GetCamera();
    Vector cameraLight;
    Sphere tmpSphere = boundingSphere;
    const rmt::Vector& worldPos = Pose->GetJoint( 0 )->worldMatrix.Row( 3 );
    tmpSphere.centre.Add( worldPos );
    camera->WorldToCamera(tmpSphere.centre, &tmpSphere.centre);

    if(!isPointLight)
    {
        camera->GetWorldToCameraMatrix().RotateVector(worldLight, &cameraLight);
    }
    else
    {
        Vector tmp;
        camera->WorldToCamera(worldLight, &tmp);
        cameraLight = tmpSphere.centre;
        cameraLight.Sub(tmp);
        cameraLight.Normalize();
    }

    unsigned i;

    for(i = 1; i < 100; ++i)
    {
        tmpSphere.centre.Add(cameraLight);
        if(!camera->SphereVisibleCamera(tmpSphere.centre, tmpSphere.radius))
        {
            break;
        }
    }

    volumeLength = (float)i;
}

//-------------------------------------------------------------------
static const int SHADOWMESH_VERSION = 0;

tShadowMeshLoader::tShadowMeshLoader() : tSimpleChunkHandler(Pure3D::Mesh::SHADOWMESH) {};

tEntity* tShadowMeshLoader::LoadObject(tChunkFile* f, tEntityStore* store)
{
    unsigned int i;

    char name[128];
    int version;
    unsigned long numVertices;
    unsigned long numTriangles;

    //Read the chunk header information
    f->GetPString(name);
        version = f->GetLong();
    numVertices = f->GetLong();
    numTriangles = f->GetLong();

    //Some basic sanity checking
    P3DASSERT(version == SHADOWMESH_VERSION);

    //Create the tShadowSkin
    tShadowMesh *shadow = Allocate();
    shadow->SetName(name);
    shadow->numTriangles = numTriangles;
    shadow->numVertices = numVertices;

    //Preallocate the static shadow data
    shadow->topology = new tShadowMesh::Topology[numTriangles];

    //Allocate space for the dynamic data
    shadow->vertices = new Vector[numVertices];
    shadow->normals = new Vector[numTriangles];
    shadow->triCulled = new bool[numTriangles];

    int index = 0;

    while(f->ChunksRemaining())
    {
        f->BeginChunk();
        switch(f->GetCurrentID())
        {
            case Pure3D::Mesh::TOPOLOGY:
            {
                unsigned int count = (unsigned int) f->GetLong();
                P3DASSERT(count == shadow->numTriangles);
                P3DASSERT(shadow->topology);
                for(i=0; i<count; i++)
                {
                   f->GetData(&shadow->topology[i], 6, tFile::WORD);
                }
                break;
            }
            case Pure3D::Mesh::POSITIONLIST:
            {
                unsigned int count = (unsigned int) f->GetLong();
                P3DASSERT(count == numVertices);
                P3DASSERT(shadow->vertices);
                for(i=0; i<count; i++)
                {
                   f->GetData(&shadow->vertices[i], 3, tFile::DWORD);
                }
                break;
            }
            case Pure3D::Mesh::SPHERE:
            {
                shadow->boundingSphere.centre.x = f->GetFloat();
                shadow->boundingSphere.centre.y = f->GetFloat();
                shadow->boundingSphere.centre.z = f->GetFloat();
                shadow->boundingSphere.radius = f->GetFloat();
                break;
            }
            case Pure3D::Mesh::BOX:
            {
                shadow->boundingBox.low.x = f->GetFloat();
                shadow->boundingBox.low.y = f->GetFloat();
                shadow->boundingBox.low.z = f->GetFloat();
                shadow->boundingBox.high.x = f->GetFloat();
                shadow->boundingBox.high.y = f->GetFloat();
                shadow->boundingBox.high.z = f->GetFloat();
                break;
            }
            default:
                break;
        }
        f->EndChunk();
    }

    //calculate normals
    Vector vAB, vBC;
    for(i=0; i<shadow->numTriangles;i++)
    {
        Vector &a = shadow->vertices[shadow->topology[i].v0];
        Vector &b = shadow->vertices[shadow->topology[i].v1];
        Vector &c = shadow->vertices[shadow->topology[i].v2];

        vAB.Sub(b, a);    
        vBC.Sub(c, b);
        shadow->normals[i].CrossProduct(vAB, vBC);
    }

    return shadow;
}

tShadowMesh* tShadowMeshLoader::Allocate()
{
    return new tShadowMesh();
}

//-------------------------------------------------------------------
static const int SHADOWSKIN_VERSION = 0;

tShadowSkinLoader::tShadowSkinLoader() : tSimpleChunkHandler(Pure3D::Mesh::SHADOWSKIN) {};

tEntity* tShadowSkinLoader::LoadObject(tChunkFile* f, tEntityStore* store)
{
    unsigned int i;

    char name[128];
    int version;
    char skelName[128];
    unsigned long numVertices;
    unsigned long numTriangles;

    //Read the chunk header information
    f->GetPString(name);
        version = f->GetLong();
    f->GetPString(skelName);
    numVertices = f->GetLong();
    numTriangles = f->GetLong();

    //Some basic sanity checking
    P3DASSERT(version == SHADOWSKIN_VERSION);

    //Create the tShadowSkin
    tShadowSkin *shadow = Allocate();
    shadow->SetName(name);
    shadow->numTriangles = numTriangles;
    shadow->numVertices = numVertices;

    // Temp buffer for holding vertex information.
    tShadowSkin::VertexThree* sourceVertexThree = (tShadowSkin::VertexThree*)p3d::MallocTemp(sizeof(tShadowSkin::VertexThree) * numVertices );
    P3DASSERT( sourceVertexThree );

    //Preallocate the static shadow data
    shadow->topology = new tShadowSkin::Topology[numTriangles];

    //Allocate space for the dynamic data
    shadow->vertices= new Vector[numVertices];
    shadow->normals = new Vector[numTriangles];
    shadow->triCulled = new bool[numTriangles];

    //Setup the skeleton
    shadow->skeleton = p3d::find<tSkeleton>(store, skelName);
    if(shadow->skeleton)
    {
        shadow->skeleton->AddRef();
        shadow->pose = shadow->skeleton->NewPose();
        shadow->pose ->AddRef();
        // allocate one extra bone for the special identity matrix
        shadow->boneMatrix = new rmt::Matrix[shadow->pose->GetNumJoint()+1];
        shadow->boneMatrix[shadow->pose->GetNumJoint()].Identity();
    }
    int index = 0;

    while(f->ChunksRemaining())
    {
        f->BeginChunk();
        switch(f->GetCurrentID())
        {
            case Pure3D::Mesh::TOPOLOGY:
            {
                unsigned int count = (unsigned int) f->GetLong();
                P3DASSERT(count == shadow->numTriangles);
                P3DASSERT(shadow->topology);
                for(i=0; i<count; i++)
                {
                   f->GetData(&shadow->topology[i], 6, tFile::WORD);
                }
                break;
            }
            case Pure3D::Mesh::POSITIONLIST:
            {
                unsigned int count = (unsigned int) f->GetLong();
                P3DASSERT(count == numVertices);

                for(i=0; i<count; i++)
                {
                    f->GetData(&sourceVertexThree[i].m_Position, 3, tFile::DWORD);
                }
                break;
            }
            case Pure3D::Mesh::WEIGHTLIST:
            {
                unsigned int count = (unsigned int) f->GetLong();
                P3DASSERT(count == numVertices);
                for(i=0; i<count; i++)
                {
                   f->GetData(&sourceVertexThree[i].m_Weights, 3, tFile::DWORD);
                }
                break;
            }
            case Pure3D::Mesh::MATRIXLIST:
            {
                unsigned int count = (unsigned int) f->GetLong();
                unsigned char tempIndex[4];
                P3DASSERT(count == numVertices);
                for(i=0; i<count; i++)
                {
                    //For some reason these seem to be in reverse order.
                    f->GetData(&tempIndex, 4, tFile::BYTE);
                    sourceVertexThree[i].m_Index[0] = tempIndex[3];
                    sourceVertexThree[i].m_Index[1] = tempIndex[2];
                    sourceVertexThree[i].m_Index[2] = tempIndex[1];
                    sourceVertexThree[i].m_Index[3] = tempIndex[0];
                }
                break;
            }
            case Pure3D::Mesh::SPHERE:
                shadow->boundingSphere.centre.x = f->GetFloat();
                shadow->boundingSphere.centre.y = f->GetFloat();
                shadow->boundingSphere.centre.z = f->GetFloat();
                shadow->boundingSphere.radius = f->GetFloat();
                break;

            case Pure3D::Mesh::BOX:
                shadow->boundingBox.low.x = f->GetFloat();
                shadow->boundingBox.low.y = f->GetFloat();
                shadow->boundingBox.low.z = f->GetFloat();
                shadow->boundingBox.high.x = f->GetFloat();
                shadow->boundingBox.high.y = f->GetFloat();
                shadow->boundingBox.high.z = f->GetFloat();
                break;

            default:
            break;
        }
        f->EndChunk();
    }

    for( i = 0; i < numVertices; ++i )
    {
        tShadowSkin::VertexThree* pSource = &( sourceVertexThree[ i ] );
        if( 0 == pSource->m_Weights[ 2 ] && 0 == pSource->m_Weights[ 1 ] )
        {
            // One bone.
            ++(shadow->numVertexOne);
        }
        else if( 0 == pSource->m_Weights[ 2 ] )
        {
            ++(shadow->numVertexTwo);
        }
        else
        {
            ++(shadow->numVertexThree);
        }
    }

    shadow->m_pSourceVertexOne = new tShadowSkin::VertexOne[ shadow->numVertexOne ];
    shadow->m_pSourceVertexTwo = new tShadowSkin::VertexTwo[ shadow->numVertexTwo ];
    shadow->m_pSourceVertexThree = new tShadowSkin::VertexThree[ shadow->numVertexThree ];

    int vertexOneIndex = 0;
    int vertexTwoIndex = 0;
    int vertexThreeIndex = 0;

    //Sort the vertices by number of weights
    for(i=0;i<numVertices;i++)
    {
        //Source vertex
        tShadowSkin::VertexThree* pSource = &(sourceVertexThree[i]);

        //Bucket according to number of bones.
        if(0==pSource->m_Weights[2] && 0==pSource->m_Weights[1])
        {
            //one bone
            tShadowSkin::VertexOne* pDest = &(shadow->m_pSourceVertexOne[ vertexOneIndex ]);
            pDest->m_Position = pSource->m_Position;
            pDest->vertexIndex = i;
            pDest->m_Index[0] = pSource->m_Index[0];
            ++vertexOneIndex;
        }
        else if(0==pSource->m_Weights[2])
        {
            //two bones
            tShadowSkin::VertexTwo* pDest = &(shadow->m_pSourceVertexTwo[ vertexTwoIndex]);
            pDest->m_Position = pSource->m_Position;
            pDest->vertexIndex = i;
            pDest->m_Index[0] = pSource->m_Index[0];
            pDest->m_Index[1] = pSource->m_Index[1];
            pDest->m_Weights[0] = pSource->m_Weights[0];
            pDest->m_Weights[1] = pSource->m_Weights[1];
            ++vertexTwoIndex;
        }
        else
        {
            //three bones
            tShadowSkin::VertexThree* pDest = &( shadow->m_pSourceVertexThree[ vertexThreeIndex ] );
            pDest->m_Position = pSource->m_Position;
            pDest->vertexIndex = i;
            pDest->m_Index[ 0 ] = pSource->m_Index[ 0 ];
            pDest->m_Index[ 1 ] = pSource->m_Index[ 1 ];
            pDest->m_Index[ 2 ] = pSource->m_Index[ 2 ];
            pDest->m_Weights[ 0 ] = pSource->m_Weights[ 0 ];
            pDest->m_Weights[ 1 ] = pSource->m_Weights[ 1 ];
            pDest->m_Weights[ 2 ] = pSource->m_Weights[ 2 ];
            ++vertexThreeIndex;
        }
    }

    P3DASSERT( (unsigned)vertexOneIndex == shadow->numVertexOne );
    P3DASSERT( (unsigned)vertexTwoIndex == shadow->numVertexTwo );
    P3DASSERT( (unsigned)vertexThreeIndex == shadow->numVertexThree );

    p3d::FreeTemp(sourceVertexThree);

    return shadow;
}

tShadowSkin* tShadowSkinLoader::Allocate()
{
    return new tShadowSkin();
}
