//===========================================================================
// Copyright (C) 2000 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   Scrooby Runtime Public Interface
//
// Description: This contains the BoundedDrawable class.
//
// Authors:     Eric Tong
//
//===========================================================================

//===========================================================================
// Includes
//===========================================================================

#include "FeBoundedDrawable.h"
#include <raddebug.hpp>
#include <p3d/drawable.hpp>
#include <p3d/matrixstack.hpp>
#include <p3d/utility.hpp>
#include <p3d/shader.hpp>
#include "FeApp.h"

//===========================================================================
// Local Constants, Typedefs, and Macros
//===========================================================================

//===========================================================================
// Global Data, Local Data, Local Classes
//===========================================================================

//===========================================================================
// Member Functions
//===========================================================================

//===========================================================================
// FeBoundedDrawable::FeBoundedDrawable
//===========================================================================
// Description: FeBoundedDrawable's constructor
//
// Constraints:    None
//
// Parameters:    name - the name of this drawable
//
// Return:      Nothing
//
//===========================================================================
FeBoundedDrawable::FeBoundedDrawable( const tName& name )
:
    FeDrawable( name ),
    m_drawable( NULL )
{
    //
    // Always use explicit qualification when calling a virtual function from
    // a constructor.
    //
    FeBoundedDrawable::Reset();
}

//===========================================================================
// FeBoundedDrawable::~FeBoundedDrawable
//===========================================================================
// Description: FeBoundedDrawable's destructor
//
// Constraints:    None
//
// Parameters:    None
//
// Return:      Nothing
//
//===========================================================================
FeBoundedDrawable::~FeBoundedDrawable()
{
}

void FeBoundedDrawable::DisplayBoundingBox( tColour colour )
{
    // The reason that our origin position is set to zero is because we have already translated by the origin
    // position to draw the object
    float height, width;
    NormalizeCoord( m_width, m_height, width, height );

    tShader* testMat = p3d::find<tShader>("p3d_default");

    pddiPrimStream* stream = p3d::pddi->BeginPrims( testMat->GetShader(), PDDI_PRIM_LINESTRIP, PDDI_V_C, 5);
    stream->Colour( colour );
    stream->Coord( -m_offsetX, -m_offsetY, 0.0f );
    stream->Colour( colour );
    stream->Coord( width, -m_offsetY, 0.0f );
    stream->Colour( colour );
    stream->Coord( width, height, 0.0f );
    stream->Colour( colour );
    stream->Coord( -m_offsetX, height, 0.0f );
    stream->Colour( colour );
    stream->Coord( -m_offsetX, -m_offsetY, 0.0f );
    p3d::pddi->EndPrims( stream );
}

//===========================================================================
// GetBoundingBox
//===========================================================================
// Description: gets the extents of the bounding box
//
// Constraints:    
//
// Parameters:    xMin - the smallext x value
//              yMin - the smallest y value
//              xMax - the largest x value
//              yMax - the largest y value
//
// Return:      NONE
//
//===========================================================================
void FeBoundedDrawable::GetBoundingBox( int& xMin, int& yMin, int& xMax, int& yMax ) const
{
    int originX;
    int originY;
    FeDrawable::GetOriginPosition( originX, originY );

    xMin = originX;
    yMin = originY;
    xMax = xMin + m_width;
    yMax = yMin + m_height;
}

//===========================================================================
// void FeBoundedDrawable::GetBoundingBoxSize( int& width, int& height ) const
//===========================================================================
// Description: get the size of the bounding box
//
// Constraints:    None
//
// Parameters:    None
//
// Return:      Nothing
//
//===========================================================================
void FeBoundedDrawable::GetBoundingBoxSize( int& width, int& height ) const
{
    width   = this->m_width;
    height  = this->m_height;
}

//===========================================================================
// void FeBoundedDrawable::GetBoundingBoxCenter
//===========================================================================
// Description: get the center of the bounding box
//
// Constraints:    None
//
// Parameters:    None
//
// Return:      Nothing
//
//===========================================================================
void
FeBoundedDrawable::GetBoundingBoxCenter( int& centerX, int& centerY ) const
{
    int posX = 0;
    int posY = 0;
    this->GetOriginPosition( posX, posY );

    centerX = posX + m_width / 2;
    centerY = posY + m_height / 2;
}

