// FeGroup.cpp
// Created by wng on Thu, May 11, 2000 @ 6:51 PM.
#include "stdafx.h"
#ifndef __FeGroup__
#include "FeGroup.h"
#endif
#include "tLinearTable.h"

FeGroup::FeGroup( const tName& name ) 
: 
    FeOwner( name ),
    m_offsetsComputed( false )
{
    //mHandle = Fe2DCore::GetInstance()->AddDummy( this );
}


FeGroup::~FeGroup()
{
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
void FeGroup::GetBoundingBox( int& xMin, int& yMin, int& xMax, int& yMax ) const
{
    bool initialized = false;
       int childrenCount = this->GetChildrenCount();
    for( int i = 0; i < childrenCount; i++ )
    {
        const FeEntity* child = this->GetChildIndex( i );
        const FeDrawable* drawable = dynamic_cast< const FeDrawable* >( child );
        if( drawable != NULL )
        {
            int dxMin;
            int dxMax;
            int dyMin;
            int dyMax;

            drawable->GetBoundingBox( dxMin, dyMin, dxMax, dyMax );

            if( initialized == false )
            {
                xMin = dxMin;
                xMax = dxMax;
                yMin = dyMin;
                yMax = dyMax;
                initialized = true;
            }
            else
            {
                if( dxMin < xMin )
                {
                    xMin = dxMin;
                }

                if( dxMax > xMax )
                {
                    xMax = dxMax;
                }

                if( dyMin < yMin )
                {
                    yMin = dyMin;
                }

                if( dyMax > yMax )
                {
                    yMax = dyMax;
                }
            }
        }
    }
    int x, y;
    this->GetOriginPosition( x,y);
    xMin += x;
    xMax += x;
    yMin += y;
    yMax += y;


}

void FeGroup::Show()
{
    //check if we need ot recompute the offsets of this item
    if( m_offsetsComputed == false )
    {
        RecomputeOffsets();
    }
    
    FeOwner::Show();
}

void FeGroup::GetBoundingBoxSize( int& width, int& height ) const
{
    int xMin;
    int xMax;
    int yMin;
    int yMax;
    this->GetBoundingBox( xMin, yMin, xMax, yMax );
    width = xMax - xMin;
    height = yMax - yMin;
}


//===========================================================================
// RecomputeOffsets
//===========================================================================
// Description: adjusts the positions of the bounding box, so that the 
//              children's positions will be relative to it
//
// Constraints:    
//
// Parameters:       width - the width is returned here
//                 height - the height is returned here
//
// Return:      
//
//===========================================================================
void FeGroup::RecomputeOffsets()
{
    //first find the minX and minY for all the children
    int minX = 65535;   //IAN IMPROVE: this is assumed to be larger than the posiiton
    int minY = 65535;   //of any of the drawables

    tLinearTable::RawIterator iter( mChildren );
    FeDrawable* drawable = dynamic_cast< FeDrawable* >( iter.First() );
    while ( drawable != NULL )
    {
        int drawableMinX;
        int drawableMaxX;
        int drawableMinY;
        int drawableMaxY;
        drawable->GetBoundingBox( drawableMinX, drawableMinY, drawableMaxX, drawableMaxY );
      
        if( drawableMinX < minX )
        {
            minX = drawableMinX;
        }

        if( drawableMinY < minY )
        {
            minY = drawableMinY;
        }
        drawable = dynamic_cast<FeDrawable*>( iter.Next() );
    }


    //go through and translate all the children down and left by xMin, yMin
    tLinearTable::RawIterator iter2( mChildren );
    drawable = dynamic_cast< FeDrawable* >( iter2.First() );
    while ( drawable )
    {
        drawable->TranslatePosition( -minX, -minY );
        drawable = dynamic_cast<FeDrawable*>( iter2.Next() );
    }

    this->SetPosition( minX, minY );

    m_offsetsComputed = true;
};

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
void FeGroup::ScaleAboutCenter( float factor )
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
void FeGroup::ScaleAboutCenter( float factorX, float factorY, float factorZ )
{
    int xmin;
    int ymin;
    int xmax;
    int ymax;
    GetBoundingBox( xmin, ymin, xmax, ymax );
    int centerPosX = ( xmin + xmax ) / 2;
    int centerPosY = ( ymin + ymax ) / 2;

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
// Description: Scale about a point on the bounded group
//
// Constraints:    None
//
// Parameters:    None
//
// Return:      None
//
//===========================================================================
void FeGroup::ScaleAboutPoint( float factor, const int x, const int y )
{
    int xmin;
    int ymin;
    int xmax;
    int ymax;
    GetBoundingBox( xmin, ymin, xmax, ymax );

    int pointX = xmin + x;
    int pointY = ymin + y;

    // translate drawable to screen origin first
    Translate( -pointX, -pointY );

    // do the scaling
    Scale( factor, factor, factor );

    // translate drawable back to original position
    Translate( pointX, pointY );
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
void FeGroup::RotateAboutCenter( float angle, rmt::Vector axis )
{
    int xmin = 0;
    int ymin = 0;
    int xmax = 0;
    int ymax = 0;
    GetBoundingBox( xmin, ymin, xmax, ymax );

    int centerPosX = ( xmin + xmax ) / 2;
    int centerPosY = ( ymin + ymax ) / 2;

    // translate drawable to screen origin first
    Translate( -centerPosX, -centerPosY );

    // do the rotation(s)
    //
    RotateArbitrary( axis.x, axis.y, axis.z, angle );

    // translate drawable back to original position
    Translate( centerPosX, centerPosY );
}

