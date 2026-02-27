//===========================================================================
// Copyright (C) 2000 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   Scrooby Runtime Public Interface
//
// Description: This contains the Scrooby::Screen class.
//
// Authors:     Ian Gipson
//
//===========================================================================

// Recompilation protection flag.
#ifndef SCROOBY_SCREEN_H
#define SCROOBY_SCREEN_H

//===========================================================================
// Nested Includes
//===========================================================================
#include "Drawable.h"
#include <p3d/p3dtypes.hpp> // for tUID

namespace Scrooby
{
//===========================================================================
// Forward References
//===========================================================================
class Page;

//===========================================================================
// Interface Definitions
//===========================================================================

//===========================================================================
//
// Description: This class is used to represent a Scrooby Screen
//
// Constraints: None
//
//===========================================================================

class Screen
:
    virtual public Drawable
{
    public:

        //reload a screen
        virtual void Reload() = 0;

        //get a page by hashValue
        virtual Page* GetPage( const tUID hashValue ) = 0;

        //get a page by name
        virtual Page* GetPage( const char* name ) = 0;

        //get a page by index
        virtual Page* GetPageByIndex( const int index ) = 0;

        //get the number of pages used by this screen
        virtual int GetNumberOfPages() const = 0;

        //screen scaling (for zooming in/out effects)
        virtual void SetScale( float screenScale ) = 0;
        virtual float GetScale() const = 0;

    protected:
    private:
};

} // End namespace

#endif // End conditional inclusion
