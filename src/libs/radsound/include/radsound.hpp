//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//============================================================================
// File: radsound.hpp
//
// Description: This file contains the complete public interface for
//              interacting with the radsound system. 
//============================================================================

#ifndef RADSOUND_HPP
#define RADSOUND_HPP

typedef void RadSoundFilePerformanceCallback( bool start, const char * pFile, unsigned int bytes );
void RadSoundSetFilePerformanceCallback( RadSoundFilePerformanceCallback * pCallback );

//============================================================================
// Include Files
//============================================================================

#include <radmemory.hpp>
#include <radfile.hpp>
#include <radsound_hal.hpp>

//============================================================================
// Forward Declarations
//============================================================================

struct IRadSoundClip;
struct IRadSoundClipPlayer;
struct IRadSoundStreamPlayer;
struct IRadSoundRsdFileDataSource;
struct IRadSoundBufferedDataSource;
struct IRadSoundStitchedDataSource;
struct IRadSoundStitchCallback;
struct IRadSoundMemoryDataSource;
struct IRadSoundMemorySpaceObject;
struct IRadSoundDataCache;
struct IRadSoundAdpcmDecodeStream;

//============================================================================
// Factories and static functions
//============================================================================

IRadSoundClip               * radSoundClipCreate(                 radMemoryAllocator = RADMEMORY_ALLOC_DEFAULT );
IRadSoundClipPlayer         * radSoundClipPlayerCreate(           radMemoryAllocator = RADMEMORY_ALLOC_DEFAULT );
IRadSoundStreamPlayer       * radSoundStreamPlayerCreate(         radMemoryAllocator = RADMEMORY_ALLOC_DEFAULT );
IRadSoundRsdFileDataSource  * radSoundRsdFileDataSourceCreate(    radMemoryAllocator = RADMEMORY_ALLOC_DEFAULT );
IRadSoundBufferedDataSource * radSoundBufferedDataSourceCreate(   radMemoryAllocator = RADMEMORY_ALLOC_DEFAULT );
IRadSoundStitchedDataSource * radSoundStitchedDataSourceCreate(   radMemoryAllocator = RADMEMORY_ALLOC_DEFAULT );
IRadSoundMemoryDataSource   * radSoundMemoryDataSourceCreate(     radMemoryAllocator = RADMEMORY_ALLOC_DEFAULT );
IRadSoundMemorySpaceObject  * radSoundMemorySpaceObjectCreate(    radMemoryAllocator = RADMEMORY_ALLOC_DEFAULT );
IRadSoundDataCache          * radSoundDataCacheCreate(            radMemoryAllocator = RADMEMORY_ALLOC_DEFAULT );
IRadSoundAdpcmDecodeStream  * radSoundAdpcmDecodeStreamCreate(    radMemoryAllocator = RADMEMORY_ALLOC_DEFAULT );

//============================================================================
// Types
//============================================================================

// This function typedef is used for trapping state changes from various
// sound components.

extern const char * radSoundDebugChannel;

//============================================================================
// Interface: IRadSoundClip
//
// Description:  The clip represents playable sound which is completely loaded
//               in memory. A clip can be played by multiple clip players.
//============================================================================

struct IRadSoundClip : public IRefCount
{
    // You specify the data source that supplies the data to load the clip
    // in this function.  This data source must not be of unknown length.
    // (An unknown length data source chain will return -1 when you call
    // GetRemainingFrames( ) )
     
    virtual void Initialize(
        IRadSoundHalDataSource * pIRshds,
        IRadSoundHalMemoryRegion * pIRshmr,
        bool looping,
        const char * pInstanceName // Used to track memory usage for memory monitor and out of memory dumps
    ) = 0;

    virtual IRadSoundHalMemoryRegion * GetMemoryRegion( void ) = 0;
    virtual bool GetLooping( void ) = 0;

    virtual IRadSoundHalAudioFormat * GetFormat( void ) = 0;
     
    virtual void  SetTrim( float trim ) = 0;
    virtual float GetTrim( void ) = 0;   
    
    enum State { NoFile, Initializing, Initialized };

    virtual State GetState( void ) = 0;
    
    virtual bool WillRelease( void ) = 0;    
};

