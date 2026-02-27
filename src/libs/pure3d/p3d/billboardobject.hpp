/*===========================================================================
    billboardQuad.hpp
    18-Sep-00 Created by Bryan

    Copyright (c)2000 Radical Entertainment, Inc.
    All rights reserved.
===========================================================================*/

#ifndef _BILLBOARD_QUAD_HPP
#define _BILLBOARD_QUAD_HPP

#include <radmath/radmath.hpp>
#include <p3d/loadmanager.hpp>
#include <p3d/drawable.hpp>
#include <p3d/shader.hpp>
#include <constants/psenum.hpp>
#include <p3d/array.hpp>

#include <render/Culling/SwapArray.h>

//*****************************************************************************
//
// Class tBillboardQuad
//
//*****************************************************************************
class BakedBillboardQuad;

class tBillboardQuad : public tEntity
{
public:
    tBillboardQuad();

    void SetVisibility( bool v )                        { visible = v; }
    bool GetVisibility()                                { return visible; }

    void SetPerspectiveScale(bool scale)                { perspective = scale; }
    bool GetPerspectiveScale() const                    { return perspective; }
    
    void SetBillboardMode(unsigned int mode)            { billboardMode = mode; }
    unsigned int GetBillboardMode() const               { return billboardMode; }

    void SetTranslation(const rmt::Vector& v)           { transform.FillTranslate(v); }
    rmt::Vector GetTranslation() const                  { return transform.Row(3); }

    void SetRotation(const rmt::Quaternion& q)          { transform.FillRotation(q); }
    rmt::Quaternion GetRotation() const                 { rmt::Quaternion q; q.BuildFromMatrix(transform); return q; }

    void SetTransform(const rmt::Matrix& m)             { transform = m; }
    rmt::Matrix GetTransform() const                    { return transform; }    

    void SetWidth(float w)                              { width = (w * 0.5f); }
    float GetWidth() const                              { return (width * 2.0f); }

    void SetHeight(float h)                             { height = (h * 0.5f); }
    float GetHeight() const                             { return (height * 2.0f); }
    
    void SetDistance(float d)                           { distance = d; }
    float GetDistance() const                           { return distance; }

    void SetColour(tColour c)                           { colour = c; }
    tColour GetColour() const                           { return colour; }

    void SetUV(int index, const rmt::Vector2& p)        { P3DASSERT((index>=0)&&(index<=3)); uv[index] = p; }
    rmt::Vector2 GetUV(int index) const                 { P3DASSERT((index>=0)&&(index<=3)); return uv[index]; }

    void SetUVOffset(const rmt::Vector2& offset)        { uvOffset = offset; }
    rmt::Vector2 GetUVOffset() const                    { return uvOffset; }

    void SetCutOffMode(unsigned int mode)               { cutOffMode = mode; }
    unsigned int GetCutOffMode() const                  { return cutOffMode; }

    void SetUVOffsetRange(const rmt::Vector2& range)    { uvOffsetRange.u = rmt::Abs(range.u); uvOffsetRange.v = rmt::Abs(range.v);}
    rmt::Vector2 GetUVOffsetRange() const               { return uvOffsetRange; }

    void SetSourceRange(float range)                    { sourceRange = rmt::Abs(range); }
    float GetSourceRange() const                        { return sourceRange; }

    void SetEdgeRange(float range)                      { edgeRange = rmt::Abs(range); }
    float GetEdgeRange() const                          { return edgeRange; }

    virtual void GetBoundingBox(rmt::Box3D* b)          { *b = boundingBox; }
    virtual void GetBoundingSphere(rmt::Sphere* s)      { *s = boundingSphere; }

    friend class tBillboardQuadGroup;
    friend class tBillboardQuadGroupLoader;

    //Hack
    rmt::Vector v[4]; 
    rmt::Vector2 t[4];
    tColour displayColour;
    bool                flip;
    unsigned int        billboardMode;
protected:
    virtual ~tBillboardQuad();

    virtual void Display(pddiPrimStream* stream, const rmt::Matrix& world, const rmt::Matrix& camera, const rmt::Matrix& worldToCamera, float intensityBias);
    int  DisplayOcclusionSprite(const rmt::Matrix& world, const rmt::Matrix& camera, const rmt::Matrix& worldToCamera);
    void Calculate();

    rmt::Box3D          boundingBox;
    rmt::Sphere         boundingSphere;

