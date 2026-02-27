//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include "pch.hpp"
#include <radsound.hpp>
#include "streamplayer.hpp"

// extern unsigned int g_GameLoops;

bool g_VoiceStoppingPlayingSilence = false;

//========================================================================
// radSoundStreamPlayer::radSoundStreamPlayer
//========================================================================

radSoundStreamPlayer::radSoundStreamPlayer( void )
	:
	m_State( IRadSoundStreamPlayer::NoSource ),
    m_QueueingSubState( Queueing_None ),
    m_LastPlaybackPositionInSamples( 0 ),
	m_WritePositionInFrames( 0 ),
	m_Full( false ),
	m_OutstandingLoadSize( 0 ),
    m_OutstandingClearSize( 0 ),
	m_SourceFramesRead( 0 ),
    m_SourceSamplesPlayed( 0 ),
	m_EndFrameCounter( 0xFFFFFFFF ),
    m_LoadSkipLastFrame( false ),
    m_PollSkipLastFrame( false ),
    m_float_LowWaterMark( 0.5f ),
	m_InitializeInfo_Size( 0 ),
	m_InitializeInfo_SizeType( IRadSoundHalAudioFormat::Milliseconds )

{
    AddToUpdateList( );

    ::radStringCreate( & m_xIRadString_Name, GetThisAllocator( ) );

	m_xIRadSoundHalVoice = ::radSoundHalVoiceCreate( GetThisAllocator( ) );
}

//========================================================================
// radSoundStreamPlayer::~radSoundStreamPlayer
//========================================================================

radSoundStreamPlayer::~radSoundStreamPlayer( void )
{
    rDebugChannelPrintf(
        radSoundDebugChannel,
        "radsound: radSoundStreamPlayer destroyed: [%s]\n",
        m_xIRadString_Name->GetChars( ) );
}

//========================================================================
// radSoundStreamPlayer::InitializeAsync
//========================================================================

void radSoundStreamPlayer::InitializeAsync
(
	unsigned int size,
    IRadSoundHalAudioFormat::SizeType sizeType,
	IRadSoundHalMemoryRegion * pIRadSoundHalMemoryRegion,
    const char * pIdentifier
)
{
    rAssertMsg( pIdentifier != NULL, "You MUST name all of your stream players so we can track memory usage" );

	rAssert( size > 0 );
	rAssert( pIRadSoundHalMemoryRegion != NULL );

	m_InitializeInfo_Size = size;
	m_InitializeInfo_SizeType = sizeType;
	m_xInitializeInfo_MemRegion = pIRadSoundHalMemoryRegion;
    m_xIRadString_Name->Copy( pIdentifier );
}

//========================================================================
// radSoundStreamPlayer::Initialize
//========================================================================

void radSoundStreamPlayer::Initialize
(
	IRadSoundHalAudioFormat * pIRadSoundHalAudioFormat,
	unsigned int size,
    IRadSoundHalAudioFormat::SizeType sizeType,
	IRadSoundHalMemoryRegion * pIRadSoundHalMemoryRegion,
    const char * pIdentifier
)
{
	rAssert( pIRadSoundHalAudioFormat != NULL );
    rAssert( pIRadSoundHalMemoryRegion != NULL );

    rAssertMsg( pIdentifier != NULL, "You MUST name all of your stream players so we can track memory usage" );

	m_InitializeInfo_Size = size;
	m_InitializeInfo_SizeType = sizeType;
	m_xInitializeInfo_MemRegion = pIRadSoundHalMemoryRegion;
    m_xIRadString_Name->Copy( pIdentifier );

	AllocateResources( pIRadSoundHalAudioFormat );
}

//========================================================================
// radSoundStreamPlayer::SetLowWaterMark
//========================================================================

void radSoundStreamPlayer::SetLowWaterMark( float lowWaterMark )
{
    rAssert( lowWaterMark > 0.0f && lowWaterMark <= 1.0f );

    m_float_LowWaterMark = lowWaterMark;
}

//========================================================================
// radSoundStreamPlayer::GetLowWaterMark
//========================================================================

