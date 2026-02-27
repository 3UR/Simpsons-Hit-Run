//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//
// File:        clipplayer.cpp
//
// Subsystem:	Radical Sound System
//
// Description:	Gamecube implementation of the ClipPlayer component.
//
//=============================================================================

//======================================================================
// Include Files
//======================================================================

#include "pch.hpp"
#include <radfile.hpp>
#include <radobjectlist.hpp>

#include <radsound.hpp>

#include "clipplayer.hpp"


//======================================================================
// radSoundClipPlayer::radSoundClipPlayer
//======================================================================

radSoundClipPlayer::radSoundClipPlayer( radMemoryAllocator allocator )
	:
	m_State( NoClip ),
	m_Trim( 1.0f )
{
    ::radSoundHalSystemGet( )->AddRef( );
    
	m_xIRadSoundHalVoice = ::radSoundHalVoiceCreate( GetThisAllocator( ) );
}

//======================================================================
// radSoundClipPlayer::~radSoundClipPlayer
//======================================================================

radSoundClipPlayer::~radSoundClipPlayer( void )
{
	::radSoundHalSystemGet( )->Release( );
}

//======================================================================
// radSoundClipPlayer::SetPriority
//======================================================================

/* virtual */ void radSoundClipPlayer::SetPriority( unsigned int priority )
{
    m_xIRadSoundHalVoice->SetPriority( priority );
}

//======================================================================
// radSoundClipPlayer::GetPriority
//======================================================================

/* virtual */ unsigned int radSoundClipPlayer::GetPriority( void )
{
    return m_xIRadSoundHalVoice->GetPriority( );
}
        
//======================================================================
// radSoundClipPlayer::SetClip
//======================================================================

/* virtual */ void radSoundClipPlayer::SetClip( IRadSoundClip * pIRadSoundClip )
{
	AddRef( );

	StopVoice( );

	SetState( IRadSoundClipPlayer::Stopped );

	m_xIRadSoundHalVoice->SetBuffer( NULL );
	
    RemoveFromUpdateList( );

	m_xRadSoundClip = dynamic_cast< radSoundClip * >( pIRadSoundClip );

	if( m_xRadSoundClip != NULL )
	{
	    rAssert( IRadSoundClip::NoFile != m_xRadSoundClip->GetState( ) );
	    
		if( m_xRadSoundClip->GetState( ) == IRadSoundClip::Initialized )
		{
			SetVoiceBuffer( );
			SetState( IRadSoundClipPlayer::Stopped );
		}
		else
		{
			SetState( IRadSoundClipPlayer::Queueing );
		}
        	
        AddToUpdateList( );
	}
	else
	{
		SetState( IRadSoundClipPlayer::NoClip );
	}

	Release( );
}

//======================================================================
// radSoundClipPlayer::GetClip
//======================================================================

/* virtual */ IRadSoundClip * radSoundClipPlayer::GetClip( void )
{
	return m_xRadSoundClip;
}

//======================================================================
// radSoundClipPlayer::IsPlaying
//======================================================================	

bool radSoundClipPlayer::IsPlaying( void )
{
    return m_State == IRadSoundClipPlayer::Playing ||
           m_State == IRadSoundClipPlayer::QueuedPlay;
}

//======================================================================
// radSoundClipPlayer::Play
//======================================================================

/* virtual */ void radSoundClipPlayer::Play( void )
{
	if
	(
		m_State == IRadSoundClipPlayer::Stopped ||
		m_State == IRadSoundClipPlayer::Queueing ||
		m_State == IRadSoundClipPlayer::QueuedPlay
	)
	{
		if( m_State == IRadSoundClipPlayer::Stopped )
		{
			StartVoice( );
			SetState( IRadSoundClipPlayer::Playing );
		}
		else if( m_State == IRadSoundClipPlayer::Queueing )
		{
			SetState( IRadSoundClipPlayer::QueuedPlay );
		}
	}
}

//======================================================================
// radSoundClipPlayer::Stop
//======================================================================