    bool                visible;
    bool                perspective;
    float               intensity;
    rmt::Matrix         transform;
    float               width;
    float               height;
    float               distance;
    tColour             colour;
    rmt::Vector2        uv[5];          // 0 = bottom left, 3 = top left, 5 = box surrounding all uv's
    rmt::Vector2        uvOffset;       // uv offset set by art
    unsigned int        cutOffMode;
    rmt::Vector2        uvOffsetRange;
    float               sourceRange;
    float               edgeRange;
};

// A special billboard class that gets automatically scaled by the distance 
// to the camera. Used to the arrows overtop of the vehicles
// Which need to grow larger the farther away the car is
class tCamRelativeBillboardQuad : public tBillboardQuad
{
public:
    tCamRelativeBillboardQuad();
    // Set the behaviour parameters. The billboard size is scaled
    // linearly between minSize and maxSize when the distance to the billboard from the camera
    // between minDist and maxDist
    void Set( float minSize, float maxSize, float minDist, float maxDist );

protected:
    virtual void Display(pddiPrimStream* stream, const rmt::Matrix& world, const rmt::Matrix& camera, const rmt::Matrix& worldToCamera, float intensityBias);
    // Determine distance from camera to billboard and determine the appropriate scaling factor
    float CalcScale( const rmt::Matrix& camera, const rmt::Matrix& world )const;

    float m; // Slope of the scaling equation
    // m = ( maxSize - 1.0f ) / ( far - near )
    float maxSize;  // scale factor maximum
    float minSize;  // scale factor minimum
    float nearDist; // distance where the scaling is at minSize

    virtual ~tCamRelativeBillboardQuad();
};

class BakedBillboardQuad
{
public:
    BakedBillboardQuad(){}
    ~BakedBillboardQuad(){}

    void Display(pddiPrimStream* stream);
    void CopyQuad( tBillboardQuad* ipQuad )
    {
        for(int i=0; i<4; i++)
        {
            v[i] = ipQuad->v[i];
            t[i] = ipQuad->t[i];
        }
        displayColour = ipQuad->displayColour;
        flip = ipQuad->flip;
        billboardMode = ipQuad->billboardMode;
    }

    rmt::Vector v[4]; 
    rmt::Vector2 t[4];
    tColour displayColour;
    bool flip;
    unsigned int billboardMode;
};

//*****************************************************************************
//
// Class tBillboardQuadGroup
//
//*****************************************************************************
class tBillboardQuadGroup: public tDrawable
{
public:
    tBillboardQuadGroup();

    virtual void Display();

    void SetShader(tShader* mat);
    tShader* GetShader() const                          { return shader; }
   
    void SetZTest(bool enable)                          { zTest = enable; }
    bool GetZTest() const                               { return zTest; }

    void SetZWrite(bool enable)                         { zWrite = enable; }
    bool GetZWrite() const                              { return zWrite; }

    void SetOcclusion(int mode)                         { occlusionMode = mode; }
    int  GetOcclusion() const                           { return occlusionMode; }    
    
    int GetNumQuads()                                   { return numQuads; }
    tBillboardQuad* GetQuad(int index)                  { return ((index>=0)&&(index<numQuads)) ? quads[index] : NULL; }
    tBillboardQuad* FindQuadByUID(tUID uid);
    tBillboardQuad* FindQuadByName(const char* name);

    virtual void GetBoundingBox(rmt::Box3D* b)          { *b = boundingBox; }
    virtual void GetBoundingSphere(rmt::Sphere* s)      { *s = boundingSphere; }

    virtual void ProcessShaders(ShaderCallback&);

    void SetIntensityBias( float bias );

    // Replaces all linked tBillboardQuads with tCameraRelativeBillboardQuads of the given specifications
    void ConvertToCameraRelative( float minSize, float maxSize, float minDist, float maxDist );

#ifdef RAD_XBOX
    struct VisibilityTests
    {
        VisibilityTests() :
            numTests(0),
            tests()
        { 
        }
        unsigned int numTests;
        tPtrDynamicArray<tBillboardQuadGroup*> tests;
    };

    static void SetMaxNumVisibilityTests( unsigned int numTests );
    static unsigned int GetMaxNumVisibilityTests( void );
    static void SubmitVisibilityTests( void );
    static void GetVisibilityResults( void );
#endif

    friend class tBillboardQuadGroupLoader;