float radSoundStreamPlayer::GetLowWaterMark( void )
{
    return m_float_LowWaterMark;
}

//========================================================================
// radSoundStreamPlayer::AllocateResources
//========================================================================

void radSoundStreamPlayer::AllocateResources( IRadSoundHalAudioFormat * pIRadSoundHalAudioFormat )
{
	rAssert( pIRadSoundHalAudioFormat != NULL );

    //
    // If we are loading a new data source, check if it is the same format
    // if not, we just set buffer to null and reallocate, but print out
    // a usage warning.
    //

	if ( m_xIRadSoundHalVoice->GetBuffer( ) != NULL )
	{	
		if ( ! m_xIRadSoundHalVoice->GetBuffer( )->GetFormat( )->Matches( pIRadSoundHalAudioFormat ) )
		{
			IRadSoundHalAudioFormat::Encoding oldEncoding = m_xIRadSoundHalVoice->GetBuffer( )->GetFormat( )->GetEncoding( );
			IRadSoundHalAudioFormat::Encoding newEncoding = pIRadSoundHalAudioFormat->GetEncoding( );
			unsigned int oldChannels = m_xIRadSoundHalVoice->GetBuffer( )->GetFormat( )->GetNumberOfChannels( );
			unsigned int newChannels = pIRadSoundHalAudioFormat->GetNumberOfChannels( );
			
            rTuneWarningMsg( false, "\n\n\nAUDIO: ERROR: Streamer buffer realloc due to format mismatch...\n\n\n" );

			m_xIRadSoundHalVoice->SetBuffer( NULL );
		}
		else
		{
            m_xIRadSoundHalVoice->GetBuffer( )->ReSetAudioFormat( pIRadSoundHalAudioFormat );
			return;
		}
	}

	unsigned int sizeInFrames = pIRadSoundHalAudioFormat->ConvertSizeType(
		IRadSoundHalAudioFormat::Frames, m_InitializeInfo_Size, m_InitializeInfo_SizeType );

    unsigned int optimalFrameMultiple = pIRadSoundHalAudioFormat->BytesToFrames(
        radSoundHalDataSourceReadMultipleGet( ) );

    // Our buffer must be at least as big as two optimal reads.

    sizeInFrames = radMemoryRoundUp( sizeInFrames, optimalFrameMultiple * 2 );

    sizeInFrames = ::radSoundHalBufferCalculateMemorySize( IRadSoundHalAudioFormat::Frames,
        sizeInFrames, IRadSoundHalAudioFormat::Frames, pIRadSoundHalAudioFormat );

	unsigned int sizeInBytes = pIRadSoundHalAudioFormat->FramesToBytes( sizeInFrames );
	
    rTunePrintf( "AUDIO: Allocating stream resource in sound memory: [0x%x] Bytes\n", sizeInBytes );

	rAssert( ( sizeInBytes % pIRadSoundHalAudioFormat->GetFrameSizeInBytes( ) ) == 0 );	
	rAssert( m_xInitializeInfo_MemRegion != NULL );

	radRef< IRadMemoryObject > xIRadMemoryObject;

	m_xInitializeInfo_MemRegion->CreateMemoryObject(
        & xIRadMemoryObject, sizeInBytes, m_xIRadString_Name->GetChars( ) );

    // Check if we are out of memory, this would be bad.

	if ( xIRadMemoryObject != NULL )
	{
		radRef< IRadSoundHalBuffer > xIRadSoundHalBuffer = ::radSoundHalBufferCreate( GetThisAllocator( ) );
		xIRadSoundHalBuffer->Initialize( pIRadSoundHalAudioFormat, xIRadMemoryObject, sizeInFrames, true, true );
		m_xIRadSoundHalVoice->SetBuffer( xIRadSoundHalBuffer );
	}
	else
	{
		rAssertMsg( false, "Out of memory" );
	}
}

//========================================================================
// radSoundStreamPlayer::Play
//========================================================================

