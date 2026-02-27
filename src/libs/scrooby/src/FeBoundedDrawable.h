//===========================================================================
// Copyright (C) 2000 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   Scrooby Runtime Interface
//
// Description: This contains the Bounded Drawable Base class.
//
// Authors:     Eric Tong
//
//===========================================================================


// Recompilation protection flag.
#ifndef __FeBoundedDrawable__
#define __FeBoundedDrawable__

//===========================================================================
// Nested Includes
//===========================================================================
#include "FeDrawable.h"
#include "BoundedDrawable.h"
#include "utility/EnumConversion.h"

//===========================================================================
// Forward References
//===========================================================================

//===========================================================================
// Constants, Typedefs, and Macro Definitions (needed by external clients)
//===========================================================================
#if defined(RAD_WIN32) || defined(RAD_UWP)
#pragma warning( disable : 4250 )
#endif

//===========================================================================
// Interface Definitions
//===========================================================================

//===========================================================================
//
// Description: This class is used to represent any drawable front end element
//
// Constraints: None
//
//===========================================================================
class FeBoundedDrawable
: 
    public FeDrawable,
    virtual public Scrooby::BoundedDrawable
{
public:
    FeBoundedDrawable( const tName& name );

    //get the extents of the bounding box
    virtual void GetBoundingBox( int& xMin, int& yMin, int& xMax, int& yMax ) const;

    //get the size of the bounding box
    virtual void GetBoundingBoxSize( int& width, int& height ) const;

    //get the center point of the bounding box
    virtual void GetBoundingBoxCenter( int& centerX, int& centerY ) const;

    virtual bool IsPointInBoundingRect( float x, float y );

    //gets the matrix associated with this bounded drawable
    virtual rmt::Matrix* GetMatrix();
    virtual const rmt::Matrix* GetMatrix() const;

    virtual void Reset();
    
    void SetHorizontalJustification( Scrooby::JustificationEnum justification );
    void SetVerticalJustification( Scrooby::JustificationEnum justification );

    Scrooby::JustificationEnum GetHorizontalJustification() const
    {
        return m_horizontalJustification;
    }

    Scrooby::JustificationEnum GetVerticalJustification() const
    {
        return m_verticalJustification;
    }

    void SetBoundingBoxSize( int width, int height );
    void SetObjectSize( int width, int height );
    
    //translates the transformation matrix to reflect the position
    void UpdatePosition() const;

    void DisplayBoundingBox( tColour colour = tColour( 255, 255, 255 ) );

    // Transformations About Bounded Drawable's Center
    //
    void ScaleAboutCenter( float factor );
    void ScaleAboutCenter( float factorX, float factorY, float factorZ );

    virtual void ScaleAboutPoint( const float factor,
                                  const int x, const int y );

    virtual void ScaleAboutPoint( const float factorX, const float factorY, const float factorZ,
                                  const int x, const int y );

    virtual void SetPositionOfCenter( const int x, const int y );
    void RotateAboutCenter( float angle,
                            rmt::Vector axis = rmt::Vector( 0, 0, 1 ) );

    // Scales Bounding Box wrt. Justification
    //
    virtual void StretchBoundingBox( const int amountX, const int amountY );
    virtual void StretchBoundingBox( const float factorX, const float factorY );

protected:
    virtual ~FeBoundedDrawable();

    int m_width;
    int m_height;

    Scrooby::JustificationEnum m_horizontalJustification;
    Scrooby::JustificationEnum m_verticalJustification;

    tDrawable* m_drawable;

    mutable bool m_isDirty;

    float m_offsetX;
    float m_offsetY;

    virtual void ReCalculateAlignment();

private:

};

#endif