//============================================================================
// Interface: IRadSoundPlayer
//
// Description: Base interface for StreamPlayer and ClipPlayer.
//============================================================================

struct IRadSoundPlayer : public IRefCount
{
    virtual void  Play( void ) = 0;
    virtual void  Stop( void ) = 0;
    virtual bool  IsPlaying( void ) = 0;

    // Priority is 0 to 10 where 10 is maximum.

    virtual void SetPriority( unsigned int priority ) = 0; 
    virtual unsigned int  GetPriority( void ) = 0;

    virtual void  SetMuted( bool ) = 0;
    virtual bool  GetMuted( void ) = 0;
    virtual void  SetVolume( float ) = 0;
    virtual float GetVolume( void ) = 0;
    virtual void  SetTrim( float ) = 0;
    virtual float GetTrim( void ) = 0; 
    virtual void  SetPitch( float ) = 0;
    virtual float GetPitch( void ) = 0;
    
    virtual void  SetPan( float ) = 0;
    virtual float GetPan( void ) = 0;

    // Players with assigned positional groups cannot call
    // the following aux functions.  

    virtual void  SetAuxMode( unsigned int aux, radSoundAuxMode mode ) = 0;
    virtual radSoundAuxMode GetAuxMode( unsigned int aux ) = 0;

    virtual void  SetAuxGain( unsigned int aux, float gain ) = 0;
    virtual float GetAuxGain( unsigned int aux ) = 0;

    // You now specify the positional group for the player.

    virtual void SetPositionalGroup( IRadSoundHalPositionalGroup * pIRshpg ) = 0;
    virtual IRadSoundHalPositionalGroup * GetPositionalGroup( void ) = 0;
    
    virtual unsigned int GetPlaybackTimeInSamples( void ) = 0;   
};


//============================================================================
// Interface: IRadSoundClipPlayer
//
// Description:  Multiple clip players can play a single clip.
//============================================================================

struct IRadSoundClipPlayer : public IRadSoundPlayer
{
    virtual void  SetClip( IRadSoundClip * pIRadSoundClip ) = 0;
    virtual IRadSoundClip * GetClip( void ) = 0;

    enum State { NoClip, Queueing, QueuedPlay, Playing, Stopped };

    virtual State GetState( void ) = 0;    

    virtual void SetPlaybackPosition(
        unsigned int position, IRadSoundHalAudioFormat::SizeType st ) = 0;
};

//============================================================================
// Interface: IRadSoundStreamPlayer
//
// Description: A stream player plays a data stream from disk, network,
//      memory, or any other IRadSoundDataSource derived stream.  A stream
//      player has no IRadStream counterpart as in the clip/clip player
//      relationship because only once instance can be played at
//      a time.
//============================================================================

struct IRadSoundStreamPlayer : public IRadSoundPlayer
{
    enum State { NoSource, Queueing, QueuedPlay, Playing, Paused };

    // Reserves memory right away, you must know the format of the audio

    virtual void Initialize(
        IRadSoundHalAudioFormat * pIRadSoundHalAudioFormat,
        unsigned int size, IRadSoundHalAudioFormat::SizeType,
        IRadSoundHalMemoryRegion * pIRadSoundHalMemoryRegion,
        const char * pIdentifier ) = 0;

    // If you don't know the audio format you can initialize the stream player
    // asyncronously. The memory will be allocated after the stream is
    // initialized (once the data source chain has been initialized).

    virtual void InitializeAsync(
        unsigned int size, IRadSoundHalAudioFormat::SizeType sizeType,
        IRadSoundHalMemoryRegion * pIRadSoundHalMemoryRegion,
        const char * pIdentifier ) = 0;

    virtual void SetLowWaterMark( float lowWaterMark ) = 0;
    virtual float GetLowWaterMark( void ) = 0;

    virtual IRadSoundHalAudioFormat * GetFormat( void ) = 0;
   
    // The data source to stream from, you can changes this at any
    // time to re-use the streamer object.

    virtual void SetDataSource( IRadSoundHalDataSource * pIRadSoundHalDataSource ) = 0;
    virtual IRadSoundHalDataSource * GetDataSource( void ) = 0;

    // For getting or receiving state notifications from this streamer.

