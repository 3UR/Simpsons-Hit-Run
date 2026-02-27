//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        radmovie.hpp
//
// Subsystem:   Foundation Technologies - Movie Player Services
//
// Description: This file contains all definitions and interfaces required to
//              interact with the Foundation Technologies Movie Player Services.
//
//              The new movieplayer handles decoding of compressed video data
//              and synchronization of audio with video playback.  
//
//              The movie player internally uses radsound to playback audio.
//
//              The movie player decodes each frame of data into a decoded 
//              video frame buffer.  The client is responsible for rendering 
//              each frame to the screen as quickly as possible.
//
// Note:        Temporarily, rendering services are provided by utility classes,
//              which are found in the radmovie/util directory.  Soon, rendering
//              will be provided entirely by pure3D
//
// Revision:    Playstation implementation only.  June 15, 2002 RWS
// 
//=============================================================================

#ifndef RADMOVIE_HPP
#define RADMOVIE_HPP

//=============================================================================
// Include Files
//=============================================================================

#include <radoptions.hpp>
#include <radobject.hpp>
#include <radmemory.hpp>
#include <radsound_hal.hpp>

//=============================================================================
// Forward Declarations and typedefs
//=============================================================================

struct IRadMoviePlayer2;
struct IRadMovieRenderLoop;
struct IRadMovieRenderStrategy;

//=============================================================================
// Extern constants
//=============================================================================

//
// Load video only by setting the audio track
// index to this radMovie_NoAudioTrack
//

extern const unsigned int radMovie_NoAudioTrack;

//
// To view movie player debug printf's call ::radDebugChannelEnable( radMovieDebugChannel );
//

extern const char * radMovieDebugChannel2;

//=============================================================================
// Global Functions
//=============================================================================

//
// Initialization and termination of the movie player system must
// occur during the set up and tear down of the game
//

void radMovieInitialize2( radMemoryAllocator allocator );
void radMovieTerminate2( void );

//
// Service the radMovie as often as possible.  The assumption is that
// only activities related to movie playback will be serviced while
// a movie is playing.
//
// This service call should occur along with radFileService( ), 
// radSoundHalSystemGet( )->Service( ), radSoundHalSystemGet( )->ServiceOncePerFrame( ),
// and optionally radControllerSystemService( ).
//

void radMovieService2( void );

//=============================================================================
// Factories
//=============================================================================

IRadMoviePlayer2 * radMoviePlayerCreate2( radMemoryAllocator alloc );
IRadMovieRenderStrategy * radMovieSimpleFullScreenRenderStrategyCreate( radMemoryAllocator alloc );

//=============================================================================
// struct IRadMoviePlayer2
//=============================================================================

struct IRadMoviePlayer2 : public IRefCount
{
    struct VideoFrameInfo
    {
        unsigned int Width;
        unsigned int Height;
    };

    enum State
    {
        NoData,         // no movie file has been loaded.
        Loading,        // in the process of loading a movie file.
        LoadToPlay,     // in the process of loading; after it is loaded, it will start
                        // the playback.
        ReadyToPlay,    // the movie file has been loaded, but the movie playback is paused/stopped.
        Playing        // in the process of playing the movie file.
    };

    //
    // Initialization: Controls the allocation of memory for the movie player
    //
    //      * Output video buffer size: This is a function of the maximum dimension of the
    //                                  the buffer in pixels (maxWidthPixels x maxHeightPixels)
    //      * Encoded video buffer:     Specified in bytes.  The size will depend on compression
    //                                  and video frame size.  Big enough for 1 second of data
    //                                  seems to work.
    //      * Audio Primary Buffer:     This controls how often the audio component of the movie
    //                                  player reads from the disk.  (2 or 3 seconds)
    //      * Audio Secondary Buffer:   This can be quite small (you'll hear skips if it's too small)
    //

#if defined( RAD_WIN32 ) || defined( RAD_UWP )

    virtual void Initialize( 
        IRadMovieRenderLoop * pIRadMovieRenderLoop, 
        IRadMovieRenderStrategy * pIRadMovieRenderStrategy ) = 0;

    #endif

    //
    // The client will call render from within the IRadMovieRenderLoop::IterateLoop
    // function.  See IRadMovieRenderLoop for more details.
    //

    virtual bool Render( void ) = 0;
    
