//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//============================================================================
// File: radsound_hal.hpp
//
// Description: This file contains the complete public interface for
//              interacting with the hardware abstracton layer of the
//              radsound system.
//============================================================================

#ifndef RADSOUND_HAL
#define RADSOUND_HAL

//============================================================================
// Include Files
//============================================================================

#include <radfile.hpp>
#include <radtime.hpp>
#include <radobject.hpp>
#include <radsoundmath.hpp>

//============================================================================
// Forward Declarations
//============================================================================

// The hal system implements these interfaces.

struct IRadSoundHalVoice;
struct IRadSoundHalBuffer;
struct IRadSoundHalSystem;
struct IRadSoundHalListener;
struct IRadSoundHalPositionalGroup;
struct IRadSoundHalMemoryRegion;
struct IRadSoundHalAudioFormat;
struct IRadSoundHalDataSource;
struct IRadSoundHalEffect;

// YOU derive from these interface to handle callbacks from the components
// above.

struct IRadSoundHalBufferClearCallback;
struct IRadSoundHalDataSourceCallback;
struct IRadSoundHalBufferLoadCallback;

//============================================================================
// Types
//============================================================================

// The auxillary mode affects the way voice output is sent to an Fx send.

enum radSoundAuxMode
{
	radSoundAuxMode_Off,        // No signal to effects send
	radSoundAuxMode_PreFader,   // signal after trim, but before volume
	radSoundAuxMode_PostFader,  // signal after trim, and after volume
	radSoundAuxMode_PostFaderDirect // Ps2 optomization, aux volume is disabled
                                    // and one less hw voice is used for sound.
};

// The global mode of the sound system.  XBox TRC says that this must be set
// only from the dashboard, so this setting is ignored.  Windows DirectSound
// doesn't allow control of this so it is igored.  Ps2 doesn't support surround
// so _Surround is equivalent to _Stereo.  Game cube supports all modes.

enum radSoundOutputMode
{
	radSoundOutputMode_Mono,    // radsound mixes to mono (PS2 only)
	radSoundOutputMode_Stereo,  // radsound mixes to stereo (PS2 only)
	radSoundOutputMode_Surround
};

//============================================================================
// Functions and Factories
//============================================================================

void radSoundHalSystemInitialize( radMemoryAllocator allocator );
void radSoundHalSystemTerminate( void );

IRadSoundHalSystem			* radSoundHalSystemGet( void );
IRadSoundHalListener		* radSoundHalListenerGet( void );
IRadSoundHalVoice			* radSoundHalVoiceCreate( radMemoryAllocator allocator );
IRadSoundHalBuffer			* radSoundHalBufferCreate( radMemoryAllocator allocator );
IRadSoundHalAudioFormat		* radSoundHalAudioFormatCreate( radMemoryAllocator allocator );
IRadSoundHalPositionalGroup * radSoundHalPositionalGroupCreate( radMemoryAllocator allocator );

unsigned int radSoundHalDataSourceReadAlignmentGet( );
unsigned int radSoundHalDataSourceReadMultipleGet( );

//=============================================================================
// Component: IRadSoundHalAudioFormat
//
// Description: Represents the "type" of audio data.  Stereo ADPCM at 41000KHz,
//		6 Channel PCM at 22050KHz, etc.  This class also houses the simple
//		helper functions for converting bytes/samples/frames.
//
//		This class was previously known as the radsoundheader class which has
//		been decoupled from .rsd files.
//=============================================================================

#define PCM_SILENCE_16_BIT ((short)0)
#define PCM_SILENCE_8_BIT ((unsigned char)128)

struct IRadSoundHalAudioFormat : public IRefCount
{
	enum Encoding
	{
		PCM,
		VAG,
		PCM_BIGENDIAN,
		XBOXADPCM,
		GCNADPCM,
		RadicalAdpcm
	};

	enum SizeType { Milliseconds, Frames, Samples, Bytes };

	virtual void Initialize(
		Encoding encoding,
        IRefCount * pIRefCount_CustomInfo,
		unsigned int samplingRate,
		unsigned int channels,
		unsigned int bitResolution ) = 0;

	virtual Encoding     GetEncoding( void ) = 0;
	virtual IRefCount *  GetCustomEncodingInfo( void ) = 0;
	virtual unsigned int GetNumberOfChannels( void ) = 0;
	virtual unsigned int GetFrameSizeInBytes( void ) = 0;
	virtual unsigned int GetFrameSizeInSamples( void ) = 0;
	virtual unsigned int GetSampleRate( void ) = 0;
	virtual unsigned int GetSampleSizeInBits( void ) = 0; // stereo matters here