    virtual State GetState( ) = 0;          
};

//============================================================================ 
// D A T A    S O U R C E    I N T E R F A C E S
//============================================================================
// The input data system has been redesigned with versatility and modularity
// in mind.  The input source to the clip/stream player and ultimately the
// buffer object can now be made up of a chain of data sources.  For example,
// if you wan't to stitch seemlessly you can set up your objects like so:
//
// radFile1 --> fileDs \
// radFile2 --> fileDs --> StitchedDs --> BufferedDs --> StreamPlayer
// radFile3 --> fileDs /
//
// Here we are wrapping .rsd files to stitch with the rsdFileDataSource
// component and feeding them into the stitched data source.  The output
// of the stitched data source is buffered (to compensate for file access
// and frame rate latencies ) and the buffered output is fed to the streamer.
//
// Note that the data-source model is actually a "PULL" model where the
// stream player calls back the buffered data source when it needs data, and
// the buffered data source calls back the stitcher when it needs to
// replenish its buffer, etc.
//
//============================================================================

//============================================================================
// Interface: IRadSoundStitchedDataSource
//
// Description: This data source allows you to seemlessly stitch
//      together pieces of audio data.
//
//  You can initialize it in two ways.
//      
//      1) From Audio Format - The stitched data source goes into the
//         initialized state immediately.  This method is cleaner because
//         your stitch handler class can handle all stitches in OnStitch( ).
//
//      2) From Data Source  - The stitched data source becomes initialized
//         when the initial input data source becomes initialized.  Thus,
//         you need not explicitly know the data format.  With this model,
//         you must supply the initial data source, then handle all subsequent
//         stitches in OnStitch( ).
//============================================================================

struct IRadSoundStitchedDataSource : public IRadSoundHalDataSource
{
    virtual void InitializeFromAudioFormat(
        IRadSoundHalAudioFormat * pIRshaf ) = 0;

    virtual void InitializeFromDataSource( IRadSoundHalDataSource *
        pIRadSoundHalDataSource ) = 0;

    // Make sure you set this to NULL before you release the
    // stiched data source.

    virtual void SetStitchCallback( IRadSoundStitchCallback * pStitchCallback,
        void * pUserData ) = 0;

    virtual void Reset( void ) = 0; // Clear current data source + counter
    virtual void ResetAudioFormat( IRadSoundHalAudioFormat * ) = 0; // GCN ADPCM hack
};

//============================================================================
// Interface: IRadSoundStitchCallback
//
// Description: Stitching source will call-back via this interface when it
//      needs more data to stitch.
//============================================================================

struct IRadSoundStitchCallback
{
    // Return a data source when the stitcher calls you back.
    //
    // Note: YOU must AddRef the new data source in compliance with radical
    // reference-counding standards.
    //
    // You should avoid performing lengthy processing or calling back into
    // the streamer in this function.

    virtual void OnStitch( IRadSoundHalDataSource ** pIRsds,
        unsigned int frameCount, void * pUserData ) = 0;
};

//============================================================================
// Interface: IRadSoundBufferedDataSource
//
// Description: A data source buffer, you can chain this in at any point
//      in the data-source stream.  You may re-use the buffered data source
//      (and thus its buffer memory) as long as the data is in the same format
//      as the previous input data source.
//
//      The buffer can be located in any memory space on the platform.
//============================================================================

struct IRadSoundBufferedDataSource : public IRadSoundHalDataSource
{
    virtual void Initialize(        
        radMemorySpace bufferMemorySpace,
        IRadMemoryAllocator * pIRadMemoryAllocator,
        unsigned int size,
        IRadSoundHalAudioFormat::SizeType,
        IRadSoundHalAudioFormat * pIRshaf,
        const char * pIdentifier ) = 0;

    virtual void SetInputDataSource( IRadSoundHalDataSource * pIRshds ) = 0;
    virtual IRadSoundHalDataSource * GetInputDataSource( void ) = 0;
    
    // Low water mark controls how little the fresh data the data source
    // will contain before it will refill itself.  The value a percentage
    // of the buffersize and lies between 0 and 1.

    virtual void SetLowWaterMark( float lowWaterMark ) = 0;
    virtual float GetLowWaterMark( void ) = 0;
    
