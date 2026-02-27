//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        radmovieinterfaces.hpp
//
// Subsystem:	Foundation Technologies - Movie Player
//
// Description:	This file contains the declaration for the Foundation Tech 
//              Movie Player
//
// Date:    	May 16, 2002
//
//=============================================================================

#ifndef RADMOVIEINTERFACES_HPP
#define RADMOVIEINTERFACES_HPP

//=============================================================================
// Include Files
//=============================================================================

#include <radobject.hpp>
#include <radfile.hpp>

//=============================================================================
// Forward Class/Struct Declarations
//=============================================================================

struct IRadMovieVideoDecoder;
struct radMovieVideoFrame;

//=============================================================================
// Global Functions
//=============================================================================

// Platform dependent initialization and termination

void radMovieVideoDecoderInitialize( radMemoryAllocator allocator );
void radMovieVideoDecoderTerminate( void );

// Creation factory for the platform dependent video decoder

IRadMovieVideoDecoder * radMovieVideoDecoderCreate( radMemoryAllocator allocator );

//=============================================================================
// Interface IRadMovieVideoDecoder
//=============================================================================

//
// The IRadMovieVideoDecoder handles the decoding of video,
// which is a platform dependent operation
//

struct IRadMovieVideoDecoder : IRefCount
{
    enum State { NoData, DataPending, Decoding, Ready, Error };
    
    //
    // Initialize( ) will allocate the buffers required for
    // decoding and storing the video data.  The size of the output 
    // video frame is based on its max height and width in pixels.
    // The size of the code data Buffer is specified in bytes.
    //

    virtual void Initialize( 
        unsigned int maxWidthPixels, 
        unsigned int maxHeightPixels, 
        unsigned int codedBufferBytes ) = 0;

    virtual void SetSource( IRadFile * pIRadFile, unsigned int widthPixels, unsigned int heightPixels, unsigned int startPosition, unsigned int frameRate ) = 0;
    virtual void Reset( void ) = 0;

    virtual void * GetLockedVideoFrame( unsigned int * pPresentationTime ) = 0;
    virtual void UnlockVideoFrame( void ) = 0;

    virtual State GetState( void ) = 0;
    virtual float GetFrameRate( void ) = 0;
    virtual unsigned int GetCurrentFrameNumber( void ) = 0;
    virtual void Service( void ) = 0;
};

//=============================================================================
// Interface IRadMovieVideoDecoder
//=============================================================================

struct radMovieVideoFrame : radObject
{
    IMPLEMENT_BASEOBJECT( "radMovieVideoFrame" )
};

#endif // RADMOVIEINTERFACES_HPP