//===========================================================================
// FeBoundedDrawable::IsPointInBoundingRect
//===========================================================================
// Description: Calculates if a point is the bounding Rectangle (Scrooby calls
//              bounding box).
//
// Constraints:    None
//
// Parameters:    P3D point.
//
// Return:      True if the point is inside the rect.
//
//===========================================================================
bool FeBoundedDrawable::IsPointInBoundingRect( float x, float y )
{
    float height = FeApp::GetInstance()->GetScreenHeight();
    float width = FeApp::GetInstance()->GetScreenWidth();

    //float height = 768;
    //float width = 1024;

    const float MAX_OFFSET = 1.0f;
    const float MIN_OFFSET = -1.0f;
    const float ASPECT = width/height; // The screen aspect ratio.

    int ix, iy;
    // convert from p3d point to scrooby screen point.

    x = (x/2.0f) + 0.5f;
    y = (((y*ASPECT)/2.0f) + 0.5f)/ASPECT;

    InverseNormalizeCoord( x, y, ix, iy );

    int xMin, yMin, xMax, yMax;
    GetBoundingBox( xMin, yMin, xMax, yMax );

    return ((ix <= xMax) && (ix >= xMin) && (iy <= yMax) && (iy >= yMin));
}

//===========================================================================
// FeBoundedDrawable::Reset
//===========================================================================
// Description: Reset to default values
//
// Constraints:    None
//
// Parameters:    None
//
// Return:      None
//
//===========================================================================
void FeBoundedDrawable::Reset()
{
    FeDrawable::Reset();

    m_width = 0;
    m_height = 0;
    m_horizontalJustification = Scrooby::Left;
    m_verticalJustification = Scrooby::Top;
    m_isDirty = true;
}

//===========================================================================
// FeBoundedDrawable::SetHorizontalJustification
//===========================================================================
// Description: Set the horizontal justification, updates on redraw
//
// Constraints:    None
//
// Parameters:    justification - the justification
//
// Return:      None
//
//===========================================================================
void FeBoundedDrawable::SetHorizontalJustification( Scrooby::JustificationEnum justification )
{
    m_horizontalJustification = justification;
    m_isDirty = true;
}

//===========================================================================
// FeBoundedDrawable::SetVerticalJustification
//===========================================================================
// Description: Set the vertical justification, updates on redraw
//
// Constraints:    None
//
// Parameters:    justification - the justification
//
// Return:      None
//
//===========================================================================
void FeBoundedDrawable::SetVerticalJustification( Scrooby::JustificationEnum justification )
{
    m_verticalJustification = justification;
    m_isDirty = true;
}

//===========================================================================
// UpdatePosition()
//===========================================================================
// Description: translates the transformation matrix to reflect the position
//
// Constraints:    NONE
//
// Parameters:    NONE
//
// Return:      NONE
//
//===========================================================================
void FeBoundedDrawable::UpdatePosition() const
{
    p3d::stack->Translate( m_offsetX, m_offsetY, 0 );
}

//===========================================================================
// FeBoundedDrawable::SetBoundingBoxSize
//===========================================================================
// Description: Set the bounding box size, updates on redraw
//
// Constraints:    None
//
// Parameters:    width - width in pixels
//              height - height in pixels
//
// Return:      None
//
//===========================================================================
void FeBoundedDrawable::SetBoundingBoxSize( int width, int height )
{
    m_width = width;
    m_height = height;
    m_isDirty = true;
}

//===========================================================================
// FeBoundedDrawable::ReCalculateAlignment
//===========================================================================
// Description: Recalculates the alignment offsets
//
// Constraints:    None
//
// Parameters:    None
//
// Return:      None
//
//===========================================================================
void FeBoundedDrawable::ReCalculateAlignment()
{
    m_offsetX = 0;
    m_offsetY = 0;
    if( m_drawable )
    {
        float height = 0.0f;
        float width = 0.0f;
        float boxWidth = 0.0f;
        float boxHeight = 0.0f;

        rmt::Box3D box;
        m_drawable->GetBoundingBox( &box );
        width = box.high.x - box.low.x;
        height = box.high.y - box.low.y;
        NormalizeCoord( m_width, m_height, boxWidth, boxHeight );

        switch( m_horizontalJustification )
        {
        case Scrooby::Right :
            m_offsetX = boxWidth - width;
            break;
        case Scrooby::Centre :
            m_offsetX = (boxWidth - width) / 2.0f;
            break;
        case Scrooby::Left :
        default :
            break;
        }

        switch( m_verticalJustification )
        {
        case Scrooby::Top :
            m_offsetY = (boxHeight - height);
            break;
        case Scrooby::Centre :
            m_offsetY = (boxHeight - height) / 2.0f;
            break;
        case Scrooby::Bottom :
        default :
            break;
        }
    }
}

