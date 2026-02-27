//===========================================================================
// Copyright (C) 2000 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   FeMovie
//
// Description: 
//
// Authors:     Darwin Chau
//
// Revisions        Date            Author        Revision
//                  2000/08/03        DChau        Created
//
//===========================================================================

#ifndef FEMOVIE_H
#define FEMOVIE_H

//===========================================================================
// Nested Includes
//===========================================================================
#include "Movie.h"
#include "FeBoundedDrawable.h"
#include "FeParent.h"

#include <p3d/p3dtypes.hpp>  // for tColour

//===========================================================================
// Forward References
//===========================================================================

//===========================================================================
// Interface Definitions
//===========================================================================
class FeMovie
 : 
    public FeBoundedDrawable,
    public FeParent,
    virtual public Scrooby::Movie
{
    public:

        FeMovie( const tName& name, int x, int y );
        virtual ~FeMovie();

        void AddMovieClip( unsigned int imageResourceID );
    
        virtual void SetAlpha( float alpha );
        virtual float GetAlpha() const;
        void SetAlphaNoRedraw( float alpha );

        virtual void SetColour( tColour colour );
        // Javascript testbed interface
        virtual void SetColour( int red, int green, int blue );

        virtual const tColour& GetColour() const;

        
        virtual void Display();
        virtual void Reset();

        //
        // Implementation of Scrooby interface.
        //
        virtual void Play();
        virtual void Pause();
        virtual void Stop();
        virtual int GetCurrentFrame() const;
        virtual void SetNextFrame( int frameIndex );


    private:

        //---------------------------------------------------------------------
        // Private Functions
        //---------------------------------------------------------------------

        // No copying or assignment. Declare but don't define.
        //
        FeMovie( const FeMovie& );
        FeMovie& operator= ( const FeMovie& );

        void SetColourNoRedraw( int red, int green, int blue );
        void SetColourNoRedraw( tColour colour );

        //---------------------------------------------------------------------
        // Private Data
        //---------------------------------------------------------------------
        float mAlpha;
        tColour mColour;

};

#endif