    virtual bool IsBufferFull( void ) = 0;

};

//============================================================================
// Interface: IRadSoundRsdFileDataSource
//
// Description: An UNBUFFERED data source that reads from a file.  Chain in a
//      buffered data source after the file data source to do custom
//      buffering to compensate for file latencies.
//============================================================================

struct IRadSoundRsdFileDataSource : public IRadSoundHalDataSource
{
    // If you know the audio format, then the header is not read this causes
    // the file system to do 1 less seek per file.

    virtual void InitializeFromFile( 
        IRadFile * pIRadFile,
        unsigned int initialPlaybackPosition,
        IRadSoundHalAudioFormat::SizeType positionType,
        IRadSoundHalAudioFormat * pIRadSoundHalAudioFormat ) = 0;

    virtual void InitializeFromFileName( 
        const char * pFieName,
        bool highPriority,
        unsigned int initialPlaybackPosition,
        IRadSoundHalAudioFormat::SizeType positionType,
        IRadSoundHalAudioFormat * pIRadSoundHalAudioFormat ) = 0;
};

//============================================================================
// Interface: IRadSoundMemorySpaceObject
//
// Description: A reference counted piece of memory that knows how to 
//      free itself when the ref count goes to zero.  This is used for caching
//      of sound data for the IRadSoundMemoryDataSource and IRadSoundDataCache
//      components (below).
//
//      This object incapsulates allocating aligned memory on any platform
//      memory space, size information, and executing the correct free()
//      function. 
//============================================================================

struct IRadSoundMemorySpaceObject : public IRefCount
{    
    virtual void Initialize(
        radMemorySpace space,
        IRadMemoryAllocator * pIRadMemoryAllocator,
        unsigned int numberOfBytes,
        unsigned int alignment,
        const char * pIdentifier ) = 0;

    virtual void *                GetAddress( void ) = 0;
    virtual IRadMemoryAllocator * GetAllocator( void ) = 0;
    virtual radMemorySpace        GetMemorySpace( void ) = 0;
    virtual unsigned int          GetSizeInBytes( void ) = 0;
    virtual const char *          GetIdentifier( void ) = 0; 
};

//============================================================================
// Interface: IRadSoundMemoryDataSource
//
// Description: A data source that reads from memory.  This memory must be
//      wrapped by an IRadSoundMemorySpaceObject (above). 
//============================================================================

struct IRadSoundMemoryDataSource : public IRadSoundHalDataSource
{
    virtual void Initialize(
        IRadSoundHalAudioFormat * pIRshaf,    // Format of the audio
        IRadSoundMemorySpaceObject * pIRsmso, // The memory containing the data
        unsigned int startOffset,             // Where to start playing the data
        unsigned int size,                    // How much data to play     
        IRadSoundHalAudioFormat::SizeType st) = 0; // Units for prev 2 params.
};

//============================================================================
// Interface: IRadSoundDataCache
//
// Description: A cache for sound data that is not directly playable by the
//      hardware.
// 
//============================================================================

struct IRadSoundDataCache : public IRefCount
{
    enum State { Idle, Initializing, Initialized, Error };

    // You initialize the data cache with the memory space to allocate the
    // memory and the data source to read the data from.  

    virtual void InitializeAsync(
        radMemorySpace memorySpace,
        IRadMemoryAllocator * pIRadMemoryAllocator,
        IRadSoundHalDataSource * pIRshds,
        const char * pIdentifier ) = 0; // For tracking memory

    virtual State GetState( void ) = 0;

    // When you are ready to use the cached data call CreateDataSource( )
    // and pass it into a clip or stream player.  You CAN call this function
    // before the data source has been initialized, or after Initialize( )
    // has been called but before finished initializing.  The state of the
    // data source returned from this function reflects the state of the
    // data cache.

    virtual IRadSoundHalDataSource * CreateDataSource(
        unsigned int startOnFrame,
        radMemoryAllocator a ) = 0;
        
    virtual bool WillRelease( void ) = 0;
};

struct IRadSoundAdpcmDecodeStream : public IRadSoundHalDataSource
{
    virtual void Initialize( IRadSoundHalDataSource* ) = 0;  
};

#endif // RADSOUND_HPP