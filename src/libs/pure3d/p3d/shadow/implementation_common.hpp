//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _SHADOW_IMPLIMENTATION_COMMOM_HPP_
#define _SHADOW_IMPLIMENTATION_COMMON_HPP_

#include <p3d/p3dtypes.hpp>

class tShader;
class tPose;
class tSkeleton;
class gcExtBufferCopy;
class pddiTexture;
class tPrimGroupOptimised;

#define MAX_SHADOW_BUFFER_NUM  4

//---------------------------------------------------------------
class tShadowGeneratorImplCommon
{
public:
    static tShader* GetCurVolumeShader(void) { return s_volumeShader; }
    
protected:
    tShadowGeneratorImplCommon();
    ~tShadowGeneratorImplCommon();

    
    // colour and shader for "wash" polygon
    tShader* washShader;
    tColour washColour;    

    // shader for shadow volumes
    tShader* volumeShader;

    static tShader* s_volumeShader;

    // saved previous cull mode
    pddiCullMode oldCull;
};

class tShadowGeneratorImpl : public tShadowGeneratorImplCommon
{
public:
    tShadowGeneratorImpl();
    ~tShadowGeneratorImpl();
    
    void PreRender(void);

    void SetWashColour(tColour c);

    void Begin(void);
    void End(void);

protected:
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
    
    static void CreateShadowBuffer(  );
    static void ReleaseShadowBuffer( );


protected:
    friend class tShadowMeshLoader;

    void UpdateEdges(void);
    void DrawShadowVolume(void);
    void DrawCaps(void);
    void GuessVolumeLength(void);

    unsigned long numVertices;
    unsigned long numTriangles;

    static tPrimGroupOptimised* staticShadowBuffer;  //for buffer-rendering shadow volume
    tPrimGroupOptimised* shadowBuffer;               //for buffer-rendering shadow volume

    struct Topology
    {
        unsigned short v0,v1,v2;    //Vertex index
        unsigned short n0,n1,n2;    //neighbour index (tri) -1 for no neighbour
    };

    //Static data
    Topology* topology;    //numTri EdgeData's.  Computed in the pipeline

    rmt::Vector* vertices;  
    rmt::Vector* normals;    
    bool*        triCulled;        // Triangle culled flags. 
    rmt::Sphere  boundingSphere;
    rmt::Box3D   boundingBox;
    float        volumeLength;
    float        setVolumeLength;

    bool        isPointLight;
    rmt::Vector light;
    rmt::Vector worldLight;    
};

//---------------------------------------------------------------
class tShadowSkinImpl : public tShadowMeshImpl
{
public:
    tShadowSkinImpl();
    ~tShadowSkinImpl();

    void Display();
    void Display(tPose* p);

    void SetPose(tPose* p);
    tPose* GetPose(void) { return pose; }

    tSkeleton* GetSkeleton() { return skeleton; }
    
protected:
    friend class tShadowSkinLoader;
    void DisplayNonIndexed();
    // This is really guess volume length for
    //skinned objects, but it's not virtual
    //and I don't want to hide parent's method
    //just to avoid unexpected behavour.
    void GuessVolumeLength( tPose* Pose );

    tSkeleton* skeleton;
    rmt::Matrix* boneMatrix;      // restpose -> joint -> world space matrices

    struct VertexOne
    {
        rmt::Vector position;
        short vertexIndex;
        unsigned char matrixIndex[4];    //We're really only supporting 3, but it would just get padded anyway.
    };
    struct VertexTwo
    {
        rmt::Vector position;
        short vertexIndex;
        float weights[2];
        unsigned char matrixIndex[4];    //We're really only supporting 3, but it would just get padded anyway.
    };
    struct VertexThree
    {
        rmt::Vector position;
        short vertexIndex;
        float weights[3];
        unsigned char matrixIndex[4];    //We're really only supporting 3, but it would just get padded anyway.
    };

    //Static data
    VertexOne* sourceVertexOne;
    VertexTwo* sourceVertexTwo;
    VertexThree* sourceVertexThree;
    unsigned long numVertexOne;
    unsigned long numVertexTwo;
    unsigned long numVertexThree;

    tPose* pose;
};

#endif