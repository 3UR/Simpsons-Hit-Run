//===========================================================================
// Copyright (C) 2000 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   Scrooby Runtime Public Interface
//
// Description: This contains the bounded drawable class.
//
// Authors:     Eric Tong
//
//===========================================================================

// Recompilation protection flag.
#ifndef SCROOBY_BOUNDEDDRAWABLE_H
#define SCROOBY_BOUNDEDDRAWABLE_H

//===========================================================================
// Nested Includes
//===========================================================================
#include "Drawable.h"
#include "Enums.h"


namespace Scrooby
{
//===========================================================================
// Interface Definitions
//===========================================================================

//===========================================================================
//
// Description: This class is used to represent a front-end page
//
// Constraints: None
//
//===========================================================================

class HasBoundingBox
:
    virtual public Drawable
{
public:
    virtual void GetBoundingBox( int& xMin, int& yMin, int& xMax, int& yMax ) const = 0;
    virtual void ScaleAboutCenter( float factor ) = 0;
    virtual void ScaleAboutCenter( float factorX, float factorY, float factorZ ) = 0;
};

class BoundedDrawable 
: 
    virtual public HasBoundingBox
{
    public:
        virtual void SetBoundingBoxSize( int width, int height ) = 0;
        virtual void SetHorizontalJustification( JustificationEnum justification ) = 0;
        virtual void SetVerticalJustification( JustificationEnum justification ) = 0;
        virtual JustificationEnum GetHorizontalJustification() const = 0;
        virtual JustificationEnum GetVerticalJustification() const = 0;
        virtual void GetBoundingBoxSize( int& width, int& height ) const = 0;
        virtual void GetBoundingBoxCenter( int& centerX, int& centerY ) const = 0;
        virtual void ScaleAboutPoint( const float factor, const int x, const int y ) = 0;
        virtual void ScaleAboutPoint( const float factorX, const float factorY, const float factorZ, const int x, const int y ) = 0;
        virtual void SetPositionOfCenter( const int x, const int y ) = 0;
        virtual void RotateAboutCenter( float angle, rmt::Vector axis = rmt::Vector( 0, 0, 1 ) ) = 0;
        virtual bool IsPointInBoundingRect( float x, float y ) = 0;
        virtual void StretchBoundingBox( const int amountX, const int amountY ) = 0;
        virtual void StretchBoundingBox( const float factorX, const float factorY ) = 0;

    protected:
    private:
};

} // End namespace

#endif // End conditional inclusion
