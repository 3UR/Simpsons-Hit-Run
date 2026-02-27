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
#include <pddi/pddiext.hpp>
#include <eeregs.h>

#ifdef PS2MW
#include <malloc.h>
#endif

tShader* tShadowGeneratorImpl::s_VolumeShader = NULL;

static const tColour s_VolumeColour(0x8,0x8,0x8,0 );

static const unsigned MAX_EDGES     = 4096;
static const unsigned MAX_CAP_POLYS = 4096;

using namespace RadicalMathLibrary;

struct DrawEdge
{
    short a;
    short b;
} s_DrawEdges[MAX_EDGES];

unsigned s_NumDrawEdges;
pddiExtPS2Control* ps2 = NULL;

//-----------------------------------------------------------------------------------
tShadowGeneratorImpl::tShadowGeneratorImpl()
{
    if (s_VolumeShader == NULL)
    {
        s_VolumeShader = new tShader("shadow");
        s_VolumeShader->AddRef();
    }

    ps2 = (pddiExtPS2Control*)p3d::pddi->GetExtension(PDDI_EXT_PS2_CONTROL);
}

//-----------------------------------------------------------------------------------
tShadowGeneratorImpl::~tShadowGeneratorImpl()
{
    tRefCounted::Release(s_VolumeShader);
}

//-----------------------------------------------------------------------------------
void tShadowGeneratorImpl::SetWashColour(tColour colour)
{
    washColour = colour;
}

//-----------------------------------------------------------------------------------
void tShadowGeneratorImpl::Begin()
{
    pddiRenderContext* pddi = p3d::pddi;

    pddi->SetZWrite(false);    

    // clear stencil buffer
    ps2->BeginStencilPass(0);
}


//-----------------------------------------------------------------------------------
void tShadowGeneratorImpl::End()
{
    pddiRenderContext* pddi = p3d::pddi;    
    ps2->SetShadowColour(washColour.Red(), washColour.Green(), washColour.Blue(), washColour.Alpha());
    ps2->EndStencilPass(0);
    pddi->SetZWrite(true);
    pddi->EnableStencilBuffer(false);
}

//-----------------------------------------------------------------------------------
void tShadowGeneratorImpl::PreRender()
{
    // nop
}


//-----------------------------------------------------------------------------------
void tShadowGeneratorImpl::SetupPass(ShadowVolPass pass)
{
    // nop
}

//=============================================================================
//tShadowMeshImpl
//=============================================================================
tShadowMeshImpl::tShadowMeshImpl() :
    topology(NULL),
    volumeLength(0.0f),
    setVolumeLength(0.0f),
    vertices(NULL),
    normals(NULL),
    triCulled(NULL)
{


}

//------------------------------------------------------------------------------------
tShadowMeshImpl::~tShadowMeshImpl()
{
    delete[] topology;
    delete[] vertices;
    delete[] normals;
    delete[] triCulled;
}

//------------------------------------------------------------------------------------
void tShadowMeshImpl::Display()
{
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

    UpdateEdges();

    // Draw the front and back facing tris in a single pass.
    // The VU program changes the tri's colour based on which way it faces.
    // If it is a front facing polygon, we set the colour to (1,1,1) (or (8,8,8) in
    // 16 bit mode). If it's back facing the colour is set to (255,255,255). We disable
    // colour clamping, which makes the additive blend of (255,255,255) the same
    // as a subtractive blend. This allows us to draw the shadows in a single pass.
    tShadowGeneratorImpl::SetupPass(tShadowGeneratorImpl::PASS_FRONT);
    DrawShadowVolume();
    //DrawCaps(); // not done on PS2 implementation

    // Reset the volume length to be recalculated next frame.
    volumeLength = 0.0f;
}


