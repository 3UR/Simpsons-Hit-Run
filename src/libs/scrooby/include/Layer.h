//===========================================================================
// Copyright (C) 2000 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   Scrooby Runtime Public Interface
//
// Description: This contains the Scrooby::Layer class.
//
// Authors:     Ian Gipson
//
//===========================================================================

// Recompilation protection flag.
#ifndef SCROOBY_LAYER_H
#define SCROOBY_LAYER_H

//===========================================================================
// Nested Includes
//===========================================================================
#include "Drawable.h"
#include "owner.h"

namespace Scrooby
{
//===========================================================================
// Interface Definitions
//===========================================================================

//===========================================================================
//
// Description: This class is used to represent a Scrooby Layer element
//
// Constraints: None
//
//===========================================================================

class Layer 
: 
    virtual public Drawable,
    virtual public Owner
{
    public:
    protected:
    private:
};

} // End namespace

#endif // End conditional inclusion