void radSoundStreamPlayer::Play( void )
{
	if ( m_State == IRadSoundStreamPlayer::Paused )
	{
        // If we are paused, we are all queued up and ready to go.

        rAssert( m_xIRadSoundHalVoice->IsPlaying( ) == false );

		m_xIRadSoundHalVoice->Play( );

		SetState( IRadSoundStreamPlayer::Playing );
	}
	else if ( m_State == IRadSoundStreamPlayer::Queueing )
	{
        // If we are queueing, we can't start the voice, but just set
        // a flag signalling us to start playing immediately after
        // we have queued up.

		SetState( IRadSoundStreamPlayer::QueuedPlay );
	}
}

//========================================================================
// radSoundStreamPlayer::SetState
//========================================================================

void radSoundStreamPlayer::SetState( IRadSoundStreamPlayer::State state )
{
    m_State = state;
}

//========================================================================
// radSoundStreamPlayer::Stop
//========================================================================

void radSoundStreamPlayer::Stop( void )
{
    if ( m_State == IRadSoundStreamPlayer::QueuedPlay )
    {
       SetState( IRadSoundStreamPlayer::Queueing );
    }
    else if( m_State == IRadSoundStreamPlayer::Playing )
    {
        StopVoice( false );

        SetState( IRadSoundStreamPlayer::Paused );
    }
}

void radSoundStreamPlayer::StopVoice( bool playingSilence )
{
    g_VoiceStoppingPlayingSilence = playingSilence;
    m_xIRadSoundHalVoice->Stop( );
    g_VoiceStoppingPlayingSilence = false;
}

//========================================================================
// radSoundStreamPlayer::IsPlaying
//========================================================================

bool radSoundStreamPlayer::IsPlaying( void )
{
	return ( m_State == IRadSoundStreamPlayer::Playing ) ||
           ( m_State == IRadSoundStreamPlayer::QueuedPlay );
}

//========================================================================
// radSoundStreamPlayer::SetDataSource
//========================================================================

void radSoundStreamPlayer::SetDataSource( IRadSoundHalDataSource * pIRadSoundHalDataSource )
{
    // rDebugPrintf( "SetDataSource: Gl:[%d] Ptr: [0x%x]\n", g_GameLoops, pIRadSoundHalDataSource );

    // Stop and reset the voice.

	StopVoice( false );
    m_xIRadSoundHalVoice->SetPlaybackPositionInSamples( 0 );

    //
    // Stop and reset the buffer.
    //

    if ( m_xIRadSoundHalVoice->GetBuffer( )  != NULL )
    {
        m_xIRadSoundHalVoice->GetBuffer( )->CancelAsyncOperations( );
    }

	m_xIRadSoundHalDataSource = pIRadSoundHalDataSource;

    m_LastPlaybackPositionInSamples = 0;
	m_SourceFramesRead = 0;
    m_SourceSamplesPlayed = 0;
	m_EndFrameCounter = 0xFFFFFFFF;
    m_OutstandingLoadSize = 0;
    m_OutstandingClearSize = 0;
    m_WritePositionInFrames = 0;
    m_Full = false;
    m_LoadSkipLastFrame = false;
    m_PollSkipLastFrame = false;

	if ( m_xIRadSoundHalDataSource != NULL )
	{
		SetState( IRadSoundStreamPlayer::Queueing );
	    m_QueueingSubState = Queueing_WaitingForDataSourceToInitialize;                
	}
	else
	{
        m_QueueingSubState = Queueing_None;
		SetState( IRadSoundStreamPlayer::NoSource );
	}

    Update( 0 );
}

//========================================================================
// radSoundStreamPlayer::OnBufferLoadComplete
//========================================================================

void radSoundStreamPlayer::OnBufferLoadComplete( unsigned int dataSourceFrames )
{
	rAssert( m_OutstandingLoadSize > 0 );
	rAssert( m_OutstandingLoadSize >= dataSourceFrames );

	if ( m_OutstandingLoadSize > dataSourceFrames )
	{
        rAssert( m_EndFrameCounter == 0xFFFFFFFF );
	
        m_EndFrameCounter = m_xIRadSoundHalVoice->GetBuffer( )->GetSizeInFrames( ) -
            ( m_OutstandingLoadSize - dataSourceFrames );
            
	}

    //
    // We assume that even though less frames were available than requested,
    // the buffereloader filled the remaining frames with silence
    //

	m_WritePositionInFrames = ( m_WritePositionInFrames + m_OutstandingLoadSize ) %
        m_xIRadSoundHalVoice->GetBuffer( )->GetSizeInFrames( );

    // But we still need to keep track of the amount of actual data
    // read.

	m_SourceFramesRead += dataSourceFrames;

    // Reset loading "flag"

	m_OutstandingLoadSize = 0;

    // rDebugPrintf( "Load Complete: Gl:[%d]\n", g_GameLoops );
}

