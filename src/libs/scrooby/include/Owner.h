//===========================================================================
// Copyright (C) 2000 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   Scrooby Runtime Public Interface
//
// Description: This contains the Owner class.
//
// Authors:     Ian Gipson
//
//===========================================================================

// Recompilation protection flag.
#ifndef SCROOBY_OWNER_H
#define SCROOBY_OWNER_H

//===========================================================================
// Nested Includes
//===========================================================================
#include "Drawable.h"

namespace Scrooby
{
    class Group;
    class Polygon;
    class Pure3dObject;
    class Sprite;
    class Text;
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

class Owner 
{
    public:
        virtual Drawable* GetChildDrawable( const unsigned int index ) = 0;
            
        //gets a group object from within the page by hash
        virtual Group*  GetGroup( const tUID hashValue )  = 0;

        //gets a text object from within the page by name
        virtual Group*  GetGroup( const char* name )      = 0;

        //gets a polygon object from within the page by hash
        virtual Polygon* GetPolygon( const tUID hashValue ) = 0;

        //gets a text object from within the page by name
        virtual Polygon* GetPolygon( const char* name ) = 0;

        //gets a pure3d  object from within the page by hash
        virtual Pure3dObject* GetPure3dObject( const tUID hashValue ) = 0;

        //gets a pure3d object from within the page by name
        virtual Pure3dObject* GetPure3dObject( const char* name ) = 0;

        //gets a sprite from within the page by hash 
        virtual Sprite* GetSprite( const tUID hashValue ) = 0;

        //gets a sprite from within the page by name
        virtual Sprite* GetSprite( const char* name )     = 0;

        //gets a text object from within the page by hash
        virtual Text*   GetText( const tUID hashValue )   = 0;
        
        //gets a text object from within the page by name
        virtual Text*   GetText( const char* name )       = 0;

        virtual unsigned int Size() const = 0;

    protected:
    private:
};

} // End namespace

#endif // End conditional inclusion