/* virtual */ void  radSoundClipPlayer::Stop( void )
{
    if ( m_State == IRadSoundClipPlayer::Playing )
    {
        StopVoice( );

        SetState( IRadSoundClipPlayer::Stopped );
    }
    else if ( m_State == IRadSoundClipPlayer::QueuedPlay )
    {
        SetState( IRadSoundClipPlayer::Queueing );
    }
}

void radSoundClipPlayer::SetPlaybackPosition(
			unsigned int position, IRadSoundHalAudioFormat::SizeType st )
{
	IRadSoundHalBuffer * pIRshb = m_xIRadSoundHalVoice->GetBuffer( );

	if ( pIRshb != NULL )
	{
		unsigned int samples = pIRshb->GetFormat( )->ConvertSizeType( IRadSoundHalAudioFormat::Samples,
			position, st );

		m_xIRadSoundHalVoice->SetPlaybackPositionInSamples( samples );
	}
};

//======================================================================
// radSoundClipPlayer::SetState
//======================================================================

void radSoundClipPlayer::SetState( IRadSoundClipPlayer::State state )
{
    m_State = state;
}

//======================================================================
// radSoundClipPlayer::GetState
//======================================================================

/* virtual */ IRadSoundClipPlayer::State radSoundClipPlayer::GetState
(
    void
)
{
    return m_State;
}

//========================================================================
// radSoundClipPlayer::SetTrim
//========================================================================

/* virtual */ void radSoundClipPlayer::SetTrim( float trim )
{
    rAssert( trim >= 0.0f );
    rAssert( trim <= 1.0f );
    
    if ( m_Trim != trim )
    {    
        m_Trim = trim;
        
        if ( m_xRadSoundClip != NULL )
        {    
            m_xIRadSoundHalVoice->SetTrim( m_xRadSoundClip->GetTrim( ) * m_Trim );
        }
    }
}

//========================================================================
// radSoundClipPlayer::GetTrim
//========================================================================

/* virtual */ float radSoundClipPlayer::GetTrim( void )
{
    return m_Trim;
}

//========================================================================
// radSoundClipPlayer::StartVoice
//========================================================================

void radSoundClipPlayer::StartVoice( void )
{
    m_xIRadSoundHalVoice->Play( );
}

//========================================================================
// radSoundClipPlayer::StopVoice
//========================================================================

void radSoundClipPlayer::StopVoice( void )
{
    m_xIRadSoundHalVoice->Stop( );
}

//========================================================================
// radSoundClipPlayer::SetVoiceBuffer
//========================================================================

void radSoundClipPlayer::SetVoiceBuffer( void )
{
    m_xIRadSoundHalVoice->SetBuffer( m_xRadSoundClip->GetBuffer( ) );

    m_xIRadSoundHalVoice->SetTrim( m_Trim * m_xRadSoundClip->GetTrim( ) );

    if ( m_State == IRadSoundClipPlayer::QueuedPlay )
    {
        StartVoice( );

        SetState( IRadSoundClipPlayer::Playing );
    }
    else
    {
        SetState( IRadSoundClipPlayer::Stopped );
    }
}

//========================================================================
// radSoundClipPlayer::OnTimerDone
//========================================================================

/* virtual */ void radSoundClipPlayer::Update( unsigned int elapsedTime ) 
{
    if ( m_State == IRadSoundClipPlayer::QueuedPlay )
    {
        if ( m_xRadSoundClip->GetState( ) == IRadSoundClip::Initialized )
        {
            SetVoiceBuffer( );
        }    
    }
    else if ( m_xIRadSoundHalVoice->IsPlaying( ) == false )
    {
        Stop( );
    }   
}

//========================================================================
// radSoundClipPlayer::SetPitch
//========================================================================

/* virtual */ void radSoundClipPlayer::SetPitch( float pitch )
{
    m_xIRadSoundHalVoice->SetPitch( pitch );
}

//========================================================================
// radSoundClipPlayer::GetPitch
//========================================================================

