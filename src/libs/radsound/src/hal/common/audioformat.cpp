//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include "pch.hpp"
#include "audioformat.hpp"
#include "../../radsound/radicaladpcm.hpp"

//
// XADPCM 36 byte block (36*2 stereo) ( 64 samples/block ) Compression ratio: 9/32
//

radSoundHalAudioFormat::radSoundHalAudioFormat( void )
	:
	m_Encoding( (IRadSoundHalAudioFormat::Encoding) 0xFFFFFFFF ),
	m_Channels( 0 ),
	m_FrameSizeInSamples( 0 ),
	m_FrameSizeInBytes( 0 ),
	m_SampleRate( 0 ),
	m_SampleSizeInBits( 0 )
{
}

radSoundHalAudioFormat::~radSoundHalAudioFormat( void )
{
}
	
void radSoundHalAudioFormat::Initialize(
	Encoding encoding,
    IRefCount * pIRefCount_CustomInfo,
    unsigned int sampleRate,
	unsigned int channels,
	unsigned int bitResolution )
{
	m_Encoding  = encoding;
	m_Channels  = ::radSoundUIntToFloat( channels );
	m_SampleRate = ::radSoundUIntToFloat( sampleRate );

    m_xIRefCount_CustInfo = pIRefCount_CustomInfo;

	if ( m_Encoding == IRadSoundHalAudioFormat::PCM ||
	     m_Encoding == IRadSoundHalAudioFormat::PCM_BIGENDIAN )
	{
		m_FrameSizeInSamples = 1;
		m_FrameSizeInBytes = ::radSoundUIntToFloat( (bitResolution / 8 ) * channels );
		m_SampleSizeInBits = ::radSoundUIntToFloat( bitResolution * channels );
	}
	else if ( m_Encoding == IRadSoundHalAudioFormat::XBOXADPCM )
	{
		m_FrameSizeInBytes =  ::radSoundUIntToFloat( 36 * channels );
		m_FrameSizeInSamples = 64.0f;
		m_SampleSizeInBits = ::radSoundUIntToFloat( bitResolution * channels );
	}
	else if ( m_Encoding == IRadSoundHalAudioFormat::RadicalAdpcm )
	{

	    m_FrameSizeInBytes = radSoundUIntToFloat( RADICAL_ADPCM_FRAME_SIZE * channels );
	    m_FrameSizeInSamples = RADICAL_ADPCM_SAMPLES_PER_FRAME;
	    m_SampleSizeInBits = ::radSoundUIntToFloat( bitResolution * channels );
	}
}

IRadSoundHalAudioFormat::Encoding radSoundHalAudioFormat::GetEncoding( void )
{
	return m_Encoding;
}

IRefCount * radSoundHalAudioFormat::GetCustomEncodingInfo( void )
{
	return m_xIRefCount_CustInfo;
}

unsigned int radSoundHalAudioFormat::GetNumberOfChannels( void )
{
	return ::radSoundFloatToUInt( m_Channels );
}

unsigned int radSoundHalAudioFormat::GetFrameSizeInSamples( void )
{
	return ::radSoundFloatToUInt( m_FrameSizeInSamples );
}

unsigned int radSoundHalAudioFormat::GetFrameSizeInBytes( void )
{
	return ::radSoundFloatToUInt( m_FrameSizeInBytes );
}

unsigned int radSoundHalAudioFormat::GetSampleRate( void )
{
	return ::radSoundFloatToUInt( m_SampleRate );
}

unsigned int radSoundHalAudioFormat::GetSampleSizeInBits( void )
{
	return ::radSoundFloatToUInt( m_SampleSizeInBits );
}

bool radSoundHalAudioFormat::Matches( IRadSoundHalAudioFormat * pOther )
{
	rAssert( pOther != NULL );
	if
	(
		m_Encoding != pOther->GetEncoding( ) ||
		m_Channels != pOther->GetNumberOfChannels( ) ||
		m_FrameSizeInSamples != pOther->GetFrameSizeInSamples( ) ||
		m_FrameSizeInBytes != pOther->GetFrameSizeInBytes( ) ||
		m_SampleRate != pOther->GetSampleRate( ) ||
		m_SampleSizeInBits != pOther->GetSampleSizeInBits( )
	)
	{
		return false;
	}
	else
	{
		return true;
	}
}

unsigned int radSoundHalAudioFormat::SamplesToBytes( unsigned int samples )
{
    float fSamples = ::radSoundUIntToFloat( samples );

	float fBytes = ( fSamples  * m_FrameSizeInBytes ) / m_FrameSizeInSamples;

    unsigned int bytes = ::radSoundFloatToUInt( fBytes );

	return bytes;
}

unsigned int radSoundHalAudioFormat::BytesToSamples( unsigned int bytes )
{
    float fBytes = ::radSoundUIntToFloat( bytes );

	float fSamples = ( fBytes * m_FrameSizeInSamples ) / m_FrameSizeInBytes;

    unsigned int samples = ::radSoundFloatToUInt( fSamples );

	return samples;
}

unsigned int radSoundHalAudioFormat::SamplesToFrames( unsigned int samples )
{
    float fSamples = ::radSoundUIntToFloat( samples );

	float fFrames = fSamples / m_FrameSizeInSamples;

    unsigned int frames = ::radSoundFloatToUInt( fFrames );

	return frames;
}

unsigned int radSoundHalAudioFormat::FramesToSamples( unsigned int frames )
{
    float fFrames = ::radSoundUIntToFloat( frames );

    float fSamples = fFrames * m_FrameSizeInSamples;

    unsigned int samples = ::radSoundFloatToUInt( fSamples );

	return samples;
}

