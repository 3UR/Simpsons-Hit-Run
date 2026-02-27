//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include <radsound_hal.hpp>
#include <radsoundobject.hpp>

struct radSoundHalAudioFormat
	:
	public IRadSoundHalAudioFormat,
	public radSoundObject
{
	IMPLEMENT_REFCOUNTED( "radSoundHalAudioFormat" )

	radSoundHalAudioFormat( void );
	virtual ~radSoundHalAudioFormat( void );
		
	// IRadSoundHalAudioFormat
	
	virtual void Initialize(
		Encoding encoding,
        IRefCount * pIRefCount_CustomInfo,
		unsigned int sampleRate,
		unsigned int channels,
		unsigned int bitResolution );

	virtual Encoding     GetEncoding( void );
	virtual IRefCount *  GetCustomEncodingInfo( void );
	virtual unsigned int GetNumberOfChannels( void );
	virtual unsigned int GetFrameSizeInSamples( void );
	virtual unsigned int GetFrameSizeInBytes( void );
	virtual unsigned int GetSampleRate( void );
	virtual unsigned int GetSampleSizeInBits( void );

	virtual bool Matches( IRadSoundHalAudioFormat * pOther );

	virtual unsigned int SamplesToBytes( unsigned int samples );
	virtual unsigned int SamplesToFrames( unsigned int samples );
	virtual unsigned int SamplesToMilliseconds( unsigned int samples );

	//
	// These functions will round down the bytes value
	// to the nearest multiple of frames, etc.
	//
	virtual unsigned int BytesToSamples( unsigned int bytes );
	virtual unsigned int BytesToFrames( unsigned int bytes );
	virtual unsigned int BytesToMilliseconds( unsigned int bytes );

	virtual unsigned int FramesToSamples( unsigned int frames );
	virtual unsigned int FramesToBytes( unsigned int frames );
	virtual unsigned int FramesToMilliseconds( unsigned int frames );

	virtual unsigned int MillisecondsToBytes( unsigned int ms );
	virtual unsigned int MillisecondsToFrames( unsigned int ms );
	virtual unsigned int MillisecondsToSamples( unsigned int ms );

	virtual unsigned int ConvertSizeType(
		SizeType target, unsigned int size, SizeType source );

	virtual unsigned int GetBitResolution( void );

	// Data members

	Encoding m_Encoding;
	float m_Channels;
	float m_FrameSizeInSamples;
	float m_FrameSizeInBytes;
	float m_SampleRate;
	float m_SampleSizeInBits; // e.g. 16 mono, 32 stereo

	radRef< IRefCount > m_xIRefCount_CustInfo;
};