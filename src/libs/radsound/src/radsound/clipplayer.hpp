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

#include <radsound_hal.hpp>
#include <radsound.hpp>
#include "clip.hpp"
#include "radsoundupdatableobject.hpp"

//======================================================================
// Class: radSoundClipPlayer
//======================================================================

class radSoundClipPlayer
	:
	public IRadSoundClipPlayer,
	public radSoundUpdatableObject
{
    public:

        // IRefCount

	    IMPLEMENT_REFCOUNTED( "radSoundClipPlayer" )

        // IRadSoundClipPlayer

		virtual void SetPlaybackPosition(
			unsigned int position, IRadSoundHalAudioFormat::SizeType st );

        virtual void SetPriority( unsigned int priority );
        virtual unsigned int GetPriority( void );
        
	    virtual void SetClip( IRadSoundClip * pIRadSoundClip );
	    virtual IRadSoundClip * GetClip( void );

	    virtual void Play( void );
	    virtual bool IsPlaying( void );
        virtual void Stop( void );
        virtual State GetState( void );

         
        virtual void  SetMuted( bool mute );
        virtual bool  GetMuted( void );
        virtual void  SetVolume( float volume );
        virtual float GetVolume( void );
        virtual void  SetTrim( float trim );
        virtual float GetTrim( void );
        virtual void  SetPitch( float pitch );
        virtual float GetPitch( void );
        virtual void  SetPan( float pan );
        virtual float GetPan( void );        

    	virtual void  SetAuxMode( unsigned int aux, radSoundAuxMode mode );
    	virtual radSoundAuxMode GetAuxMode( unsigned int aux );

    	virtual void  SetAuxGain( unsigned int aux, float gain );
    	virtual float GetAuxGain( unsigned int aux );

		virtual void SetPositionalGroup( IRadSoundHalPositionalGroup * pIrshpg );	        
		virtual IRadSoundHalPositionalGroup * GetPositionalGroup( void );

		virtual unsigned int GetPlaybackTimeInSamples( void );

    private: 

    	radSoundClipPlayer( radMemoryAllocator allocator );
	    ~radSoundClipPlayer( void );

        void StartVoice( void );
        void StopVoice( void );
        void SetVoiceBuffer( void );
	    void SetState( IRadSoundClipPlayer::State state );
    
        // IRadTimerCallback

	    virtual void Update( unsigned int elapsedTime );

        // Data members

	    IRadSoundClipPlayer::State m_State;
	    
	    float m_Trim;

	    radRef< radSoundClip >         m_xRadSoundClip;
	    radRef< IRadSoundHalVoice > 	m_xIRadSoundHalVoice;

    friend IRadSoundClipPlayer * radSoundClipPlayerCreate( radMemoryAllocator allocator );
};