unsigned int radSoundHalAudioFormat::FramesToBytes( unsigned int frames )
{
    float fFrames = ::radSoundUIntToFloat( frames );

    float fBytes = fFrames * m_FrameSizeInBytes;

    unsigned int bytes = ::radSoundFloatToUInt( fBytes );

	return bytes;
}

unsigned int radSoundHalAudioFormat::BytesToFrames( unsigned int bytes )
{
    float fBytes = ::radSoundUIntToFloat( bytes );

    float fFrames = fBytes / m_FrameSizeInBytes;

    unsigned int frames = ::radSoundFloatToUInt( fFrames );

	return frames;
}

unsigned int radSoundHalAudioFormat::GetBitResolution( void )
{
	return ::radSoundFloatToUInt( m_SampleSizeInBits / m_Channels );
}

unsigned int radSoundHalAudioFormat::ConvertSizeType
(
	SizeType target,
	unsigned int size,
	SizeType source
)
{
	if ( source == IRadSoundHalAudioFormat::Bytes )
	{
		if ( target == IRadSoundHalAudioFormat::Bytes )
		{
			return size;
		}
		if ( target == IRadSoundHalAudioFormat::Frames )
		{
			return BytesToFrames( size );
		}
		else if ( target == IRadSoundHalAudioFormat::Samples )
		{
			return BytesToSamples( size );
		}
		else if ( target == IRadSoundHalAudioFormat::Milliseconds )
		{
			return BytesToMilliseconds( size );
		}
	}
	else if ( source == IRadSoundHalAudioFormat::Frames )
	{
		if ( target == IRadSoundHalAudioFormat::Frames )
		{
			return size;
		}
		else if ( target == IRadSoundHalAudioFormat::Bytes )
		{
			return FramesToBytes( size );
		}
		else if ( target == IRadSoundHalAudioFormat::Samples )
		{
			return FramesToSamples( size );
		}
		else if ( target == IRadSoundHalAudioFormat::Milliseconds )
		{
			return FramesToMilliseconds( size );
		}
	}
	else if ( source == IRadSoundHalAudioFormat::Samples )
	{
		if ( target == IRadSoundHalAudioFormat::Samples )
		{
			return size;
		}
		else if ( target == IRadSoundHalAudioFormat::Bytes )
		{
			return SamplesToBytes( size );
		}
		else if ( target == IRadSoundHalAudioFormat::Frames )
		{
			return SamplesToFrames( size );
		}
		else if ( target == IRadSoundHalAudioFormat::Milliseconds )
		{
			return SamplesToMilliseconds( size );
		}
	}
	else if ( source == IRadSoundHalAudioFormat::Milliseconds )
	{
		if ( target == IRadSoundHalAudioFormat::Milliseconds )
		{
			return size;
		}
		if ( target == IRadSoundHalAudioFormat::Bytes )
		{
			return MillisecondsToBytes( size );
		}
		else if ( target == IRadSoundHalAudioFormat::Frames )
		{
			return MillisecondsToFrames( size );
		}
		else if ( target == IRadSoundHalAudioFormat::Samples )
		{
			return MillisecondsToSamples( size );
		}
	}

	rAssert( 0 );

	return 0;

}

unsigned int radSoundHalAudioFormat::BytesToMilliseconds( unsigned int bytes )
{
    float fBytes = ::radSoundUIntToFloat( bytes );

    float fFrames = fBytes / m_FrameSizeInBytes;

    float fSamples = fFrames * m_FrameSizeInSamples;

    float fMs = ( fSamples / m_SampleRate ) * 1000.0f;

	unsigned ms = ::radSoundFloatToUInt( fMs );

    return ms;
}
unsigned int radSoundHalAudioFormat::MillisecondsToBytes( unsigned int ms )
{
    float fMs = ::radSoundUIntToFloat( ms );

    float fSamples = ( fMs / 1000.0f ) * m_SampleRate;

    float fFrames = fSamples / m_FrameSizeInSamples;

    float fBytes = fFrames * m_FrameSizeInBytes;

    unsigned int bytes = ::radSoundFloatToUInt( fBytes );

	return bytes;
}

unsigned int radSoundHalAudioFormat::SamplesToMilliseconds( unsigned int samples )
{
    float fSamples = ::radSoundUIntToFloat( samples );

    float fMs = ( fSamples / m_SampleRate ) * 1000.0f;

    unsigned int ms = ::radSoundFloatToUInt( fMs );

	return ms;
}

unsigned int radSoundHalAudioFormat::FramesToMilliseconds( unsigned int frames )
{
    float fFrames = ::radSoundUIntToFloat( frames );

    float fSamples = fFrames * m_FrameSizeInSamples;

    float fMs = ( fSamples / m_SampleRate ) * 1000.0f;

    unsigned ms = ::radSoundFloatToUInt( fMs );

	return ms;
}

unsigned int radSoundHalAudioFormat::MillisecondsToFrames( unsigned int ms )
{
    float fMs = ::radSoundUIntToFloat( ms );
    
    float fSamples = ( fMs / 1000.0f ) * m_SampleRate;

    float fFrames = fSamples / m_FrameSizeInSamples;

    unsigned int frames = ::radSoundFloatToUInt( fFrames );

	return frames;
}

unsigned int radSoundHalAudioFormat::MillisecondsToSamples( unsigned int ms )
{
    float fMs = ::radSoundUIntToFloat( ms );

    float fSamples = ( fMs / 1000.0f ) * m_SampleRate;

    unsigned int samples = ::radSoundFloatToUInt( fSamples );

	return samples;
}

IRadSoundHalAudioFormat * radSoundHalAudioFormatCreate( radMemoryAllocator allocator )
{
	return new ( "radSoundHalAudioFormat", allocator ) radSoundHalAudioFormat;
}