    void AddQuads(tBillboardQuadGroup* ipToBeCopied)
    {
        for(int i=ipToBeCopied->numQuads-1; i>-1; i--)
        {
            tBillboardQuad* quad = ipToBeCopied->quads[i];

            if( (quad->GetVisibility() == true) && 
                (quad->GetColour().Alpha() != 0) && 
                !((quad->GetColour().Red() == 0) && 
                  (quad->GetColour().Green() == 0) &&
                  (quad->GetColour().Blue()  == 0)) )
            {
                mTempQuads.AddUse(1);
                mTempQuads[mTempQuads.mUseSize-1].CopyQuad(quad);
            }
        }
    }

    void ShallowCopy(tBillboardQuadGroup* ipToBeCopied)
    {
        //Shallow copy hack (see bottom of file
        numQuads        = ipToBeCopied->numQuads;
        quads           = ipToBeCopied->quads;
        boundingBox     = ipToBeCopied->boundingBox;
        //boundingBox.low.Scale(100.0f);
        //boundingBox.high.Scale(100.0f);
        boundingSphere  = ipToBeCopied->boundingSphere;
        //boundingSphere.radius = boundingSphere.radius*100.0f;
        shader          = ipToBeCopied->shader;
        zTest           = ipToBeCopied->zTest;//true;//ipToBeCopied->zTest;
        zWrite          = ipToBeCopied->zWrite;//false;//ipToBeCopied->zWrite;

        if(mTempQuads.IsSetUp())    mTempQuads.ClearUse();
        else                        mTempQuads.Allocate(200+numQuads);

        AddQuads(ipToBeCopied);
        //for(int i=numQuads-1; i>-1; i--)
        //{
        //    mTempQuads.Add(quads[i]);
        //}
    }

    tShader* shader;
    int numQuads;
    tPtrArray<tBillboardQuad*> quads;
    SwapArray<BakedBillboardQuad> mTempQuads;
protected:
    virtual ~tBillboardQuadGroup();

    rmt::Box3D  boundingBox;
    rmt::Sphere boundingSphere;

    bool     zTest;
    bool     zWrite;
    int      occlusionMode;
    float    intensityBias;

#ifdef RAD_XBOX
    int maxPixelsVisible;
    int numPixelsVisible;
    rmt::Vector visibilityPosition;
    
    static unsigned int maxVisibilityTests;
    static VisibilityTests oldVisibilityTests;
    static VisibilityTests currVisibilityTests;
#endif
};

//*****************************************************************************
//
// Class tBillboardQuadGroupLoader - loads billboard quad
//
//*****************************************************************************
class tBillboardQuadGroupLoader: public tSimpleChunkHandler
{
public:
    tBillboardQuadGroupLoader();
protected:
    ~tBillboardQuadGroupLoader() {};
    tEntity* LoadObject(tChunkFile*, tEntityStore* store);
    tBillboardQuad* LoadQuad(tChunkFile*, tEntityStore* store);
};

//////////////////////////////////////////////////////////////////////////
//<HACK>
// Since this is a hack, it does shallow copies responsably, and does not
// AddRef/Release.
//////////////////////////////////////////////////////////////////////////
#include <p3d/displaylist.hpp>
#include <render/Culling/SwapArray.h>

class BakedBillboardQuad;
class BillboardQuadManager
{
protected:
    BillboardQuadManager();

    ~BillboardQuadManager()
    {
        for(int i=0; i<mpBBQGs.mSize-1;i++)
        {
            mpBBQGs[i]->ReleaseVerified();
        }
    }

public:
    // Static Methods 
    static BillboardQuadManager* CreateInstance()
    {
        rTuneAssert(spInstance == NULL);
        spInstance = new BillboardQuadManager();
        return spInstance;
    }

    static BillboardQuadManager* GetInstance()
    {
        return spInstance;
    }

    static void DestroyInstance()
    {
        rTuneAssert(spInstance != NULL);
        delete spInstance;
    }

    void Clear()
    {
        mpBBQGs.ClearUse();
    }

    void DisplayAll();

    void Add(tBillboardQuadGroup* ipBBQG);

    static void Enable()
    {
        sEnabled = true;
    }
    static void Disable()
    {
        sEnabled = false;
    }

    static bool sEnabled;
    static bool sCollectPass;
protected:

    SwapArray<tBillboardQuadGroup*> mpBBQGs;

    static BillboardQuadManager* spInstance;
};

inline BillboardQuadManager* GetBillboardQuadManager() { return( BillboardQuadManager::GetInstance() ); }
//////////////////////////////////////////////////////////////////////////
//</HACK>
//////////////////////////////////////////////////////////////////////////

#endif /* _OPTIC_HPP */