	virtual bool Matches( IRadSoundHalAudioFormat * pOther ) = 0;

	// Conversion functions

	virtual unsigned int SamplesToBytes( unsigned int samples ) = 0;
	virtual unsigned int BytesToSamples( unsigned int bytes ) = 0;
	virtual unsigned int SamplesToFrames( unsigned int samples ) = 0;
	virtual unsigned int FramesToSamples( unsigned int frames ) = 0;
	virtual unsigned int FramesToBytes( unsigned int frames ) = 0;
	virtual unsigned int BytesToFrames( unsigned int bytes ) = 0;
    virtual unsigned int SamplesToMilliseconds( unsigned int samples ) = 0;
    virtual unsigned int MillisecondsToSamples( unsigned int milliseconds ) = 0;
    virtual unsigned int BytesToMilliseconds( unsigned int bytes ) = 0;
    virtual unsigned int MillisecondsToBytes( unsigned int samples ) = 0;
    virtual unsigned int FramesToMilliseconds( unsigned int frames ) = 0;
    virtual unsigned int MillisecondsToFrames( unsigned int milliseconds ) = 0;

	virtual unsigned int ConvertSizeType(
		SizeType target, unsigned int size, SizeType source ) = 0;

	virtual unsigned int GetBitResolution( void ) = 0;
};

//======================================================================
// Interface: IRadSoundHalDataSourceCallback
//
// Description:	Derive from this interface to receive notification
//		from the data source that the outstanding Async load is
//      complete.
//======================================================================

struct IRadSoundHalDataSourceCallback : public IRefCount
{
	virtual void OnDataSourceFramesLoaded( unsigned int framesActuallyRead ) = 0;
};

//======================================================================
// Interface: IRadSoundDataSource
//
// Description: All data fed to buffers must come from a component
//		that supports this interface.  You can only issue one
//      GetFramesAsync( ) request at a time.
//
//      Generally, there are two types of data sources.  Finite length
//      and unknown length.  An unknown length data source such as a
//      network radio stream or interactive music stream will report
//      -1 (0xFFFFFFFF) when GetRemainingFrames( ) is called.  In this
//      case the end of stream (EOS) is signalled when the data source
//      returns less frames then it was asked for.  Note that it is an
//      error to ask for more frames than are available from a finite
//      length data source.
//
//      Generally speaking, users of radsound will not implement this
//      interface directly--it is implemented by other components of
//      radsound.
//======================================================================

struct IRadSoundHalDataSource : public IRefCount
{
	// This function Allows us to hide async activity.
	
	enum State { Initializing, Initialized, Error };

	virtual State GetState( void ) = 0;

	virtual IRadSoundHalAudioFormat * GetFormat( void ) = 0;

	// (0xFFFFFFFF) signifies infinitely long or unknown length.

	virtual unsigned int GetRemainingFrames( void ) = 0;

    // Only one outstanding request is allowed.

	virtual void GetFramesAsync( 
		void * pBytes, 
		radMemorySpace destinationMemorySpace, 
		unsigned int numberOfFrames,
		IRadSoundHalDataSourceCallback * pCallback ) = 0;
		    
    virtual const char * GetName( void ) = 0;
    
    virtual unsigned int GetAvailableFrames( void ) = 0;
};

//============================================================================
// Interface: IRadSoundHalBufferLoadCallback
//
// Description:  Derive from this interface to receive notification from
//		a buffer that the current outstanding async load is completed.
//      'dataSourceFrames' reports the number of frames provided by
//      the datasource.
//============================================================================

struct IRadSoundHalBufferLoadCallback :	public IRefCount
{
	virtual void OnBufferLoadComplete( unsigned int dataSourceFrames ) = 0;
};

//============================================================================
// Interface: IRadSoundHalBufferClearCallback
//
// Description: Derive from this interface to receive notification from a
//		a buffer that the current oustanding async clear is completed.
//============================================================================

struct IRadSoundHalBufferClearCallback : public IRefCount
{
    virtual void OnBufferClearComplete( void ) = 0;
};

//======================================================================
// Interface: IRadSoundHalBuffer
//
// Description: All data is ultimately loaded into a buffer object then
//		played back with one or more voices.  The Buffer object also
//      abstracts loading and clearing data, DMA, mono vs interleaved
//      hardware playback, etc.
//======================================================================