//===========================================================================
// FeBoundedDrawable::GetMatrix
//===========================================================================
// Description: Returns the final matrix, after applying alignment
//
// Constraints:    None
//
// Parameters:    None
//
// Return:      rmt::Matrix* - pointer to the final matrix
//
//===========================================================================
rmt::Matrix* FeBoundedDrawable::GetMatrix()
{
    // Do we need to recalculate the offsets?
    if( m_isDirty )
    {
        ReCalculateAlignment();
        m_isDirty = false;
    }

    static rmt::Matrix tmpMatrix;           //IAN IMPROVE: this does not seem wise - returning a pointer to a static?
    static rmt::Matrix finalMatrix;
    
    // Apply the offset matrix
    tmpMatrix.Identity();
    tmpMatrix.FillTranslate( rmt::Vector( m_offsetX, m_offsetY, 0 ) );
    finalMatrix.Mult( tmpMatrix, m_matrix );

    return &finalMatrix;
}

//===========================================================================
// GetMatrix
//===========================================================================
// Description: Returns the final matrix, after applying alignment
//
// Constraints:    None
//
// Parameters:    None
//
// Return:      rmt::Matrix* - pointer to the final matrix
//
//===========================================================================
const rmt::Matrix* FeBoundedDrawable::GetMatrix() const
{
    // Do we need to recalculate the offsets?
    if( m_isDirty )
    {
        FeBoundedDrawable* drawable = const_cast<FeBoundedDrawable*>(this);
        drawable->ReCalculateAlignment();
        m_isDirty = false;
    }

    static rmt::Matrix tmpMatrix;       //IAN IMPROVE: this does not seem wise
    static rmt::Matrix finalMatrix;
    
    // Apply the offset matrix
    tmpMatrix.Identity();
    tmpMatrix.FillTranslate( rmt::Vector( m_offsetX, m_offsetY, 0 ) );
    finalMatrix.Mult( tmpMatrix, m_matrix );

    return &finalMatrix;
}

//===========================================================================
// ScaleAboutCenter
//===========================================================================
// Description: Scale uniformly about bounded drawable's center point.
//
// Constraints:    None
//
// Parameters:    None
//
// Return:      None
//
//===========================================================================
void FeBoundedDrawable::ScaleAboutCenter( float factor )
{
    ScaleAboutCenter( factor, factor, 1.0f );
}

//===========================================================================
// ScaleAboutCenter
//===========================================================================
// Description: Scale about bounded drawable's center point.
//
// Constraints:    None
//
// Parameters:    None
//
// Return:      None
//
//===========================================================================
void FeBoundedDrawable::ScaleAboutCenter( float factorX, float factorY, float factorZ )
{
    int originPosX = 0;
    int originPosY = 0;

    // get drawable's origin co-ordinates
    GetOriginPosition( originPosX, originPosY );

    // determine drawable's center co-ordinates
    int centerPosX = originPosX + m_width / 2;
    int centerPosY = originPosY + m_height / 2;

    // translate drawable to screen origin first
    Translate( -centerPosX, -centerPosY );

    // do the scaling
    Scale( factorX, factorY, factorZ );

    // translate drawable back to original position
    Translate( centerPosX, centerPosY );
}

//===========================================================================
// ScaleAboutPoint
//===========================================================================
// Description: Scale about a point on the bounded drawable
//
// Constraints:    None
//
// Parameters:    None
//
// Return:      None
//
//===========================================================================
void FeBoundedDrawable::ScaleAboutPoint( const float factor, const int x, const int y )
{
    int originPosX = 0;
    int originPosY = 0;

    // get drawable's origin co-ordinates
    GetOriginPosition( originPosX, originPosY );

    // determine drawable's center co-ordinates
    int centerPosX = originPosX + x;
    int centerPosY = originPosY + y;

    // translate drawable to screen origin first
    Translate( -centerPosX, -centerPosY );

    // do the scaling
    Scale( factor, factor, factor );

    // translate drawable back to original position
    Translate( centerPosX, centerPosY );
}

