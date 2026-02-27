//===========================================================================
// Copyright (C) 2000 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   Scrooby Runtime Public Interface
//
// Description: This contains the Scrooby::Movie class.
//
// Authors:     Darwin Chau
//
//===========================================================================

// Recompilation protection flag.
#ifndef SCROOBY_MOVIE_H
#define SCROOBY_MOVIE_H

//===========================================================================
// Nested Includes
//===========================================================================
#include "BoundedDrawable.h"

namespace Scrooby
{
//===========================================================================
// Interface Definitions
//===========================================================================

//===========================================================================
//
// Description: This class is used to represent a movie drawing element.
//
// Constraints: None
//
//===========================================================================

class Movie 
: 
    virtual public BoundedDrawable
{
    public:

        virtual void Play() = 0;
        virtual void Pause() = 0;
        
        virtual void Stop() = 0;
        
        virtual int GetCurrentFrame() const = 0;
        virtual void SetNextFrame( int frameIndex ) = 0;
    
    protected:
    private:
};


} // End namespace

#endif // End conditional inclusion