struct IRadSoundHalBuffer : public IRefCount
{
    virtual void Initialize(
		IRadSoundHalAudioFormat * pIRadSoundHalAudioFormat,
        IRadMemoryObject * pIRadMemoryObject,
        unsigned int sizeInFrames, // For Re-using a memory object for a smaller buffer
		bool looping,
        bool streaming ) = 0;
    
	virtual IRadSoundHalAudioFormat * GetFormat( void ) = 0;
	virtual IRadMemoryObject * GetMemoryObject( void ) = 0;
	virtual bool IsLooping( void ) = 0;

	// SizeInSamples( ) is now size in frames.  Frames is smallest
    // manipulatable unit of data.

	virtual unsigned int GetSizeInFrames( void ) = 0;

	// Loads and clears are now specified in terms of frames, not samples.
	// A frame is the smallest logical unit of data.
    // If the datasource provides less frames than were reqested, the
    // buffer will fill the remaining frames with silence and still load
    // the entire request.  In the callback will report the number of frames
    // obtained from the datasource, however it is safe to assume that the
    // entire load request was satisfied.
		
	virtual void LoadAsync(
		IRadSoundHalDataSource * pIRadSoundHalDataSource,
		unsigned int startPositionInFrames,
		unsigned int numberOfFrames,
		IRadSoundHalBufferLoadCallback * pIRadSoundHalBufferLoadCallback ) = 0;

    virtual void ClearAsync( 
        unsigned int startPositionInFrames,
		unsigned int numberOfFrames,
        IRadSoundHalBufferClearCallback * pIRadSoundHalBufferClearCallback ) = 0;

    //
    // The number of frames to be transfered by Load or Clear operations
    // must be a multiple of the minimum transfer size
    //

    virtual unsigned int GetMinTransferSize( IRadSoundHalAudioFormat::SizeType sizeType ) = 0;

    // This will cancel all outstaning load/clear operations.

    virtual void CancelAsyncOperations( void ) = 0;

    //
    // The audio formats must be the same except for the custom encoding info.
    //
    virtual void ReSetAudioFormat( IRadSoundHalAudioFormat * pIRadSoundHalAudioFormat ) = 0;
};

// Helper function for initialization of IRadSoundHalBuffer.  Rounds requested 
// buffer size, taking audio format and optimal dma transfer multiples into account.
// Will calculate the required IRadMemoryObject size.

unsigned int radSoundHalBufferCalculateMemorySize( 
    IRadSoundHalAudioFormat::SizeType resultType,
    unsigned int requestedSize, 
    IRadSoundHalAudioFormat::SizeType requestedSizeType, 
    IRadSoundHalAudioFormat * pIRadSoundHalAudioFormat );

//======================================================================
// Interface: IRadSoundHalVoice
//
// Description: A voice plays back buffers.  One or more voices can
//		play back a single buffer.  Actual hardware resources are
//      consumed only when the voice is actually playing.
//======================================================================

struct IRadSoundHalVoice : public IRefCount
{
    // Priority is a number from zero to 10, where 10 is the highest
    // prioirty.  If the system run's out of voices, voices of lowere
    // priority will be stopped and given to the higher priority voices.

	virtual void SetPriority( unsigned int priority ) = 0;
    virtual unsigned int GetPriority( void ) = 0;
        
    virtual void SetBuffer( IRadSoundHalBuffer * pIRadSoundHalBuffer ) = 0;
    virtual IRadSoundHalBuffer * GetBuffer( void ) = 0;

    virtual void Play( void ) = 0;
    virtual void Stop( void ) = 0;
    virtual bool IsPlaying( void ) = 0;

    virtual void SetPlaybackPositionInSamples( unsigned int position ) = 0;
    virtual unsigned int GetPlaybackPositionInSamples( void ) = 0;

    virtual void SetVolume( float volume ) = 0;
    virtual float GetVolume( void ) = 0;
	    
    virtual void SetTrim( float trim ) = 0;
    virtual float GetTrim( void ) = 0;
	    
    virtual void SetMuted( bool muteOn ) = 0;
    virtual bool GetMuted( void ) = 0;

	virtual void  SetPan( float pan ) = 0;
	virtual float GetPan( void ) = 0;

    virtual void SetPitch( float pitch ) = 0;
    virtual float GetPitch( void ) = 0;
	    
    // This is the Fx send mode (see radSoundAuxMode above)
    virtual void SetAuxMode( unsigned int aux, radSoundAuxMode mode ) = 0;
    virtual radSoundAuxMode GetAuxMode( unsigned int aux ) = 0;