//------------------------------------------------------------------------------------
void tShadowMeshImpl::UpdateEdges()
{
    P3DASSERT(vertices);
    P3DASSERT(normals);
    unsigned short i;

    //Update back face culling information (from pov of light source)
    if (!isPointLight)
    {
        for (i = 0; i < numTriangles ; i++)
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
    s_NumDrawEdges = 0;
    for(i=0;i<numTriangles;i++)
    {
        if(!triCulled[i])
        {
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

//------------------------------------------------------------------------------------
// render the caps on the front and rear of the shadow volume
void tShadowMeshImpl::DrawCaps()
{
    //nop
}


//------------------------------------------------------------------------------------
//Render the shadow volume 
void tShadowMeshImpl::DrawShadowVolume()
{
    if (s_NumDrawEdges == 0) return;
    
    unsigned short i;
    P3DASSERT(tShadowGeneratorImpl::GetCurVolumeShader());

    pddiPrimStream* stream;
    stream = p3d::pddi->BeginPrims(tShadowGeneratorImpl::GetCurVolumeShader()->GetShader(), PDDI_PRIM_TRIANGLES, PDDI_V_POSITION, 6 * s_NumDrawEdges);

    //Set the shadow volume colour. It would be better to have this as a shader constant,
    //so we wouldn't have to send this data over the bus many times.
    tShadowGeneratorImpl::GetCurVolumeShader()->SetColour(PDDI_SP_DIFFUSE, s_VolumeColour);

    Vector s;
    if (!isPointLight)
    {

        register unsigned tmp = 0x08080808;

        // Stash light volume vector in vf8, colour in vf9
#ifdef PS2MW        
        asm
        {
            lqc2            vf8, 0x10(this)     // Load light vector in local space
            vmulw.xyzw      vf8,  vf8, vf8w     // scale light vector by volume length

            pextlb          tmp, r0, tmp        // unpack to half words
            pextlh          tmp, r0, tmp        // unpack to words

            qmtc2           tmp, vf9            // Move unpacked colour to vf9
        }
#else
        asm __volatile__ ("

            lqc2            vf8, 0x10(%0)       # Load light vector in local space
            vmulw.xyzw      vf8,  vf8, vf8w     # scale light vector by volume length

            pextlb          %1, $0, %1          # unpack to half words
            pextlh          %1, $0, %1          # unpack to words

            qmtc2           %1, vf9             # Move unpacked colour to vf9

        ": // Outputs
         : "r" (this), "r" (tmp)  // Inputs
          ); // Clobber list
#endif

        register DrawEdge *drawEdges = s_DrawEdges;
        register Vector4 *verts = vertices;
        register unsigned indexA = 0, indexB = 0;

        unsigned loopCount;
        unsigned edgesProcessed = 0;
        while (edgesProcessed < s_NumDrawEdges)
        {
            loopCount = s_NumDrawEdges - edgesProcessed;
            unsigned edgesThisBuffer = stream->GetBufferSizeAvailable() / 6;
            if (edgesThisBuffer < loopCount) loopCount = edgesThisBuffer;

            edgesProcessed += loopCount;

            register Vector4 *dest = (Vector4 *)stream->GetNextVertexAddress();

            for (i = 0; i < loopCount; i++)
            {
#ifdef PS2MW        
                asm
                {
                    lh      indexA, 0x00(drawEdges)     // Get index of vertices
                    lh      indexB, 0x02(drawEdges)
                    addiu   drawEdges, drawEdges, 4

                    sll     indexA, indexA, 4         // Change index to offset in array
                    sll     indexB, indexB, 4         // Change index to offset in array
            
                    addu    indexA, indexA, verts     // Make offsets into pointers
                    addu    indexB, indexB, verts

                    lqc2    vf1, 0x00(indexA)         // Load verts
                    lqc2    vf2, 0x00(indexB)

                    vadd    vf3, vf1, vf8               // Extend volume 
                    vadd    vf4, vf2, vf8

                    sqc2    vf9, 0x010(dest)             // Vertex 1
                    sqc2    vf1, 0x020(dest)

                    sqc2    vf9, 0x040(dest)             // Vertex 2
                    sqc2    vf3, 0x050(dest)

                    sqc2    vf9, 0x070(dest)             // Vertex 3
                    sqc2    vf2, 0x080(dest)

                    sqc2    vf9, 0x0A0(dest)             // Vertex 4
                    sqc2    vf2, 0x0B0(dest)

                    sqc2    vf9, 0x0D0(dest)             // Vertex 5
                    sqc2    vf3, 0x0E0(dest)

                    sqc2    vf9, 0x100(dest)             // Vertex 6
                    sqc2    vf4, 0x110(dest)

                    addiu   dest, dest, 0x120
                }
#else
                asm __volatile__ ("
                    .set noreorder
                    lh      %1, 0x00(%0)    # Get index of vertices
                    lh      %2, 0x02(%0)
                    addiu   %0, %0, 4

                    sll     %1, %1, 4       #  Change index to offset in array
                    sll     %2, %2, 4       #  Change index to offset in array
            
                    addu    %1, %1, %4      #  Make offsets into pointers
                    addu    %2, %2, %4

                    lqc2    vf1, 0x00(%1)   #  Load verts
                    lqc2    vf2, 0x00(%2)

                    vadd    vf3, vf1, vf8   # Extend volume 
                    vadd    vf4, vf2, vf8

                    sqc2    vf1, 0x00(%3)
                    sqc2    vf3, 0x10(%3)
                    sqc2    vf2, 0x20(%3)
                    sqc2    vf2, 0x30(%3)
                    sqc2    vf3, 0x40(%3)
                    sqc2    vf4, 0x50(%3)
                    addiu   %3, %3, 0x60
                    .set reorder
                ": "+&r" (drawEdges), "+&r" (indexA), "+&r" (indexB), "+&r" (dest)   // Outputs
                 : "r" (verts)  // Inputs
                 : "memory" ); // Clobber list
#endif
            }

            stream->Advance(loopCount * 6);
        }
    }
    else
    {
        s = light;

        //Draw the shadow
        for (i = 0; i < s_NumDrawEdges; i++)
        {
            Vector4 &va = vertices[s_DrawEdges[i].a];
            Vector4 &vb = vertices[s_DrawEdges[i].b];

            pddiVector a,b,c,d;
            a.Set(va.x, va.y, va.z);
            b.Set(vb.x, vb.y, vb.z);

            Vector4 sa, sb;
            sa.Sub(va, s);          sb.Sub(vb, s);
            sa.Normalize();         sb.Normalize();
            sa.Scale(volumeLength); sb.Scale(volumeLength);
        
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

//------------------------------------------------------------------------------------
void tShadowMeshImpl::DrawNormals(void)
{
    pddiPrimStream* stream = p3d::pddi->BeginPrims(NULL, PDDI_PRIM_LINES, PDDI_V_C, 2 * numTriangles);
    tColour white(255,255,255,255);
    
    for (unsigned i = 0; i < numTriangles; i++)
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


//------------------------------------------------------------------------------------
void tShadowMeshImpl::DrawShell(void)
{
    tShader *s = new tShader();

    pddiPrimStream* stream = p3d::pddi->BeginPrims(s->GetShader(), PDDI_PRIM_TRIANGLES, PDDI_V_C, 3 * numTriangles);
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

    tRefCounted::Release(s);
}


//------------------------------------------------------------------------------------
void tShadowMeshImpl::GuessVolumeLength(void)
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
//=============================================================================
// tShadowSkinImpl
//=============================================================================
tShadowSkinImpl::tShadowSkinImpl() :
    skeleton(NULL),
    sourceVertexOne(NULL),
    sourceVertexThree(NULL),
    numVertexOne(0),
    numVertexThree(0),
    pose(NULL),
    poseUpdateCount(0xFFFFFFFFUL)
{
}

tShadowSkinImpl::~tShadowSkinImpl()
{
    tRefCounted::Release(skeleton);
    tRefCounted::Release(pose);
    delete[] sourceVertexOne;
    delete[] sourceVertexThree;
}

void tShadowSkinImpl::SetPose(tPose* p) 
{ 
    tRefCounted::Assign(pose, p); 
}

void tShadowSkinImpl::Display() 
{ 
    if(!pose) 
    {
        tPose* p = skeleton->NewPose();
        tRefCounted::Assign(pose, p);
    }

    Display(pose);
}

void tShadowSkinImpl::Display(tPose* p)
{
    if(!p) 
    {
        return;
    }

    register unsigned int i;

//    timeF = radTimeGetMicroseconds();
    // transform the pose joints into world space
    p->Evaluate();

    //if (poseUpdateCount != p->UpdateCount())
    if (1)
    {

        //poseUpdateCount = p->UpdateCount();

        
#ifdef PS2MW
        asm { visub   vi1, vi1, vi1 }   // zero out vi1
#else
        asm (" visub   vi1, vi1, vi1 " );   // zero out vi1
#endif

        // The skin vertices are stored in rest-pose relative world space.
        // The below matrix will transform from this space, into bone-local
        // space where the animation is applied, and then back into camera space.
        unsigned numJoints = rmt::Min( skeleton->GetNumJoint(), p->GetNumJoint() );
        P3DASSERT((numJoints < 64) && "Matrix palette too large for accelerated shadow skinning!");
        for(i = 0; i < numJoints; ++i)
        {
            Matrix m;
            u_long128 *src  = (u_long128 *)&m;
            m.MultAligned(skeleton->GetJoint(i)->inverseWorldMatrix, p->GetJoint(i)->worldMatrix);
            //m.Identity();

            // Upload matrix palette
#ifdef PS2MW
            asm
            {
                lqc2    vf1, 0x00(src)
                lqc2    vf2, 0x10(src)
                lqc2    vf3, 0x20(src)
                lqc2    vf4, 0x30(src)

                vsqi    vf1, (vi1++)
                vsqi    vf2, (vi1++)
                vsqi    vf3, (vi1++)
                vsqi    vf4, (vi1++)
             }   
#else
            asm __volatile__ ("
                lqc2    vf1, 0x00(%0)
                lqc2    vf2, 0x10(%0)
                lqc2    vf3, 0x20(%0)
                lqc2    vf4, 0x30(%0)

                vsqi    vf1, (vi1++)
                vsqi    vf2, (vi1++)
                vsqi    vf3, (vi1++)
                vsqi    vf4, (vi1++)
            ":    // Outputs
             : "r" (src)  // Inputs
              ); // Clobber list
#endif

        }

        register rmt::Vector4 *pOut = vertices;

        register VertexThree *pVertex3 = sourceVertexThree;
        register u_long128 tmp = 0;
        for (i = 0; i < numVertexThree; i++)
        {
#ifdef PS2MW        
            asm
            {
                lqc2            vf5, 0x00(pVertex3) // Load position into v5
                lw              tmp, 0x0C(pVertex3) // Load all four indices
                lqc2            vf7, 0x10(pVertex3) // Load all four weights
                pextlb          tmp, r0, tmp        // unpack to half words
                pextlh          tmp, r0, tmp        // unpack to words
                qmtc2           tmp, vf8            // Move indices to vf8

                vmtir           vi1, vf8x
                vmtir           vi2, vf8y
                vmtir           vi3, vf8z

                vmulx.xyzw      vf9,  vf5, vf7x      // scale input vertex by weight
                vmuly.xyzw      vf10, vf5, vf7y      // scale input vertex by weight
                vmulz.xyzw      vf11, vf5, vf7z      // scale input vertex by weight
                
                vlqi            vf1, (vi1++)        // Load matrix into regs v1-v4
                vlqi            vf2, (vi1++)
                vlqi            vf3, (vi1++)
                vlqi            vf4, (vi1++)

                vmulax.xyzw     ACC, vf1, vf9x
                vmadday.xyzw    ACC, vf2, vf9y
                vmaddaz.xyzw    ACC, vf3, vf9z
                vmaddax.xyzw    ACC, vf4, vf7x

                vlqi            vf1, (vi2++)        // Load matrix into regs v1-v4
                vlqi            vf2, (vi2++)
                vlqi            vf3, (vi2++)
                vlqi            vf4, (vi2++)

                vmaddax.xyzw    ACC, vf1, vf10x
                vmadday.xyzw    ACC, vf2, vf10y
                vmaddaz.xyzw    ACC, vf3, vf10z
                vmadday.xyzw    ACC, vf4, vf7y

                vlqi            vf1, (vi3++)        // Load matrix into regs v1-v4
                vlqi            vf2, (vi3++)
                vlqi            vf3, (vi3++)
                vlqi            vf4, (vi3++)

                vmaddax.xyz     ACC, vf1, vf11x
                vmadday.xyzw    ACC, vf2, vf11y
                vmaddaz.xyzw    ACC, vf3, vf11z
                vmaddz.xyzw     vf6, vf4, vf7z      // result into vf6

                sqc2    vf6, 0x00(pOut)     // store result into main memory
            }
#else
            asm __volatile__ ("
                lqc2            vf5, 0x00(%1)       # Load position into v5
                lw              %0,  0x0C(%1)       # Load all four indices
                lqc2            vf7, 0x10(%1)       # Load all four weights
                pextlb          %0, $0, %0          # unpack to half words
                pextlh          %0, $0, %0          # unpack to words
                qmtc2           %0, vf8             # Move indices to vf8

                vmtir           vi1, vf8x
                vmtir           vi2, vf8y
                vmtir           vi3, vf8z

                vmulx.xyzw      vf9,  vf5, vf7x     # scale input vertex by weight
                vmuly.xyzw      vf10, vf5, vf7y     # scale input vertex by weight
                vmulz.xyzw      vf11, vf5, vf7z     # scale input vertex by weight
                
                vlqi            vf1, (vi1++)        # Load matrix into regs v1-v4
                vlqi            vf2, (vi1++)
                vlqi            vf3, (vi1++)
                vlqi            vf4, (vi1++)

                vmulax.xyzw     ACC, vf1, vf9x
                vmadday.xyzw    ACC, vf2, vf9y
                vmaddaz.xyzw    ACC, vf3, vf9z
                vmaddax.xyzw    ACC, vf4, vf7x

                vlqi            vf1, (vi2++)        # Load matrix into regs v1-v4
                vlqi            vf2, (vi2++)
                vlqi            vf3, (vi2++)
                vlqi            vf4, (vi2++)

                vmaddax.xyzw    ACC, vf1, vf10x
                vmadday.xyzw    ACC, vf2, vf10y
                vmaddaz.xyzw    ACC, vf3, vf10z
                vmadday.xyzw    ACC, vf4, vf7y

                vlqi            vf1, (vi3++)        # Load matrix into regs v1-v4
                vlqi            vf2, (vi3++)
                vlqi            vf3, (vi3++)
                vlqi            vf4, (vi3++)

                vmaddax.xyz     ACC, vf1, vf11x
                vmadday.xyzw    ACC, vf2, vf11y
                vmaddaz.xyzw    ACC, vf3, vf11z
                vmaddz.xyzw     vf6, vf4, vf7z      # result into vf6

                sqc2    vf6, 0x00(%2)               # store result into main memory
            ": "+r" (tmp)   // Outputs
             : "r" (pVertex3), "r" (pOut)  // Inputs
             : "memory" ); // Clobber list
#endif
            ++pVertex3;
            ++pOut;
        }

        VertexOne *pVertex1 = sourceVertexOne;
        for(i=0; i<numVertexOne;i++)
        {
#ifdef PS2MW        
            asm 
            {
                lqc2            vf5, 0x00(pVertex1)          // Load position into v5
                vmtir           vi1, vf5w                    // move index into int register

                // Load matrix into regs v1-v4
                vlqi            vf1, (vi1++)
                vlqi            vf2, (vi1++)
                vlqi            vf3, (vi1++)
                vlqi            vf4, (vi1++)

                vmulax.xyzw     ACC, vf1, vf5x
                vmadday.xyzw    ACC, vf2, vf5y
                vmaddaz.xyzw    ACC, vf3, vf5z
                vmaddw.xyzw     vf6, vf4, vf0w // result into vf6

                sqc2    vf6, 0x00(pOut)     // store result into main memory
            }
#else
            asm __volatile__ ("
                lqc2            vf5, 0x00(%0)   # Load position into v5
                vmtir           vi1, vf5w       # move index into int register

                # Load matrix into regs v1-v4
                vlqi            vf1, (vi1++)
                vlqi            vf2, (vi1++)
                vlqi            vf3, (vi1++)
                vlqi            vf4, (vi1++)

                vmulax.xyzw     ACC, vf1, vf5x
                vmadday.xyzw    ACC, vf2, vf5y
                vmaddaz.xyzw    ACC, vf3, vf5z
                vmaddw.xyzw     vf6, vf4, vf0w  # result into vf6

                sqc2    vf6, 0x00(%1)         # store result into main memory

            ":  // Outputs  
             : "r" (pVertex1), "r" (pOut)  // Inputs
             : "memory" ); // Clobber list
#endif
            ++pVertex1;
            ++pOut;
        }

        //Update per triangle normal
        pOut = normals;
        for (i = 0; i < numTriangles; i++)
        {
            register Vector4 *a = &vertices[topology[i].v0];
            register Vector4 *b = &vertices[topology[i].v1];
            register Vector4 *c = &vertices[topology[i].v2];
                
#ifdef PS2MW        
            asm
            {
                lqc2        vf1, 0x00(a)
                lqc2        vf2, 0x00(b)
                lqc2        vf3, 0x00(c)
                
                vsub        vf4, vf2, vf1
                vsub        vf5, vf3, vf2

                vopmula.xyz ACC, vf4, vf5
                vopmsub.xyz vf1, vf5, vf4

                sqc2        vf1, 0x00(pOut)
            }
#else
            asm __volatile__ ("
                lqc2        vf1, 0x00(%0)
                lqc2        vf2, 0x00(%1)
                lqc2        vf3, 0x00(%2)
                
                vsub        vf4, vf2, vf1
                vsub        vf5, vf3, vf2

                vopmula.xyz ACC, vf4, vf5
                vopmsub.xyz vf1, vf5, vf4

                sqc2        vf1, 0x00(%3)

            ":  // Outputs
             : "r" (a), "r" (b), "r" (c), "r" (pOut)  // Inputs
             : "memory" ); // Clobber list
#endif
            ++pOut;
        }
    }

    GuessVolumeLength( p );
    tShadowMeshImpl::Display();
}

void tShadowSkinImpl::GuessVolumeLength( tPose* Pose )
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



//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
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
    version      = f->GetLong();
    numVertices  = f->GetLong();
    numTriangles = f->GetLong();

    //Some basic sanity checking
    P3DASSERT(version == SHADOWMESH_VERSION);

    //Create the tShadowSkinImpl
    tShadowMesh *s = Allocate();
    s->SetName(name);

    tShadowMeshImpl* shadow = &s->impl;
    
    shadow->numTriangles = numTriangles;
    shadow->numVertices = numVertices;

    //Preallocate the static shadow data
    unsigned numTrisAligned = numTriangles;
    if ((numTrisAligned % 3) != 0) numTrisAligned += (3 - (numTrisAligned % 3));
    shadow->topology = new tShadowMeshImpl::Topology[numTrisAligned];

    //Allocate space for the dynamic data
    shadow->vertices = new Vector4[numVertices];
    shadow->normals = new Vector4[numTrisAligned];
    shadow->triCulled = new unsigned char[numTrisAligned];

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
    Vector4 vAB, vBC;
    for(i=0; i<shadow->numTriangles;i++)
    {
        Vector4 &a = shadow->vertices[shadow->topology[i].v0];
        Vector4 &b = shadow->vertices[shadow->topology[i].v1];
        Vector4 &c = shadow->vertices[shadow->topology[i].v2];

        vAB.Sub(b, a);    
        vBC.Sub(c, b);
        shadow->normals[i].CrossProduct(vAB, vBC);
    }

    return s;
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

    //Create the tShadowSkinImpl
    tShadowSkin *s = Allocate();
    s->SetName(name);

    tShadowSkinImpl* shadow = &s->impl;

    shadow->numTriangles = numTriangles;
    shadow->numVertices = numVertices;

    // Temp buffer for holding vertex information.
    tShadowSkinImpl::VertexThree* sourceVertexThree = (tShadowSkinImpl::VertexThree*)p3d::MallocTemp(sizeof(tShadowSkinImpl::VertexThree) * numVertices );
    P3DASSERT( sourceVertexThree );

    //Preallocate the static shadow data
    shadow->topology = new tShadowSkinImpl::Topology[numTriangles];

    //Allocate space for the dynamic data
    shadow->vertices  = new Vector4[numVertices];
    shadow->normals   = new Vector4[numTriangles];
    shadow->triCulled = new unsigned char [numTriangles];

    unsigned short *vertexRemapping = (unsigned short *)p3d::MallocTemp(sizeof(unsigned short) * numVertices);

    //Setup the skeleton
    shadow->skeleton = p3d::find<tSkeleton>(store, skelName);
    if(shadow->skeleton)
    {
        shadow->skeleton->AddRef();
        shadow->pose = shadow->skeleton->NewPose();
        shadow->pose ->AddRef();
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
                    f->GetData(&sourceVertexThree[i].position, 3, tFile::DWORD);
                }
                break;
            }
            case Pure3D::Mesh::WEIGHTLIST:
            {
                unsigned int count = (unsigned int) f->GetLong();
                P3DASSERT(count == numVertices);
                for(i=0; i<count; i++)
                {
                   f->GetData(&sourceVertexThree[i].weights, 3, tFile::DWORD);
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
                    sourceVertexThree[i].matrixIndex[0] = tempIndex[3];
                    sourceVertexThree[i].matrixIndex[1] = tempIndex[2];
                    sourceVertexThree[i].matrixIndex[2] = tempIndex[1];
                    sourceVertexThree[i].matrixIndex[3] = tempIndex[0];
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


    // Count quantity of each vertex type
    for( i = 0; i < numVertices; ++i )
    {
        tShadowSkinImpl::VertexThree* pSource = &( sourceVertexThree[ i ] );
        if ( 0 == pSource->weights[ 2 ] && 0 == pSource->weights[ 1 ] )
        {
            // One bone.
            ++(shadow->numVertexOne);
        }
        else
        {
            ++(shadow->numVertexThree);
        }
    }

    shadow->sourceVertexOne   = new tShadowSkinImpl::VertexOne[ shadow->numVertexOne ];
    shadow->sourceVertexThree = new tShadowSkinImpl::VertexThree[ shadow->numVertexThree ];

    int vertexOneIndex = 0;
    int vertexThreeIndex = 0;

    //Sort the vertices by number of weights
    for (i = 0; i < numVertices; i++)
    {
        //Source vertex
        tShadowSkinImpl::VertexThree* source = &(sourceVertexThree[i]);

        //Bucket according to number of bones.
        if (0==source->weights[2] && 0==source->weights[1])
        {
            // one bone
            tShadowSkinImpl::VertexOne* pDest = &(shadow->sourceVertexOne[ vertexOneIndex ]);
            pDest->position = source->position;
            pDest->matrixIndex = source->matrixIndex[0] * 4; // pointer in VU0 mem space
            vertexRemapping[i] = vertexOneIndex + shadow->numVertexThree;
            ++vertexOneIndex;
        }
        else
        {
            // three bones
            tShadowSkinImpl::VertexThree* pDest = &( shadow->sourceVertexThree[ vertexThreeIndex ] );
            pDest->position = source->position;
            pDest->matrixIndex[0] = source->matrixIndex[ 0 ] * 4; // pointer in VU0 mem space
            pDest->matrixIndex[1] = source->matrixIndex[ 1 ] * 4;
            pDest->matrixIndex[2] = source->matrixIndex[ 2 ] * 4;
            pDest->matrixIndex[3] = (0==source->weights[2]) ? 2 : 3; // Bone count
            pDest->weights[0] = source->weights[ 0 ];
            pDest->weights[1] = source->weights[ 1 ];
            pDest->weights[2] = source->weights[ 2 ];
            pDest->weights[3] = 0.0F;
            vertexRemapping[i] = vertexThreeIndex;
            ++vertexThreeIndex;
        }
    }

    P3DASSERT( (unsigned)vertexOneIndex == shadow->numVertexOne );
    P3DASSERT( (unsigned)vertexThreeIndex == shadow->numVertexThree );

    // Remap topology
    for (i = 0; i < shadow->numTriangles; i++)
    {
        shadow->topology[i].v0 = vertexRemapping[shadow->topology[i].v0];
        shadow->topology[i].v1 = vertexRemapping[shadow->topology[i].v1];
        shadow->topology[i].v2 = vertexRemapping[shadow->topology[i].v2];
    }
    
    p3d::FreeTemp(vertexRemapping);
    p3d::FreeTemp(sourceVertexThree);

    return s;
}

tShadowSkin* tShadowSkinLoader::Allocate()
{
    return new tShadowSkin();
}
*/