/* virtual */ float radSoundClipPlayer::GetPitch( void )
{
    return m_xIRadSoundHalVoice->GetPitch( );
}

//========================================================================
// radSoundClipPlayer::SetPan
//========================================================================

/* virtual */ void radSoundClipPlayer::SetPan( float pan )
{
    m_xIRadSoundHalVoice->SetPan( pan );
}

//========================================================================
// radSoundClipPlayer::GetPan
//========================================================================

/* virtual */ float radSoundClipPlayer::GetPan( void )
{
    return m_xIRadSoundHalVoice->GetPan( );
}

//========================================================================
// radSoundStreamPlayer::SetAuxMode
//========================================================================

/* virtual */ void radSoundClipPlayer::SetAuxMode( unsigned int aux, radSoundAuxMode mode )
{
    m_xIRadSoundHalVoice->SetAuxMode( aux, mode );
}
    	
//========================================================================
// radSoundStreamPlayer::GetAuxMode
//========================================================================
    	
/* virtual */ radSoundAuxMode radSoundClipPlayer::GetAuxMode( unsigned int aux )
{
    return m_xIRadSoundHalVoice->GetAuxMode( aux );
}

//========================================================================
// radSoundStreamPlayer::SetAuxGain
//========================================================================

/* virtual */ void radSoundClipPlayer::SetAuxGain( unsigned int aux, float gain )
{
    m_xIRadSoundHalVoice->SetAuxGain( aux, gain );
}

//========================================================================
// radSoundStreamPlayer::GetAuxGain
//========================================================================

/* virtual */ float radSoundClipPlayer::GetAuxGain( unsigned int aux )
{
    return m_xIRadSoundHalVoice->GetAuxGain( aux );
}

//========================================================================
// radSoundClipPlayer::GetPositionInSamples( )
//========================================================================

/* virtual */ unsigned int radSoundClipPlayer::GetPlaybackTimeInSamples( void )
{
	if ( m_xIRadSoundHalVoice != NULL && m_xIRadSoundHalVoice->GetBuffer( ) != NULL )
	{
		return m_xIRadSoundHalVoice->GetPlaybackPositionInSamples( );
	}

	return 0;
}

//========================================================================
// radSoundClipPlayer::SetMuted
//========================================================================

void  radSoundClipPlayer::SetMuted( bool mute )
{
	m_xIRadSoundHalVoice->SetMuted( mute );
}

//========================================================================
// radSoundClipPlayer::GetMuted
//========================================================================

bool  radSoundClipPlayer::GetMuted( void )
{
	return m_xIRadSoundHalVoice->GetMuted( );
}

//========================================================================
// radSoundClipPlayer::SetVolume
//========================================================================

void  radSoundClipPlayer::SetVolume( float volume )
{
	m_xIRadSoundHalVoice->SetVolume( volume );
}

//========================================================================
// radSoundClipPlayer::GetVolume
//========================================================================

float radSoundClipPlayer::GetVolume( void )
{
	return m_xIRadSoundHalVoice->GetVolume( );
}

//========================================================================
// radSoundClipPlayer::SetPositionalGroup
//========================================================================

void radSoundClipPlayer::SetPositionalGroup
(
	IRadSoundHalPositionalGroup * pIrshpg
)
{
	m_xIRadSoundHalVoice->SetPositionalGroup( pIrshpg );
}

//========================================================================
// radSoundClipPlayer::GetPositionalGroup
//========================================================================
	        
IRadSoundHalPositionalGroup * radSoundClipPlayer::GetPositionalGroup( void )
{
	return m_xIRadSoundHalVoice->GetPositionalGroup( );
}

//========================================================================
// radSoundClipPlayer::radSoundClipPlayerCreate
//========================================================================

IRadSoundClipPlayer * radSoundClipPlayerCreate( radMemoryAllocator allocator )
{
	return new ( "radSoundClipPlayer", allocator ) radSoundClipPlayer( allocator );
};

