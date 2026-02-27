//===========================================================================
// Copyright (C) 2000 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   Scrooby Runtime Public Interface
//
// Description: This contains the Group class.
//
// Authors:     Ian Gipson
//
//===========================================================================

// Recompilation protection flag.
#ifndef SCROOBY_GROUP_H
#define SCROOBY_GROUP_H

//===========================================================================
// Nested Includes
//===========================================================================
#include "boundeddrawable.h"
#include "Owner.h"

namespace Scrooby
{
//===========================================================================
// Interface Definitions
//===========================================================================

//===========================================================================
//
// Description: This class is used to represent a Scrooby Group element
//
// Constraints: None
//
//===========================================================================

class Group 
:
    virtual public Owner,
    virtual public HasBoundingBox
{
    public:
        virtual void GetBoundingBoxSize( int& width, int& height ) const = 0;
        virtual void GetBoundingBox( int& xMin, int& yMin, int& xMax, int& yMax ) const = 0;
        virtual void ScaleAboutCenter( float factor ) = 0;
        virtual void ScaleAboutCenter( float factorX, float factorY, float factorZ ) = 0;
        virtual void ScaleAboutPoint( float factor, const int x, const int y ) = 0;
        virtual void RotateAboutCenter( float angle, rmt::Vector axis = rmt::Vector( 0, 0, 1 ) ) = 0;
    protected:
    private:
};

} // End namespace

#endif // End conditional inclusion
