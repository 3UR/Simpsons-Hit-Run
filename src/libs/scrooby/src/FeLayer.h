// FeLayer.h
// Created by wng on Mon, Mar 20, 2000 @ 5:20 PM.

#ifndef __FeLayer__
#define __FeLayer__

#include "FeDrawable.h"
#include "FeOwner.h"
#include "Layer.h"

#if defined( RAD_WIN32 ) || defined( RAD_UWP )
#pragma warning( disable :4250 )
#endif

class FeLayer 
: 
    public FeOwner,
    virtual public Scrooby::Layer
{
public:
    FeLayer( const char* name );
    virtual ~FeLayer();

    void Display() { FeOwner::Display(); }

    virtual void GetBoundingBoxSize( int& width, int& height ) const;
    virtual void GetBoundingBox( int& xMin, int& yMin, int& xMax, int& yMax ) const;

private:
    void HaveNewObject(FeDrawable *s);
friend class FeDebug;
}; 

#endif
