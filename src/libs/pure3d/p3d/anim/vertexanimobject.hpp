//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _VERTEXANIMOBJECT_HPP_
#define _VERTEXANIMOBJECT_HPP_

#include <p3d/drawable.hpp>

class tGeometry;
class tVertexAnimKey;
class tVertexList;
class tColourOffset;

//a class for keeping offset of a vertex list, non-indexable
class tVertexOffsetList : public tRefCounted
{
public:
    enum tVertexOffsetMask {
        eOffsetNone     = 0x00,
        eOffsetUV0      = 0x01,
        eOffsetUV1      = 0x02,
        eOffsetUV2      = 0x04,
        eOffsetUV3      = 0x08,
        eOffsetCoord    = 0x10,
        eOffsetNormal   = 0x20,
        eOffsetColour   = 0x40
    };

    tVertexOffsetList( );
    tVertexOffsetList( tVertexList *vl );

    void SetNormal( int i, rmt::Vector* n );
    void SetPosition( int i, rmt::Vector* p );
    void SetUV( int i, rmt::Vector2 *u, int iu );
    void SetColour( int i, tColourOffset * );
    
    void AddNormalDiff( int i, rmt::Vector* n, float delta );
    void AddPositionDiff( int i, rmt::Vector* p, float delta );
    void AddUVDiff( int i, rmt::Vector2 *u, int iu, float delta );
    void AddColourDiff( int i, tColourOffset *, float delta );

    void Reset( );

    int  mVertexNum;
    rmt::Vector*   mCoord;
    rmt::Vector*   mNormal;
    rmt::Vector2*  mUV[4];
    tColourOffset*       mColour;
    long    mMask;

protected:
    ~tVertexOffsetList( );    
};

//a class for keeping the offset list of all primgroups of a geometry
class tGeometryOffset : public tRefCounted
{
public:
    tGeometryOffset( );
    tGeometryOffset( tGeometry *geo );

    int mOffsetsNum;
    tVertexOffsetList **mOffsets;

protected:
    ~tGeometryOffset(  );
};

class tVertexAnimObject:  public tDrawable
{
public:
    enum eInterpolateMask{
        eInterpolateNone = 0x00,
        eInterpolateUV0 = 0x01,
        eInterpolateUV1 = 0x02,
        eInterpolateUV2 = 0x04,
        eInterpolateUV3 = 0x08,
        eInterpolateCoord = 0x10,
        eInterpolateNormal = 0x20,
        eInterpolateColour = 0x40
    };

    tVertexAnimObject( );
    void Reset( );
    void SetActiveObject( tGeometry *p );
    void SetInterpolateMask( int m ){ mMask = m; }
    int  GetInterpolateMask( ){ return mMask; }
    tGeometry *GetActiveObject( ){ return mActiveGeo; };
    void AddDifference( tVertexAnimKey *key, int idxPrim, float delta );
    void Update( tVertexAnimKey *key, int idxPrim );
    virtual void Display( );
    virtual void ProcessShaders(ShaderCallback&);

    void EnableAutoOffsetClear(bool enable) { mAutoClear = enable; }
    void ClearOffsets();

protected:
    virtual ~tVertexAnimObject( );

    bool mAutoClear;
    tGeometry * mActiveGeo;          //the active geometry to be shown
    tGeometryOffset *mGeoOffset;
    int  mMask;          //interpolation mask for different component
};



#endif