//========================================================================
// radSoundStreamPlayer::OnBufferClearComplete
//========================================================================

void radSoundStreamPlayer::OnBufferClearComplete( void )
{
    rAssert( m_OutstandingClearSize > 0 );

    // Bump up our write position.

	m_WritePositionInFrames = ( m_WritePositionInFrames + m_OutstandingClearSize )
        % m_xIRadSoundHalVoice->GetBuffer( )->GetSizeInFrames( );

    // We are always ending if we are clearing, so update our end frame
    // counter.  It is possible that we loaded more data than was left
    // until the end marker, just set end frame counter to zero in this
    // case, otherwise update the counter with how many bytes we
    // did clear.

    if( m_OutstandingClearSize > m_EndFrameCounter )
    {
        m_EndFrameCounter = 0;
    }
    else
    {
        m_EndFrameCounter -= m_OutstandingClearSize;
    }

    m_OutstandingClearSize = 0; 
}

//========================================================================
// radSoundStreamPlayer::Update
//========================================================================

void radSoundStreamPlayer::Update( unsigned int elapsedTime )
{
    // Spin the state machine until we have to wait for IO.
    
    IRadSoundStreamPlayer::State prevState;
    QueueingSubState             prevQueueingSubState;
    unsigned int                 prevOutstandingLoadSize;

    do
    {
        prevState                = m_State;
        prevQueueingSubState     = m_QueueingSubState;
        prevOutstandingLoadSize  = m_OutstandingLoadSize;

        ServiceStateMachine( );

    } while( ( m_State != prevState ) ||
             ( m_QueueingSubState != prevQueueingSubState ) ||
             ( prevOutstandingLoadSize != m_OutstandingLoadSize ) );

}

//========================================================================
// radSoundStreamPlayer::ServiceStateMachine
//========================================================================

void radSoundStreamPlayer::ServiceStateMachine( void )
{
	if ( m_State == IRadSoundStreamPlayer::NoSource )
	{
        //
        // We are idle, no data source to play, nothing to do.
        //
    }
    else if ( m_State == IRadSoundStreamPlayer::Queueing || m_State == IRadSoundStreamPlayer::QueuedPlay )
    {
        // There are two queing states, we are either waiting for the data
        // source to initialize, or we are waiting for the fist block to load

        if ( m_QueueingSubState == Queueing_WaitingForDataSourceToInitialize )
        {
		    if ( m_xIRadSoundHalDataSource->GetState( ) != IRadSoundHalDataSource::Initialized )
		    {
			    // Do nothing, the data source is still not initialized
		    }
		    else
		    {
                // Ok the data source is done, allocate our resources--this
                // function might do nothing if our buffer is aready allocated.

			    AllocateResources( m_xIRadSoundHalDataSource->GetFormat( ) );
                m_QueueingSubState = Queueing_LoadingFirstBlock;
		    }
        }
        else if ( m_QueueingSubState == Queueing_LoadingFirstBlock )
        {
            // Check to see if we are finished loading the first block

		    if ( m_Full == true && m_OutstandingLoadSize == 0 && m_OutstandingClearSize == 0 )
		    {
                // Here we have queued up an entire buffers worth of data, so move on
                // to one of the non-queuing states and start the voice if
                // we are in "queuedplay" mode.

                m_QueueingSubState = Queueing_None;

	            if ( m_State == IRadSoundStreamPlayer::Queueing )
	            {
                    SetState( IRadSoundStreamPlayer::Paused );
	            }
	            else if ( m_State == IRadSoundStreamPlayer::QueuedPlay )
	            {
                    rAssert( m_xIRadSoundHalVoice->IsPlaying( ) == false );

                    m_xIRadSoundHalVoice->Play( );

                    SetState( IRadSoundStreamPlayer::Playing );
	            }
    
                // rDebugPrintf( "Queued: Gl:[%d]\n", g_GameLoops );

            }
            else
            {
                // Fist block is still not loaded, keep loading.

                ServiceLoad( );
            }
        }
        else
        {
            rAssert( false ); // ooops logic problem in streamer!
        }
    }
    else if ( m_State == IRadSoundStreamPlayer::Playing )
    {
        if ( ServicePlay( ) )
        {
            ServiceLoad( ); // Just keep the buffer full
        }
    }
    else if ( m_State == IRadSoundStreamPlayer::Paused )
    {
        ServiceLoad( ); // And keep the buffer full even if we are paused.
    }
}

