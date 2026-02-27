//===========================================================================
// Copyright (C) 2001 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   Scrooby Runtime Public Interface
//
// Description: This contains the BoundedDrawable class.
//
// Authors:     Ian Gipson
//
//===========================================================================

#include "FeMovie.h"
#include <raddebug.hpp>

//===========================================================================
// FeMovie::FeMovie
//===========================================================================
// Description: Constructor.
//
// Constraints:    None.
//
// Parameters:    name - the name assinged to this object in the builder
//              x - x position (left)
//              y - y position (bottom)
//
// Return:      n/a
//
//===========================================================================
FeMovie::FeMovie
( 
    const tName& name,
    int x,
    int y
) 
: 
    FeBoundedDrawable( name ),
    FeParent()
{
    this->NormalizeCoord( x, y, mPosX, mPosY );
    
    Reset();
}



//===========================================================================
// FeMovie::~FeMovie()
//===========================================================================
// Description: Destructor.
//
// Constraints:    None.
//
// Parameters:    n/a
//
// Return:      n/a
//
//===========================================================================
FeMovie::~FeMovie()
{
}


//===========================================================================
// void FeMovie::Reset()
//===========================================================================
// Description: 
//
// Constraints:    
//
// Parameters:    
//
// Return:      
//
//===========================================================================
void FeMovie::Reset()
{
    this->FeBoundedDrawable::Reset();

    this->SetAlphaNoRedraw( 1.0f );
    
    this->SetColourNoRedraw( 255, 255, 255 );
}


//===========================================================================
// void FeMovie::Display()
//===========================================================================
// Description: 
//
// Constraints:    
//
// Parameters:    
//
// Return:      
//
//===========================================================================
void FeMovie::Display()
{
    //
    // STUB.
    //
}

//===========================================================================
// void FeMovie::AddMovieClip( unsigned int imageResourceID )
//===========================================================================
// Description: 
//
// Constraints:    
//
// Parameters:    
//
// Return:      
//
//===========================================================================
void FeMovie::AddMovieClip
( 
    unsigned int 
)
{
    //
    // STUB.
    //
    
//    FeResourceEntry* pResourceEntry = new FeResourceEntry( "", imageResourceID );
//  AddChild( pResourceEntry );
}


//===========================================================================
// void FeMovie::SetAlpha( float alpha )
//===========================================================================
// Description: 
//
// Constraints:    
//
// Parameters:    
//
// Return:      
//
//===========================================================================
void FeMovie::SetAlpha
(
    float alpha
)
{
    this->SetAlphaNoRedraw( alpha );
    
    this->Display();
}


//===========================================================================
// void FeMovie::SetAlphaNoRedraw( float alpha )
//===========================================================================
// Description: 
//
// Constraints:    
//
// Parameters:    
//
// Return:      
//
//===========================================================================
void FeMovie::SetAlphaNoRedraw
( 
    float alpha 
)
{
    rAssert( alpha >= 0 && alpha <= 1.0f );
    
    mAlpha = alpha;
    
    mColour.Set( mColour.Red(), 
                 mColour.Green(),
                 mColour.Blue(),
                 static_cast<int>( (alpha*255) ) );
}


//===========================================================================
// void FeMovie::SetColour( int red, int green, int blue )
//===========================================================================
// Description: 
//
// Constraints:    
//
// Parameters:    
//
// Return:      
//
//===========================================================================
void FeMovie::SetColour
(
    int red,
    int green,
    int blue
)
{
    rAssert( red >= 0 && red <= 255 );
    rAssert( green >= 0 && green <= 255 );
    rAssert( blue >= 0 && blue <= 255 );

    this->SetColourNoRedraw( red, green, blue );

    this->Display();
}


//===========================================================================
// void FeMovie::SetColour( tColour c )
//===========================================================================
// Description: 
//
// Constraints:    
//
// Parameters:    
//
// Return:      
//
//===========================================================================
void FeMovie::SetColour( tColour c )
{
    this->SetColourNoRedraw( c );
    
    this->Display();
}


//===========================================================================
// void FeMovie::SetColourNoRedraw( tColour c )
//===========================================================================
// Description: 
//
// Constraints:    
//
// Parameters:    
//
// Return:      
//
//===========================================================================
void FeMovie::SetColourNoRedraw( tColour c )
{
    SetColourNoRedraw( c.Red(), c.Green(), c.Blue() );
}


//===========================================================================
// void FeMovie::SetColourNoRedraw( int red, int green, int blue )
//===========================================================================
// Description: 
//
// Constraints:    
//
// Parameters:    
//
// Return:      
//
//===========================================================================
void FeMovie::SetColourNoRedraw
( 
    int red,
    int green,
    int blue 
)
{
    rAssert( red >= 0 && red <= 255 );
    rAssert( green >= 0 && green <= 255 );
    rAssert( blue >= 0 && blue <= 255 );

    mColour.Set( red, green, blue, mColour.Alpha() );
}


//===========================================================================
// float GetAlpha() const
//===========================================================================
// Description: get the alpha value for this object
//
// Constraints:    None
//
// Parameters:    None
//
// Return:      the alpha value for this object
//
//===========================================================================
float FeMovie::GetAlpha() const
{
    return( mAlpha );
}


//===========================================================================
// const tColour& FeMovie::GetColour() const
//===========================================================================
// Description: get the alpha value for this object
//
// Constraints:    None
//
// Parameters:    None
//
// Return:      the alpha value for this object
//
//===========================================================================
const tColour& FeMovie::GetColour() const
{
    return( mColour );
}


//===========================================================================
// void FeMovie::Play()
//===========================================================================
// Description: 
//
// Constraints:    
//
// Parameters:    
//
// Return:      
//
//===========================================================================
void FeMovie::Play()
{
    //
    // Stub.
    //
}


//===========================================================================
// void FeMovie::Pause()
//===========================================================================
// Description: 
//
// Constraints:    
//
// Parameters:    
//
// Return:      
//
//===========================================================================
void FeMovie::Pause()
{
    //
    // Stub.
    //
}


//===========================================================================
// void FeMovie::Stop()
//===========================================================================
// Description: 
//
// Constraints:    
//
// Parameters:    
//
// Return:      
//
//===========================================================================
void FeMovie::Stop()
{
    //
    // Stub.
    //
}


//===========================================================================
// int FeMovie::GetCurrentFrame()
//===========================================================================
// Description: 
//
// Constraints:    
//
// Parameters:    
//
// Return:      
//
//===========================================================================
int FeMovie::GetCurrentFrame() const
{
    //
    // Stub.
    //
    return( 0 );
}


//===========================================================================
// void FeMovie::SetNextFrame( int frameIndex )
//===========================================================================
// Description: 
//
// Constraints:    
//
// Parameters:    
//
// Return:      
//
//===========================================================================
void FeMovie::SetNextFrame
(
    int
)
{
    //
    // Stub.
    //
}