    // 
    // Loading movies: Provide the name of the movie to load.  Rmv movie
    // files may contain several audio tracks (.e.g for multiple languages).  
    // Specify the index of the audio track to load.
    //
    // If a movie file doesn't contain audio, or if you would like to 
    // play video only, set the audio track index to 'radMovie_NoAudioTrack'
    //
    // State transitions: 
    //
    //      Load:   NoData -> Loading -> ReadyToPlay
    //      Unload: * -> NoData
    //

    virtual void Load( const char * pFileName, unsigned int audioTrackIndex ) = 0;
    virtual void Unload( void ) = 0;

    //
    // Play( ) causes the movie player to playback audio and decode video frames.
    // 
    // State transitions:   ReadyToPlay -> Playing   or
    //                      Loading -> LoadingToPlay -> Playing
    //
    //                      When the player reaches the end of the file -> NoData
    //

    virtual void Play( void ) = 0;

    //
    // Pause( ) halts audio playback and video decoding.  During the Pause
    // state, no render will be be happening.
    //
    // State transitions:   Playing -> ReadyToPlay or
    //                      LoadToPlay -> Loading -> ReadyToPlay
    //

    virtual void Pause( void ) = 0;

    //
    // Audio control
    //

    virtual void  SetVolume( float volume ) = 0; // 0.0 to 1.0 ( min to max )
    virtual float GetVolume( void ) = 0;
    virtual void  SetPan( float pan ) = 0; // -1.0 to 1.0 ( left to right )
    virtual float GetPan( void ) = 0;

    //
    // State information.  Note that video frame info can only be obtained 
    // if a movie has been loaded.
    //

    virtual State GetState( void ) = 0;
    virtual bool GetVideoFrameInfo( VideoFrameInfo * pFrameInfo ) = 0;
    virtual float GetFrameRate( void ) = 0;
    virtual unsigned int GetCurrentFrameNumber( void ) = 0;
};

//=============================================================================
// struct IRadMovieRenderLoop
//=============================================================================

struct IRadMovieRenderLoop : public IRefCount
{
    //
    // IterateLoop is called by the movie player when it is time to render
    // the next frame.  The render loop object will begin the frame and end
    // the frame and must call pIRadMoviePlayer->Render( ) when it is okay
    // for the movie frame to be rendered to the screen.
    //

    virtual void IterateLoop( IRadMoviePlayer2 * pIRadMoviePlayer ) = 0;
};

//=============================================================================
// struct IRadMovieRenderStrategy
//=============================================================================

//
// The member functions of this interface used internally by radmovie.  
// You don't need to call any of them, the movie player itself will take
// care of that.
//

struct IRadMovieRenderStrategy : public IRefCount
{
    #if defined( RAD_WIN32 ) || defined( RAD_UWP )

        struct LockedDestination
        {
            // Info about where to copy data to...

            void * m_pDest;            // Destination buffer address
            int m_DestPitch;           // Pitch of the destination buffer in pixels
            unsigned int m_DestPosX;   // X offset into the dest buffer in pixels
            unsigned int m_DestPosY;   // Y offset into the dest buffer in piexls
            unsigned int m_SrcPosX;    // X offset into the src buffer in pixels
            unsigned int m_SrcPosY;    // Y offset into the src buffer in piexls
            unsigned int m_Width;      // The width in pixels to copy
            unsigned int m_Height;     // The height in pixels to copy
        };

        //
        // The render strategy implementation may spread video data across a 
        // number of buffers.  Clients iterate through all destinantions
        // when they wish to render a frame.  ResetDesination must be called 
        // for each new frame.  LockNextDestination returns the number of 
        // destinations remaining.  For a complete frame to be rendered,
        // LockNextDestination followed by UnlockDestination must be called
        // until the return value of LockNextDestination returns 0.
        //

        virtual void ResetDestinations( void ) = 0;
        virtual unsigned int LockNextDestination( LockedDestination * pLockedDestination ) = 0;
        virtual void UnlockDestination( void ) = 0;

        // ChangeParameters may cause textures and other buffers to be re-allocated

        virtual void ChangeParameters( unsigned int width, unsigned int height ) = 0;
        virtual bool Render( void ) = 0;

    #else

        #if defined( RAD_WIN32 ) || defined( RAD_UWP )

        virtual void SetParameters( unsigned int width, unsigned int height ) = 0;
        virtual bool Render( void * pBuffers[], int strides[] ) = 0;

        #else

        virtual void SetParameters( unsigned int width, unsigned int height ) = 0;
        virtual bool Render( void * pBuffer ) = 0;

        #endif 

    #endif
};

#endif // RADMOVIE_HPP