//========================================================================
// radSoundStreamPlayer::ServicePlay
//========================================================================

bool radSoundStreamPlayer::ServicePlay( void )
{
    IRadSoundHalBuffer * pIRshb = m_xIRadSoundHalVoice->GetBuffer( );
    IRadSoundHalAudioFormat * pIRshaf = pIRshb->GetFormat( );

    unsigned int bufferSizeInSamples = pIRshaf->FramesToSamples( pIRshb->GetSizeInFrames( ) );
    unsigned int playbackPositionInSamples = m_xIRadSoundHalVoice->GetPlaybackPositionInSamples( );
    unsigned int sourceSamplesLoaded = pIRshaf->FramesToSamples( m_SourceFramesRead );
    
    unsigned int samplesPlayedThisFrame;

    if ( playbackPositionInSamples >= m_LastPlaybackPositionInSamples )
    {
        samplesPlayedThisFrame = playbackPositionInSamples - m_LastPlaybackPositionInSamples;
    }
    else
    {
        samplesPlayedThisFrame = bufferSizeInSamples - ( m_LastPlaybackPositionInSamples - playbackPositionInSamples );
    }
        
    m_SourceSamplesPlayed += samplesPlayedThisFrame;

    if ( samplesPlayedThisFrame > ( bufferSizeInSamples / 3 ) )
    {
        if ( m_PollSkipLastFrame == false )
        {
            #ifndef RAD_DEBUG
                rTunePrintf( "radsound: TRC Violation: Streamer: [%s] is skipping (polling latency).\n", m_xIRadString_Name->GetChars( ) );
            #endif
            
            m_PollSkipLastFrame = true;
        }   
    }
    else
    {
        m_PollSkipLastFrame = false;
    }
            
    if ( m_SourceSamplesPlayed > sourceSamplesLoaded )
    {
        if ( m_EndFrameCounter == 0xFFFFFFFF )
        {
            if ( m_LoadSkipLastFrame == false )
            {
                m_LoadSkipLastFrame = true;
                #ifndef RAD_DEBUG                
                    rTunePrintf( "radsound: TRC Violation: Streamer: [%s] is skipping (loading latency).\n", m_xIRadString_Name->GetChars( ) );
                #endif
            }
        }
        else
        {
            StopVoice( true ); // true means don't print out trc warning
		    SetDataSource( NULL );
		    return false;
        }

        m_SourceSamplesPlayed = sourceSamplesLoaded;
    }
    else
    {
        m_LoadSkipLastFrame = false;
    }
    
    m_LastPlaybackPositionInSamples = playbackPositionInSamples;
    
    return true;
}

//========================================================================
// radSoundStreamPlayer::ServiceLoad
//========================================================================