    // How much signal to send to the Fx send.  Global effects sends
    // are set up through the IRadSoundHalSystem component.

    virtual void  SetAuxGain( unsigned int aux, float gain ) = 0;
    virtual float GetAuxGain( unsigned int aux ) = 0; 

	// Set the positional group, there is no longer individual SetPosition( ),
	// SetVelocity( ), etc. parameters.
    // NOTE: Positional voices lose control of their aux sends (the environmental
    // effects system gains control)

	virtual void SetPositionalGroup( IRadSoundHalPositionalGroup * pIRshpg ) = 0;
	virtual IRadSoundHalPositionalGroup * GetPositionalGroup( void ) = 0;
};

//============================================================================
// Interface: IRadSoundListener
//
// Description:  The listener hears positional groups and defines its 
//               environment in terms of an effect to apply to what it
//               hears from positional groups.
//============================================================================

struct IRadSoundHalListener : public IRefCount
{
    virtual void  SetPosition( radSoundVector * ) = 0;
    virtual void  GetPosition( radSoundVector * ) = 0;
    virtual void  SetVelocity( radSoundVector * ) = 0;
    virtual void  GetVelocity( radSoundVector * ) = 0;
    virtual void  SetOrientation( radSoundVector * pFront, radSoundVector * pTop ) = 0;
    virtual void  GetOrientation( radSoundVector * pFront, radSoundVector * pTop ) = 0;

    virtual void  SetDistanceFactor( float ) = 0;
    virtual float GetDistanceFactor( void ) = 0;
    virtual void  SetDopplerFactor( float ) = 0;
    virtual float GetDopplerFactor( void ) = 0;
    virtual void  SetRollOffFactor( float ) = 0;
    virtual float GetRollOffFactor( void ) = 0;

    virtual void  SetEnvEffectsEnabled( bool enabled ) = 0;
    virtual bool  GetEnvEffectsEnabled( void ) = 0;
    virtual void  SetEnvironmentAuxSend( unsigned int auxsend ) = 0;
    virtual unsigned int GetEnvironmentAuxSend( void ) = 0;
};

//======================================================================
// Interface: IRadSoundHalPositionalGroup
//
// Description: A positional group may group 0 or more voies.  This
//		grouping concept is to minimize the amout of calculations.
//		For example, a character might have 2 voices for footsteps
//		1 voice for animation sounds and 1 voice for dialog.  Becaused
//      they are related to the same world object they could all
//      belong to the same group.
//
//      Obstruction is the muffling of sound by an object between the 
//      listener and the source, contained within a common environment 
//      (like a room). 
//
//      Occlusion is the muffling of sound by a partition or wall 
//      separating two environments. 
//
//======================================================================

struct IRadSoundHalPositionalGroup : public IRefCount
{
    virtual void  SetPosition( radSoundVector * pPosition ) = 0;
    virtual void  GetPosition( radSoundVector * pPosition ) = 0;
    virtual void  SetVelocity( radSoundVector * pVelocity ) = 0;
    virtual void  GetVelocity( radSoundVector * pVelocity ) = 0;
    virtual void  SetOrientation( radSoundVector * pFront, radSoundVector * pTop ) = 0;
    virtual void  GetOrientation( radSoundVector * pFront, radSoundVector * pTop ) = 0;
    virtual void  SetMinMaxDistance( float min, float max ) = 0;
    virtual void  GetMinMaxDistance( float * pMin, float * pMax ) = 0;
    virtual void  SetConeOutsideVolume( float ov ) = 0;
    virtual float GetConeOutsideVolume( void ) = 0;
    virtual void  SetConeAngles( float in, float out ) = 0;
    virtual void  GetConeAngles( float * pIn, float * pOut ) = 0;
    virtual void  SetOcclusion( float occl ) = 0;
    virtual float GetOcclusion( void ) = 0;
    virtual void  SetObstruction( float obst ) = 0;
    virtual float GetObstruction( void ) = 0;
};

//============================================================================
// Interface: IRadSoundHalEffect
//
// Description: All effects in the system derive from this interface.  The
//		actual effects are platform dependent and thus appear in the
//		radsound_???.hpp headers (where ??? is the platform).
//============================================================================

struct IRadSoundHalEffect : public IRefCount
{
    virtual void Attach( unsigned int auxSend ) = 0;
    virtual void Detach( void ) = 0;
    virtual void Update( void ) = 0;
};

//============================================================================
// Interface: IRadSoundSoundSystem
//
// Description: This is the one sigleton object that represents "The Sound
//      System"  call radSoundHalSystemGet( ) to retrieve a pointer to it.
//============================================================================

