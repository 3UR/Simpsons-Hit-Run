//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _VERTEXLIST_HPP_
#define _VERTEXLIST_HPP_

#include <p3d/entity.hpp>
#include <p3d/anim/vertexanimobject.hpp>
#include <radmath/radmath.hpp>

class pddiPrimStream;

class tVertexList : public tRefCounted
{
public:
    struct MatrixIndices
    {
        unsigned char index[4];
    };

    struct MatrixWeights
    {
        float weight[3];
    };

    tVertexList(unsigned num, unsigned vertexFormat);
    tVertexList(tVertexList&);

    unsigned GetFormat() const             { return format; }
    int GetNumVertex() const               { return nVertex; }
    int GetNumVertices() const             { return nVertex; }   // for compatibility with v12/13

    rmt::Vector*   GetPositions()          { return coord; }
    rmt::Vector*   GetNormals()            { return normal; }
    rmt::Vector*   GetBinormals()          { return binormal; }
    rmt::Vector*   GetTangents()           { return tangent; }
    rmt::Vector2*  GetUVs(int channel)     { return uv[channel]; }
    tColour*       GetColours(int channel = 0) { return colour[channel]; }
    MatrixIndices* GetMatrixIndices()      { return matIndices; }
    MatrixWeights* GetMatrixWeights()      { return matWeights; }
    tVertexOffsetList* GetOffsetList( )    { return offset; }
    bool           HasOffsets( )           { return offset != NULL; };

    //set the offset list
    void SetOffsetList( tVertexOffsetList *l ){ tRefCounted::Assign( offset, l ); };

    void Stream(pddiPrimStream* stream, int start, int count);

    enum { MAX_NUM_UV_SETS = 4, MAX_NUM_COLOUR_SETS = 7 };

protected:
    virtual ~tVertexList();  // protected to get rid of GCC warning

private:
    void Allocate(unsigned nVertex, unsigned format);

    unsigned format;
    unsigned nVertex;
    rmt::Vector*   coord;
    rmt::Vector*   normal;
    rmt::Vector*   binormal;
    rmt::Vector*   tangent;
    rmt::Vector2*  uv[MAX_NUM_UV_SETS];
    tColour*       colour[MAX_NUM_COLOUR_SETS];
    MatrixIndices* matIndices;
    MatrixWeights* matWeights;

    tVertexOffsetList *offset;

};

#endif

