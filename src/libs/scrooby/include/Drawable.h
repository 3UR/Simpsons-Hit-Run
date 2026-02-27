//===========================================================================
// Copyright (C) 2000 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   Scrooby Runtime Public Interface
//
// Description: This contains the Drawable Base class.
//
// Authors:     Ian Gipson
//
//===========================================================================

// Recompilation protection flag.
#ifndef SCROOBY_DRAWABLE_H
#define SCROOBY_DRAWABLE_H

#include <p3d/p3dtypes.hpp>
#include <raddebugwatch.hpp>

namespace Scrooby
{
//===========================================================================
// Interface Definitions
//===========================================================================

//===========================================================================
//
// Description: This class is used to represent any Scrooby Drawable element.
//
// Constraints: None
//
//===========================================================================

class Drawable 
{
    public:
        
        virtual void Display() = 0;
        //allows access to the alpha value
        virtual float GetAlpha() const = 0;

        //allows access to the colour value
        virtual const tColour& GetColour() const = 0;

        //allows access to the original position of the object
        virtual void GetOriginPosition( int& x, int& y ) const = 0;

        virtual void GetBoundingBoxSize( int& width, int& height ) const = 0;
 
        virtual void GetBoundingBox( int& xMin, int& yMin, int& xMax, int& yMax ) const = 0;
        
        virtual void GetNormalizedPosition(float &x, float &y) const = 0;
        
        //allows access to the matrix
        virtual rmt::Matrix* GetTransform() = 0;

        //allows you to determine if a drawable is visible or not
        virtual bool IsVisible() const = 0;

        //resets the transformation matirx to the indentity matrix
        virtual void ResetTransformation() = 0;

        //ARBITRARY angle rotation
        virtual void RotateArbitrary( float axisX, float axisY, float axisZ, float angle ) = 0;

        //rotation about one of the coordinate axes
        virtual void RotateX( float angleX ) = 0;

        //rotation about one of the coordinate axes
        virtual void RotateY( float angleY ) = 0;

        //rotation about one of the coordinate axes
        virtual void RotateZ( float angleZ ) = 0;

        //uniform scaling function
        virtual void Scale( float factor ) = 0;

        //non uniform scaling operator
        virtual void Scale( float factorX, float factorY, float factorZ ) = 0;

        //sets the alpha value for display purposes
        virtual void SetAlpha( float a ) = 0;

        //sets the colour value for display purposes
        virtual void SetColour( tColour c ) = 0;

        //sets the origin position
        virtual void SetPosition( int x, int y ) = 0;

        //allows access to the transformation matrix
        virtual void SetTransform( rmt::Matrix* matrix ) = 0;
        virtual void SetTransform( const rmt::Matrix& matrix ) = 0;

        //equivilant to translate
        virtual void SetTranslation( int x, int y ) = 0;
    
        //turns on or off the object
        virtual void SetVisible( bool v ) = 0;

        //resets the position, and translates to the position
        virtual void Translate( int x, int y ) = 0;

        #ifdef DEBUGWATCH
        virtual void WatchAll       ( const char* nameSpace ) = 0;
        virtual void WatchColor     ( const char* nameSpace ) = 0;
        virtual void WatchPosition  ( const char* nameSpace ) = 0;
        virtual void WatchVisibility( const char* nameSpace ) = 0;
        #endif
    
    protected:
    private:
};

} // End namespace

#endif // End conditional inclusion
