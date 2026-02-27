// FePolygon.h
// Created by wng on May 25, 2000 @ 2:55 PM.

#ifndef __FePolygon__
#define __FePolygon__

#include "FeDrawable.h"
#include "Polygon.h"

#if defined( RAD_WIN32 ) || defined( RAD_UWP )
#pragma warning( disable : 4250 ) 
#endif

#define MAX_POLYVERTEX 4
class tShader;

class FePolygon 
: 
    public FeDrawable,
    virtual public Scrooby::Polygon
{
public:
    FePolygon( const tName& name );
    void Display();

    //allows access to the bounding box size
    void GetBoundingBox( int& xMin, int& yMin, int& xMax, int& yMax ) const;

    //get the size of the bounding box
    virtual void GetBoundingBoxSize( int& width, int& height ) const;

    int GetNumOfVertexes();
    virtual float GetVertexAlpha( const int which ) const;
    virtual tColour GetVertexColour( const int which ) const;
    virtual void GetVertexLocation( const int which, int& left, int& bottom ) const;
    virtual void Reset();
    void SetNumVertex( int n );
    void SetVertexLocation( int which, int left, int bottom );
    void SetVertexColour( int which, int red, int green, int blue );
    void SetVertexColour( const unsigned int which, const tColour c );
    void SetVertexAlpha( int which, float alpha );
    void SetAlpha( float a );
    void SetColour( tColour c );

protected:
    virtual ~FePolygon();
    void RecalculateDisplayColour( int which = -1 );

    pddiVector mVertex[MAX_POLYVERTEX];
    tColour mVertexColour[MAX_POLYVERTEX];           //this is the raw vertex color
    tColour mDisplayVertexColour[ MAX_POLYVERTEX ];    //this is modulated by the overall alpha
    int mNumVertex;
    tShader* mShader;
};

#endif
