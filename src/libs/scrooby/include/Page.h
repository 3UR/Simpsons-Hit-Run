//===========================================================================
// Copyright (C) 2000 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   Scrooby Runtime Public Interface
//
// Description: This contains the Scrooby::Page class.
//
// Authors:     Ian Gipson
//
//===========================================================================

// Recompilation protection flag.
#ifndef SCROOBY_PAGE_H
#define SCROOBY_PAGE_H

//===========================================================================
// Nested Includes
//===========================================================================
#include <p3d/p3dtypes.hpp>  // for tUID
#include "drawable.h"
#include "owner.h"

namespace Scrooby
{
//===========================================================================
// Forward References
//===========================================================================
class Layer;
class Group;
class Sprite;
class Polygon;
class Text;
class Movie;
class Pure3dObject;

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

class Page 
: 
    virtual public Drawable,
    virtual public Owner
{
    public:
        //gets a movie object from within the page by hash
        virtual Movie* GetMovie( const tUID hashValue ) = 0;

        //gets a movie object from within the page by name
        virtual Movie* GetMovie( const char* name ) = 0;

           //gets a text object from within the page by name
        virtual int GetNumberOfLayers() const = 0;
        
        //gets a layer object from within the page by hash
        virtual Layer* GetLayer( const tUID hashValue ) = 0;

        //gets a text object from within the page by name
        virtual Layer* GetLayer( const char* name ) = 0;

        //gets a text object from within the page by name
        virtual Layer* GetLayerByIndex( const int index ) = 0;

    protected:
    private:
};

} // End namespace

#endif // End conditional inclusion
