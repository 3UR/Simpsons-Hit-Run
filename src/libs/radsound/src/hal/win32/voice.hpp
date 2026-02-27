//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================

#ifndef VOICE_HPP
#define VOICE_HPP

//============================================================================
// Interface: Include Files
//============================================================================

#include <radlinkedclass.hpp>
#include <radsound_hal.hpp>
#include <radsoundobject.hpp>
#include "buffer.hpp"
#include "positionalgroup.hpp"

//============================================================================
// Interface: IRadSoundVoiceWin
//============================================================================

class radSoundHalVoiceWin
    :
    public IRadSoundHalVoice,
	public radSoundHalPositionalEntity,
	public radLinkedClass< radSoundHalVoiceWin >,
	public radSoundObject
{

	public:

		virtual void SetPriority( unsigned int priority );
		virtual unsigned int GetPriority();

		virtual void Play();
		virtual void Stop();
		virtual bool IsPlaying();
		virtual unsigned int GetPlaybackPositionInSamples();
		virtual void SetPlaybackPositionInSamples( unsigned int positionInSamples );
		         
		virtual void SetBuffer( IRadSoundHalBuffer * pIRadSoundHalBuffer );
		virtual IRadSoundHalBuffer * GetBuffer();

		virtual void  SetMuted( bool );
		virtual bool  GetMuted();
		virtual void  SetVolume( float );
		virtual float GetVolume();
		virtual void  SetTrim( float );
		virtual float GetTrim(); 
		virtual void  SetPitch( float );
		virtual float GetPitch();
		virtual void  SetPan( float );
		virtual float GetPan();

		virtual void SetAuxMode( unsigned int aux, radSoundAuxMode  mode );
		virtual radSoundAuxMode GetAuxMode( unsigned int aux );
		virtual void SetAuxGain( unsigned int aux, float gain );
		virtual float GetAuxGain( unsigned int aux );

		virtual void SetPositionalGroup( IRadSoundHalPositionalGroup * pIRadSoundHalPositionalGroup );
		virtual IRadSoundHalPositionalGroup * GetPositionalGroup();

		virtual void OnApplyPositionalInfo( float listenerRolloffFactor );
	
		radSoundHalVoiceWin();

	protected:

		virtual ~radSoundHalVoiceWin();

	private:

		IMPLEMENT_REFCOUNTED( "radSoundHalVoiceWin" )

		bool IsHardwarePlaying();

		void SetVolumeInternal();
		void SetPitchInternal();

		unsigned int m_Priority;

		float m_Pitch;
		float m_Volume;
		float m_MuteFactor;
		float m_Trim;

		ALuint m_Source;

		radRef< radSoundHalBufferWin >		m_xRadSoundHalBufferWin;
		radRef< IRadSoundHalAudioFormat >	m_xIRadSoundHalAudioFormat;
		radRef< radSoundHalPositionalGroup > m_xRadSoundHalPositionalGroup;
};

#endif // VOICE_HPP