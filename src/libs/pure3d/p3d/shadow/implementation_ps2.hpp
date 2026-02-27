//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _SHADOW_IMPLIMENTATION_PS2_HPP_
#define _SHADOW_IMPLIMENTATION_PS2_HPP_

#include <p3d/p3dtypes.hpp>

class tShader;
class tPose;
class tSkeleton;
class gcExtBufferCopy;
class pddiTexture;


class tShadowGeneratorImpl
{
public:
    tShadowGeneratorImpl();
    ~tShadowGeneratorImpl();
    
    void PreRender(void);

    void SetWashColour(tColour c);

    void Begin(void);
    void End(void);

    static tShader* GetCurVolumeShader(void) { return s_VolumeShader; }

    enum ShadowVolPass { PASS_FRONT, PASS_BACK };
    static void SetupPass(ShadowVolPass pass);

protected:

    // colour for "wash" polygon
    tColour washColour;    

    static tShader* s_VolumeShader;

    // saved previous cull mode
    pddiCullMode oldCull;
};



//---------------------------------------------------------------
class tShadowMeshImpl
{
public:
    tShadowMeshImpl();
    ~tShadowMeshImpl();

    void Display();

    void GetBoundingBox(rmt::Box3D* b)          { *b = boundingBox; }
    void GetBoundingSphere(rmt::Sphere* s)      { *s = boundingSphere; }

    void SetLight(bool isPoint, const rmt::Vector& posOrDir) { isPointLight = isPoint; worldLight = posOrDir;}
    void SetVolumeLength(float len) { setVolumeLength = len; }

    void DrawNormals(void);
    void DrawShell(void);

protected:
    friend class tShadowMeshLoader;

    void UpdateEdges(void);
    void DrawShadowVolume(void);
    void DrawCaps(void);
    void GuessVolumeLength(void);

    unsigned numVertices;
    unsigned numTriangles;

    struct Topology
    {
        unsigned short v0,v1,v2;    //Vertex index
        unsigned short n0,n1,n2;    //neighbour index (tri) -1 for no neighbour
    };

    //Static data
    Topology* topology;    //numTri EdgeData's.  Computed in the pipeline

    bool          isPointLight;
    rmt::Vector   light;    
    float         volumeLength;
    rmt::Vector   worldLight;
    float         setVolumeLength;

    rmt::Vector4  *vertices;  
    rmt::Vector4  *normals;    
    unsigned char *triCulled;        // Triangle culled flags. 
    rmt::Sphere    boundingSphere;
    rmt::Box3D     boundingBox;


} __attribute__((aligned(16)));

//---------------------------------------------------------------
class tShadowSkinImpl : public tShadowMeshImpl
{
public:
    tShadowSkinImpl();
    ~tShadowSkinImpl();

    void Display(void);
    void Display(tPose *p);

    void SetPose(tPose *p);
    tPose* GetPose(void) { return pose; }

    tSkeleton* GetSkeleton(void) { return skeleton; }
    
protected:
    friend class tShadowSkinLoader;
    void DisplayNonIndexed();
    // This is really guess volume length for
    //skinned objects, but it's not virtual
    //and I don't want to hide parent's method
    //just to avoid unexpected behavour.
    void GuessVolumeLength(tPose *Pose);

    tSkeleton *skeleton;

    struct VertexOne
    {
        rmt::Vector position;
        unsigned    matrixIndex;   // index into VU0 memory
    };

    struct VertexThree
    {
        rmt::Vector   position;
        unsigned char matrixIndex[4]; // We're really only supporting 3, but it would just get padded anyway.
        float         weights[4];
    };

    //Static data
    VertexOne   *sourceVertexOne;
    VertexThree *sourceVertexThree;
    unsigned numVertexOne;
    unsigned numVertexThree;

    tPose* pose;
    unsigned poseUpdateCount;
} __attribute__((aligned(16)));

#endif