struct IRadSoundHalSystem : public IRefCount
{
	// Set up the root memory region.
    //
    // Because sound memory is managed externally, we need local memory to
    // manage it.  You are required to set an upper limit on the number of
    // allocation slots available in each memory region including the root.
    //
    // Sampling rate is only used by pc builds.  Set it equal to the maximum
    // sampling rate used by sounds in your game (e.g. 48000Hz).  Must be set
    // to 0 for other platforms, will assert.
    //
    // PC can optionally be initialized with StickyFocus flag.  This means that
    // sound will continue to play if the game's window loses focus, otherwise
    // the sounds will mute.

    struct SystemDescription
    {
        unsigned int m_MaxRootAllocations;
        unsigned int m_NumAuxSends;

        #if defined( RAD_WIN32 ) || defined( RAD_UWP )
        unsigned int m_ReservedSoundMemory;
        #endif 

        #if defined( RAD_WIN32 ) || defined( RAD_UWP )
        unsigned int m_SamplingRate;
        #endif
    };

    virtual void Initialize( const SystemDescription & systemDescription ) = 0;

	// Create a region tree based on this root to organize your sound memory

	virtual IRadSoundHalMemoryRegion * GetRootMemoryRegion( void ) = 0;
    virtual unsigned int GetNumAuxSends( ) = 0;

	// Assign effects to auxillary sends

	virtual void SetAuxEffect( unsigned int auxNumber, IRadSoundHalEffect * pIRadSoundHalEffect ) = 0;
	virtual IRadSoundHalEffect * GetAuxEffect( unsigned int auxNumber ) = 0;
    virtual void SetAuxGain( unsigned int aux, float gain ) = 0;
    virtual float GetAuxGain( unsigned int aux ) = 0;
    
    // The global output mode (see radSoundOutputMode) above.

	virtual void SetOutputMode( radSoundOutputMode mode ) = 0;
	virtual radSoundOutputMode GetOutputMode( void ) = 0;

	// Call Service() as often as possible

    virtual void Service( void ) = 0;
    
	// Call ServiceOncePerFrame() like you would call Render(), positions are
	// calculated and sounds are played.  You only need to call this function
	// once per frame.

	virtual void ServiceOncePerFrame( void ) = 0;

	//
	// Bulk memory statistics
	//

    struct Stats
    {
		unsigned int m_NumBuffers;
        unsigned int m_NumVoices;
        unsigned int m_NumVoicesPlaying;
        unsigned int m_NumPosVoices;
        unsigned int m_NumPosVoicesPlaying;
    
		unsigned int m_BufferMemoryUsed;       
        unsigned int m_EffectsMemoryUsed;	
		unsigned int m_TotalFreeSoundMemory;
    };

    virtual void GetStats( IRadSoundHalSystem::Stats * pStats ) = 0;
};

//============================================================================
// Interface: IRadSoundHalMemoryRegion
//
// Description:  Memory Regions allow you to create pools for sound in a
//		tree structure to avoid memory fragmentation.  You can create
//      buffers using only the memory returned from this region tree.
//		
//		Memory regions abstract the actual physical memory space ( XBOX Main
//		memory, PS2SPU ram, ARAM, etc. )
//============================================================================

struct IRadSoundHalMemoryRegion : public IRefCount
{
	// Create sub region, using memory from this region

	virtual IRadSoundHalMemoryRegion * CreateChildRegion(
		unsigned int sizeInBytes,
        unsigned int maxAllocations,
        const char * pIdentifier ) = 0;

	// Allocate memory from this region

	virtual void CreateMemoryObject(
		IRadMemoryObject ** ppIRadMemoryObject,
        unsigned int bytes,
        const char * pIdentifier ) = 0;

	// Functions for recursing through region tree.

	virtual IRadSoundHalMemoryRegion * GetParent( void ) = 0;

	virtual IRadSoundHalMemoryRegion * GetFirstChild( void ) = 0;	
	virtual IRadSoundHalMemoryRegion * GetNextChild(
		IRadSoundHalMemoryRegion * pIRSoundMemoryRegion_NextChild ) = 0;
	
	// Status funcions
	 
	virtual unsigned int GetSize( void ) = 0;

	virtual void GetStats( unsigned int * pFreeMemory,
		unsigned * pNumObjects, unsigned int * pLargestBlock,
		bool recurseChildren ) = 0;
};

#endif // RADSOUND_HAL