//===========================================================================
// ScaleAboutPoint
//===========================================================================
// Description: Scale about a point on the bounded drawable
//
// Constraints:    None
//
// Parameters:    None
//
// Return:      None
//
//===========================================================================
void FeBoundedDrawable::ScaleAboutPoint( const float factorX, const float factorY, const float factorZ,
                                         const int x, const int y )
{
    int originPosX = 0;
    int originPosY = 0;

    // get drawable's origin co-ordinates
    GetOriginPosition( originPosX, originPosY );

    // determine drawable's center co-ordinates
    int centerPosX = originPosX + x;
    int centerPosY = originPosY + y;

    // translate drawable to screen origin first
    Translate( -centerPosX, -centerPosY );

    // do the scaling
    Scale( factorX, factorY, factorZ );

    // translate drawable back to original position
    Translate( centerPosX, centerPosY );
}

//===========================================================================
// FeBoundedDrawable::SetPositionOfCenter
//===========================================================================
// Description: set the position of the object's center
//
// Constraints: None
//
// Parameters:  x,y the x,y position of the cneter
//
// Return:      None
//
//===========================================================================
void FeBoundedDrawable::SetPositionOfCenter( const int x, const int y )
{
    SetPosition( x - m_width / 2, y - m_height / 2 );
}

//===========================================================================
// RotateAboutCenter
//===========================================================================
// Description: Rotate about bounded drawable's center point.
//
// Constraints:    None
//
// Parameters:    None
//
// Return:      None
//
//===========================================================================
void FeBoundedDrawable::RotateAboutCenter( float angle, rmt::Vector axis )
{
    int originPosX = 0;
    int originPosY = 0;

    // get drawable's origin co-ordinates
    GetOriginPosition( originPosX, originPosY );

    // TC: why is the center not really the center???
    //
    static int centerOffsetX = -1;
    static int centerOffsetY = 1;

    // determine drawable's center co-ordinates
    int centerPosX = originPosX + m_width / 2 + centerOffsetX;
    int centerPosY = originPosY + m_height / 2 + centerOffsetY;

    // translate drawable to screen origin first
    Translate( -centerPosX, -centerPosY );

    // do the rotation(s)
    //
    RotateArbitrary( axis.x, axis.y, axis.z, angle );

    // translate drawable back to original position
    Translate( centerPosX, centerPosY );
}

//===========================================================================
// StretchBoundingBox
//===========================================================================
// Description: Scales the bounding box with respect to horizontal and
//              vertical justification.
//
// Constraints: None
//
// Parameters:  None
//
// Return:      None
//
//===========================================================================
void FeBoundedDrawable::StretchBoundingBox( const int amountX, const int amountY )
{
    int newWidth = m_width + amountX;
    int newHeight = m_height + amountY;

    int newPosX = 0;
    int newPosY = 0;
    this->GetOriginPosition( newPosX, newPosY );

    if( m_horizontalJustification == Scrooby::Center )
    {
        newPosX -= amountX / 2;
    }
    else if( m_horizontalJustification == Scrooby::Right )
    {
        newPosX -= amountX;
    }

    if( m_verticalJustification == Scrooby::Center )
    {
        newPosY -= amountY / 2;
    }
    else if( m_verticalJustification == Scrooby::Top )
    {
        newPosY -= amountY;
    }

    this->SetPosition( newPosX, newPosY );
    this->SetBoundingBoxSize( newWidth, newHeight );
}

//===========================================================================
// StretchBoundingBox
//===========================================================================
// Description: Scales the bounding box with respect to horizontal and
//              vertical justification.
//
// Constraints: None
//
// Parameters:  None
//
// Return:      None
//
//===========================================================================
void FeBoundedDrawable::StretchBoundingBox( const float factorX, const float factorY )
{
    int newWidth = static_cast<int>( m_width * factorX );
    int newHeight = static_cast<int>( m_height * factorY );

    this->StretchBoundingBox( newWidth - m_width, newHeight - m_height );
}

