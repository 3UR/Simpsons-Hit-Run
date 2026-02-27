// FeGroup.h
// Created by wng on Thu, May 11, 2000 @ 6:51 PM.

#ifndef __FeGroup__
#define __FeGroup__

#include "Group.h"
#include "FeOwner.h"

#if defined(RAD_WIN32) || defined(RAD_UWP)
#pragma warning( disable : 4250 ) 
#endif

class FeGroup 
: 
    public FeOwner,
    virtual public Scrooby::Group
{

public:
    FeGroup( const tName& name );

    //get the extents of the bounding box
    virtual void GetBoundingBox( int& xMin, int& yMin, int& xMax, int& yMax ) const;

    //gets the size of the bounding box
    virtual void GetBoundingBoxSize( int& width, int& height ) const;

    virtual void ScaleAboutCenter( float factor );
    virtual void ScaleAboutCenter( float factorX, float factorY, float factorZ );
    virtual void ScaleAboutPoint( float factor, const int x, const int y );

    virtual void RotateAboutCenter( float angle, rmt::Vector axis = rmt::Vector( 0, 0, 1 ) );

    void Display() { FeOwner::Display(); }
  
    void Show();    // called on screen update
protected:
    virtual ~FeGroup();

    //this recomputes the offsets and the bounding boxes for all the drawables
    void RecomputeOffsets();
    
    bool m_offsetsComputed;
};

#endif
