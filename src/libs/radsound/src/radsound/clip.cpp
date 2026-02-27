//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//
// File:        clip.cpp
//
// Subsystem:	Radical Sound System
//
// Description:	Gamecube implementation of the clip component.
//
//=============================================================================
#include "pch.hpp"
#include <radfile.hpp>
#include <radobjectlist.hpp>

#include <radsound.hpp>
#include <radsoundfile.hpp>

#include "clip.hpp"

//========================================================================
// Macros
//========================================================================

//
// These all help with the empty data sources
//

#define MIN_FRAMES 2

#if defined( RAD_WIN32 ) || defined( RAD_UWP )
	#define PLATFORM_AUDIO_FORMAT IRadSoundHalAudioFormat::PCM
#endif

//========================================================================
// Class Static Init
//========================================================================

template<> radSoundClip * radLinkedClass< radSoundClip >::s_pLinkedClassHead = NULL;
template<> radSoundClip * radLinkedClass< radSoundClip >::s_pLinkedClassTail = NULL;

//======================================================================
// Component: radSoundClip
//======================================================================

radSoundClip::radSoundClip( void )
	:
	m_Looping( false ),
	m_Trim( 1.0f ),
	m_State( NoFile )
{   
    ::radStringCreate( & m_xIRadString_Name, GetThisAllocator( ) );
}

//======================================================================
// radSoundClip::~radSoundClip
//======================================================================

radSoundClip::~radSoundClip( void )
{
    rDebugChannelPrintf( radSoundDebugChannel,
        "radsound: Clip Destroyed: [%s]\n", m_xIRadString_Name->GetChars( ) );
}

//======================================================================
// radSoundClip::GetMemoryRegion
//======================================================================

IRadSoundHalMemoryRegion * radSoundClip::GetMemoryRegion
(
    void
)
{
    return m_xIRadSoundHalMemoryRegion;
}
 
//======================================================================
// radSoundClip::Initialize
//======================================================================
       
void radSoundClip::Initialize
(
	IRadSoundHalDataSource * pIRshds,
	IRadSoundHalMemoryRegion * pIRadSoundHalMemoryRegion,
	bool looping,
    const char * pIdentifier
)
{
    rAssert( pIRshds != NULL );
    rAssert( pIRadSoundHalMemoryRegion != NULL );
    rAssertMsg( pIdentifier != NULL, "You MUST name all of your sound clips so we can track memory usage" );

    m_xIRadString_Name->Copy( pIdentifier );
    m_xIRadSoundHalMemoryRegion = pIRadSoundHalMemoryRegion;
	m_xIRadSoundHalDataSource = pIRshds;
	m_Looping = looping;

    SetState( IRadSoundClip::Initializing );

    AddToUpdateList( );
    
    Update( 0 ); // 1 frame kick start.
}

//======================================================================
// radSoundClip::GetLooping
//======================================================================

/* virtual */ bool radSoundClip::GetLooping( void )
{
	return m_Looping;
}

//======================================================================
// radSoundClip::GetFormat
//======================================================================

/* virtual */ IRadSoundHalAudioFormat * radSoundClip::GetFormat( void )
{
    if ( m_xIRadSoundHalBuffer != NULL )
    {
	    return m_xIRadSoundHalBuffer->GetFormat( );
    }

    return NULL;
}

//======================================================================
// radSoundClip::SetTrim
//======================================================================

/* virtual */ void radSoundClip::SetTrim( float trim )
{
	m_Trim = trim;
}

//======================================================================
// radSoundClip::GetTrim
//======================================================================

/* virtual */ float radSoundClip::GetTrim( void )
{
	return m_Trim;
}

//========================================================================
// radSoundClip::SetState
//========================================================================

void radSoundClip::SetState( IRadSoundClip::State state )
{
    m_State = state;
}

//========================================================================
// radSoundClip::GetState
//========================================================================

/* virtual */ IRadSoundClip::State radSoundClip::GetState( void )
{
	return m_State;
}

//========================================================================
// radSoundClip::WillRelease
//========================================================================

/* virtual */ bool radSoundClip::WillRelease( void )
{
    return GetRefCount( ) <= 1;
}
        
//======================================================================
// radSoundClip::OnTimerDone
//======================================================================