void radSoundStreamPlayer::ServiceLoad( void )
{
    // If we currently have an outstanding load or clear, we can't issue
    // another one until it is done so just do nothing for now.

    radRef< IRadSoundHalAudioFormat > xIRadSoundHalAudioFormat = m_xIRadSoundHalVoice->GetBuffer( )->GetFormat( );

	unsigned int bufferSizeInFrames = m_xIRadSoundHalVoice->GetBuffer( )->GetSizeInFrames( );

    unsigned int frameSizeInSamples =
        xIRadSoundHalAudioFormat->BytesToSamples( xIRadSoundHalAudioFormat->GetFrameSizeInBytes( ) );

    unsigned int currentPlaybackPositionInFrames =
    	m_xIRadSoundHalVoice->GetPlaybackPositionInSamples( ) / frameSizeInSamples;

    rAssert( currentPlaybackPositionInFrames < bufferSizeInFrames ); // Sanity check

	if ( m_OutstandingLoadSize == 0 && m_OutstandingClearSize == 0 )
	{
        unsigned int lowWaterMarkInFrames =
            radSoundFloatToUInt(  radSoundUIntToFloat( bufferSizeInFrames ) * ( 1.0f - m_float_LowWaterMark ) );

        unsigned int optimalFrameMultiple = xIRadSoundHalAudioFormat->BytesToFrames(
            radSoundHalDataSourceReadMultipleGet( ) * 2 );
        
        //
        // Calculate how may frames we need to load (if any).  Because the
        // playback pointer and write position can be at the same position,
        // we need a flag to specify whether this is a full or empty condition
        //

		unsigned int framesNeeded;

        if ( currentPlaybackPositionInFrames == m_WritePositionInFrames )
        {
            // Here the playback pointer and write pointer are the same, which
            // means we are either full or empty, if we are full, we don't
            // need to load any data, otherwise we need to load the entire
            // buffer.

            if ( m_Full )
            {
                framesNeeded = 0;
            }
            else
            {
                framesNeeded = bufferSizeInFrames;
            }
        }
        else
        {
            // Here the playback pointer and write pointer are at a different
            // spot in the buffer--there are two cases: one where the whole area to
            // load wraps around the buffer and two when the whole area to
            // load is contiguous in the buffer.

		    if ( currentPlaybackPositionInFrames > m_WritePositionInFrames )
		    {
                // The area to load is contiguous between the write and
                // play pointers

			    framesNeeded = currentPlaybackPositionInFrames - m_WritePositionInFrames;
		    }
		    else
		    {
                // The area to load wraps, so we calculate the area NOT
                // between the play and write pointers.

			    framesNeeded = bufferSizeInFrames - ( m_WritePositionInFrames - currentPlaybackPositionInFrames );
		    }
        }

        framesNeeded = ::radMemoryRoundDown( framesNeeded,
            m_xIRadSoundHalVoice->GetBuffer( )->GetMinTransferSize( IRadSoundHalAudioFormat::Frames ) );

        framesNeeded = ::radMemoryRoundDown( framesNeeded,
            optimalFrameMultiple );

        // If we have hit our low water mark we load, otherwise just
        // do nothing and try again next time.  If we are clearing silence
        // at the very end of the buffer (stopping), we don't care about
        // the low water mark because we won't hit the disk.

        if ( framesNeeded >= lowWaterMarkInFrames || m_EndFrameCounter != 0xFFFFFFFF )
        {
		    // The buffer is cirular and we can only read continuous chunks so
            // adjust frames needed so it won't write past the end of the buffer.
            // If we have to adjust the frames needed it means we are not full,
            // otherwise we have loaded right up to the play pointer and we
            // are full (which was is always our ultimate goal).

		    if ( (m_WritePositionInFrames + framesNeeded) > bufferSizeInFrames )
		    {
                // Oops, we have to wrap, wich means we are not full...

			    m_Full = false;
			    framesNeeded = framesNeeded - ( (m_WritePositionInFrames + framesNeeded) % bufferSizeInFrames );
		    }
		    else
		    {
                // Otherwise we can read the entire thing (or nothing at all) which fills us up
                // completely.
			    m_Full = true;
		    }
        
            // Ok, if we have frames to read, do it.

		    if ( framesNeeded > 0  )
		    {
                if ( m_EndFrameCounter == 0 )
                {
                    // This is kind of tricky, but if we need to read frames here, it means
                    // the playback pointer has moved past the end of the data--
                    // go into the "NoSource" state.

                    StopVoice( true ); // true means don't print out trc warning
		            SetDataSource( NULL );
                }
                else if ( m_EndFrameCounter != 0xFFFFFFFF )
			    {
                    // Here we are out of data and are clearing ahead of the play
                    // pointer so we don't play garbage.  We would otherwise always
                    // play a bit of garbage due to the game loop latency.

                    m_OutstandingClearSize = framesNeeded;

				    m_xIRadSoundHalVoice->GetBuffer( )->ClearAsync(
					    m_WritePositionInFrames, framesNeeded, this );			
			    }
			    else
			    {
                    //
                    // Here we are not of data, so load the bytes from the data
                    // source.
                    //
                                        
				    
				    unsigned int available = m_xIRadSoundHalDataSource->GetAvailableFrames( );
				    unsigned int remaining = m_xIRadSoundHalDataSource->GetRemainingFrames( );
				    
				    if ( available == 0xFFFFFFFF )
				    {
				        // unbuffered
				    
                         m_OutstandingLoadSize = framesNeeded;				    
				    }
				    else
				    {
                        rAssert(available <= remaining);

                        if ( remaining == 0xFFFFFFFF )
                        {
                            // not out of data
                            
                            if ( available >= framesNeeded )
                            {
                                // enough to satisfy request
                                m_OutstandingLoadSize = framesNeeded;
                            }
                            else
                            {
                                // not enough to satisfy request.
                                m_OutstandingLoadSize = available;
                            }
                        }
                        else
                        {
                            // out of data
                            
                            if ( available >= remaining )
                            {
                                // last chunk remaining.
                                
                                m_OutstandingLoadSize = framesNeeded;
                            }
                            else
                            {
                                // still more to chunk
                                                        
                                if ( available < framesNeeded )
                                {
                                    m_OutstandingLoadSize = available;
                                }
                                else
                                {
                                    m_OutstandingLoadSize = framesNeeded;
                                }
                            }
                        }
                    } // buffered?

                    if ( m_OutstandingLoadSize < framesNeeded )
                    {
                        m_Full = false; // icky
                    }
                    
                    if ( m_OutstandingLoadSize > 0 )
                    {
				        m_xIRadSoundHalVoice->GetBuffer( )->LoadAsync(
					        m_xIRadSoundHalDataSource,
					        m_WritePositionInFrames,
					        m_OutstandingLoadSize, this );
                    }
			    }
		    }
	    }
    }
}

