//===========================================================================
// Copyright (C) 2000 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   Scrooby Runtime Public Interface
//
// Description: This contains the Scrooby::Polygon class.
//
// Authors:     Ian Gipson
//
//===========================================================================

// Recompilation protection flag.
#ifndef SCROOBY_POLYGON_H
#define SCROOBY_POLYGON_H

//===========================================================================
// Nested Includes
//===========================================================================
#include "Drawable.h"
#include <p3d/p3dtypes.hpp>

namespace Scrooby
{
//===========================================================================
// Interface Definitions
//===========================================================================

//===========================================================================
//
// Description: This class is used to represent a Scrooby Polygon element
//
// Constraints: None
//
//===========================================================================

class Polygon 
: 
    virtual public Drawable
{
    public:
        virtual float   GetVertexAlpha( const int which ) const = 0;
        virtual tColour GetVertexColour( const int which ) const = 0;
        virtual void    GetVertexLocation( const int which, int& left, int& bottom ) const = 0;
        virtual void    SetVertexLocation(int which,int left,int bottom) = 0;
        virtual void    SetVertexColour(int which,int red,int green,int blue) = 0;
        virtual void    SetVertexColour( const unsigned int which, const tColour c ) = 0;
        virtual void    SetVertexAlpha(int which,float alpha) = 0;
        virtual int     GetNumOfVertexes() = 0;

    protected:
    private:
};

} // End namespace

#endif // End conditional inclusion