/* virtual */ void radSoundClip::Update( unsigned int elapsedTime )
{

	if ( m_xIRadSoundHalDataSource->GetState( ) == IRadSoundHalDataSource::Initialized )
	{
	    rAssertMsg( m_xIRadSoundHalDataSource->GetRemainingFrames( ) != 0xFFFFFFFF, 
		    "radSoundClip: DataSource reports an unknown number of remaining frames.  Clips must use finite DataSources" );
		// I'll be registering for some asynchronous funtions.
		// If they're not really asynchronous then they could
		// cause me to get destroyed too early

		AddRef( );

		// Here are some things that will be calculated in various ways

		unsigned int initialBufferSizeInFrames = 0;
		unsigned int initialBufferMemorySizeInBytes = 0;
		radRef< IRadSoundHalAudioFormat > xIRadSoundHalAudioFormat = NULL;

		if( m_xIRadSoundHalDataSource->GetRemainingFrames( ) >= MIN_FRAMES )
		{
			// This is a normal data source that actually contains data

			xIRadSoundHalAudioFormat = m_xIRadSoundHalDataSource->GetFormat( );
			rAssert( xIRadSoundHalAudioFormat );  // It would be weird for this to be null

            // Calculate the initial buffer size and buffer memory size

            initialBufferSizeInFrames = m_xIRadSoundHalDataSource->GetRemainingFrames( );

            initialBufferMemorySizeInBytes = ::radSoundHalBufferCalculateMemorySize( IRadSoundHalAudioFormat::Bytes, 
                initialBufferSizeInFrames, IRadSoundHalAudioFormat::Frames, xIRadSoundHalAudioFormat );
		}
		else
		{
			//
			// If the data source is empty, it will not provide an
			// audio format.  We need to handle an empty data source
			// somehow.  So we'll give ourselves some kind of a 
			// dummy format.
			//

			xIRadSoundHalAudioFormat = ::radSoundHalAudioFormatCreate( RADMEMORY_ALLOC_TEMP );
			xIRadSoundHalAudioFormat->Initialize(PLATFORM_AUDIO_FORMAT, NULL, 22050, 1,	16 );

			// Give the buffer some kind of a minimum size
			// just so that we can carry on

            initialBufferMemorySizeInBytes = ::radSoundHalBufferCalculateMemorySize( IRadSoundHalAudioFormat::Bytes,
                MIN_FRAMES, IRadSoundHalAudioFormat::Frames, xIRadSoundHalAudioFormat );

            initialBufferSizeInFrames = xIRadSoundHalAudioFormat->BytesToFrames( initialBufferMemorySizeInBytes );
		}

		// Set up the new buffer's memory object

		radRef< IRadSoundHalMemoryRegion > xIRadSoundHalMemoryRegion;

		// If we weren't provided with a memory region, we are
		// perfectly capable for grabbing our own

		if ( m_xIRadSoundHalMemoryRegion != NULL )
		{
			xIRadSoundHalMemoryRegion = m_xIRadSoundHalMemoryRegion;        
		}
		else
		{
			xIRadSoundHalMemoryRegion = ::radSoundHalSystemGet( )->GetRootMemoryRegion( );
		}

		radRef< IRadMemoryObject > xIRadMemoryObject;
		xIRadSoundHalMemoryRegion->CreateMemoryObject( & xIRadMemoryObject,
				initialBufferMemorySizeInBytes,
                m_xIRadString_Name->GetChars( ) );
    
		if ( xIRadMemoryObject != NULL )
		{	
			// We are ready to set up the buffer

			m_xIRadSoundHalBuffer = ::radSoundHalBufferCreate( GetThisAllocator( ) );
        
			m_xIRadSoundHalBuffer->Initialize
			(
				 xIRadSoundHalAudioFormat,
				 xIRadMemoryObject,
                 initialBufferSizeInFrames,
				 m_Looping,
				 false

			);

			if ( m_xIRadSoundHalDataSource->GetRemainingFrames( ) >= MIN_FRAMES )
			{
				// Load the data source if it has something to offer

				m_xIRadSoundHalBuffer->LoadAsync
				(
					m_xIRadSoundHalDataSource,
					0,                                          // Buffer frame offset
					m_xIRadSoundHalBuffer->GetSizeInFrames( ),
					this
				);
			}
			else
			{
				// Otherwise make sure that it plays silently

				m_xIRadSoundHalBuffer->ClearAsync( 0, m_xIRadSoundHalBuffer->GetSizeInFrames( ), this );
			}
		}
		else
		{
			char buf[ 128 ];
			unsigned int freeBytesInRegion = 0;
			xIRadSoundHalMemoryRegion->GetStats( & freeBytesInRegion, NULL, NULL, false );

			rTunePrintf( "radSoundClip: [%s] Out of memory.\n", m_xIRadString_Name->GetChars( )  );
        
			//sprintf( buf, "	   * File: [%s]\n", m_xIRadFile->GetFilename( ) );
			//rTuneString( buf );

			sprintf( buf, "    * Allocating:     [%d] Bytes.\n", initialBufferMemorySizeInBytes );
			rTuneString( buf );

			sprintf( buf, "    * Free in region: [%d] Bytes\n", freeBytesInRegion );
			rTuneString( buf );

            rTuneAssert( false );
			
			SetState( IRadSoundClip::NoFile );
		}

		m_xIRadSoundHalDataSource = NULL;

		RemoveFromUpdateList( );

		// Now I'm safe

		Release( );
	}
       
}

//======================================================================
// radSoundClip::OnBufferLoadComplete
//======================================================================

/* virtual */ void radSoundClip::OnBufferLoadComplete( unsigned framesLoaded )
{
	rAssert( framesLoaded == m_xIRadSoundHalBuffer->GetSizeInFrames( ) );

	SetState( IRadSoundClip::Initialized );
}

//======================================================================
// radSoundClip::OnBufferLoadComplete
//======================================================================

/* virtual */ void radSoundClip::OnBufferClearComplete( void )
{
	SetState( IRadSoundClip::Initialized );
}

//======================================================================
// ::radSoundClipCreate
//======================================================================

IRadSoundClip * radSoundClipCreate( radMemoryAllocator allocator )
{
	return new ( "radSoundClip", allocator ) radSoundClip( );
}


IRadSoundHalBuffer * radSoundClip::GetBuffer( void )
{
    return m_xIRadSoundHalBuffer;
}

void radSoundDumpClipMemory( void )
{
	char buf[ 128 ];

	rDebugString( "radSoundClip Dump:\n" );
	rDebugString( "------------------\n" );

	radSoundClip * pSearch = radSoundClip::GetLinkedClassHead( );

	while ( pSearch != NULL )
	{
		IRadSoundHalBuffer * pIRadSoundHalBuffer = pSearch->GetBuffer( );

		if ( pIRadSoundHalBuffer != NULL )
		{
			sprintf(
				buf,
				"Size: [%d] File: [%s]\n",
				pIRadSoundHalBuffer->GetFormat( )->FramesToBytes( pIRadSoundHalBuffer->GetSizeInFrames( ) ),
				"Unknown" );

				rDebugString( buf );
		}

		pSearch = pSearch->GetLinkedClassNext( );
	}
}