//========================================================================
// radSoundStreamPlayer::GetPlaybackTimeInSamples
//========================================================================

unsigned int radSoundStreamPlayer::GetPlaybackTimeInSamples( void )
{
    
    IRadSoundHalBuffer * pIRshb  = m_xIRadSoundHalVoice->GetBuffer( );
    
    if( pIRshb == NULL )
    {
        return 0; // not initialized yet
    }
    
    ServicePlay( ); // Make sure we get the most accureate reading!

    return m_SourceSamplesPlayed;
}

//========================================================================
// radSoundStreamPlayer::SetPriority
//========================================================================

void radSoundStreamPlayer::SetPriority( unsigned int priority )
{
	m_xIRadSoundHalVoice->SetPriority( priority );
}

//========================================================================
// radSoundStreamPlayer::GetPriority
//========================================================================

unsigned int radSoundStreamPlayer::GetPriority( void )
{
	return m_xIRadSoundHalVoice->GetPriority( );
}

//========================================================================
// radSoundStreamPlayer::SetMuted
//========================================================================

void  radSoundStreamPlayer::SetMuted( bool muted )
{
	m_xIRadSoundHalVoice->SetMuted( muted );
}

//========================================================================
// radSoundStreamPlayer::GetMuted
//========================================================================

bool  radSoundStreamPlayer::GetMuted( void )
{
	return m_xIRadSoundHalVoice->GetMuted( );
}

//========================================================================
// radSoundStreamPlayer::SetVolume
//========================================================================

void  radSoundStreamPlayer::SetVolume( float volume )
{
	m_xIRadSoundHalVoice->SetVolume( volume );
}

//========================================================================
// radSoundStreamPlayer::GetVolume
//========================================================================

float radSoundStreamPlayer::GetVolume( void )
{
	return m_xIRadSoundHalVoice->GetVolume( );
}

//========================================================================
// radSoundStreamPlayer::SetTrim
//========================================================================

void  radSoundStreamPlayer::SetTrim( float trim )
{
	m_xIRadSoundHalVoice->SetTrim( trim );
}

//========================================================================
// radSoundStreamPlayer::GetTrim
//========================================================================

float radSoundStreamPlayer::GetTrim( void )
{
	return m_xIRadSoundHalVoice->GetTrim( );
}

//========================================================================
// radSoundStreamPlayer::SetPitch
//========================================================================

void  radSoundStreamPlayer::SetPitch( float pitch )
{
	m_xIRadSoundHalVoice->SetPitch( pitch );
}

//========================================================================
// radSoundStreamPlayer::GetPitch
//========================================================================

float radSoundStreamPlayer::GetPitch( void )
{
	return m_xIRadSoundHalVoice->GetPitch( );
}

//========================================================================
// radSoundStreamPlayer::SetPan
//========================================================================

void  radSoundStreamPlayer::SetPan( float pan )
{
	m_xIRadSoundHalVoice->SetPan( pan );
}

//========================================================================
// radSoundStreamPlayer::GetPan
//========================================================================

float radSoundStreamPlayer::GetPan( void )
{
	return m_xIRadSoundHalVoice->GetPan( );
}

//========================================================================
// radSoundStreamPlayer::SetAuxMode
//========================================================================

void  radSoundStreamPlayer::SetAuxMode( unsigned int aux, radSoundAuxMode mode )
{
	m_xIRadSoundHalVoice->SetAuxMode( aux, mode );

}

//========================================================================
// radSoundStreamPlayer::GetAuxMode
//========================================================================

radSoundAuxMode radSoundStreamPlayer::GetAuxMode( unsigned int aux )
{
	return m_xIRadSoundHalVoice->GetAuxMode( aux );
}

//========================================================================
// radSoundStreamPlayer::SetAuxGain
//========================================================================

void  radSoundStreamPlayer::SetAuxGain( unsigned int aux, float gain )
{
	m_xIRadSoundHalVoice->SetAuxGain( aux, gain );
}

//========================================================================
// radSoundStreamPlayer::GetAuxGain
//========================================================================

float radSoundStreamPlayer::GetAuxGain( unsigned int aux )
{
	return m_xIRadSoundHalVoice->GetAuxGain( aux );
}

//========================================================================
// radSoundStreamPlayer::SetPositionalGroup
//========================================================================

void radSoundStreamPlayer::SetPositionalGroup( IRadSoundHalPositionalGroup * pIRshpg )
{
	m_xIRadSoundHalVoice->SetPositionalGroup( pIRshpg );
}

//========================================================================
// radSoundStreamPlayer::GetPositionalGroup
//========================================================================

IRadSoundHalPositionalGroup * radSoundStreamPlayer::GetPositionalGroup( void )
{
	return m_xIRadSoundHalVoice->GetPositionalGroup( );
}

//========================================================================
// radSoundStreamPlayer::GetDataSource
//========================================================================

IRadSoundHalDataSource * radSoundStreamPlayer::GetDataSource( void )
{
	return m_xIRadSoundHalDataSource;
}

//========================================================================
// radSoundStreamPlayer::GetFormat
//========================================================================

IRadSoundHalAudioFormat * radSoundStreamPlayer::GetFormat( void )
{
    rAssertMsg( m_xIRadSoundHalVoice->GetBuffer( ) != NULL, "Can't get format of streamer until it is initilized" );

    IRadSoundHalBuffer * pIRshb = m_xIRadSoundHalVoice->GetBuffer( );

    return pIRshb->GetFormat( );
}

//========================================================================
// radSoundStreamPlayer::GetState
//========================================================================

IRadSoundStreamPlayer::State radSoundStreamPlayer::GetState( )
{
    return m_State;
}

//========================================================================
// ::radSoundStreamPlayerCreate
//========================================================================

IRadSoundStreamPlayer * radSoundStreamPlayerCreate( radMemoryAllocator allocator )
{
	return new ( "radSoundStreamPlayer", allocator ) radSoundStreamPlayer;
}
