//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include "pch.hpp"
#include <radsoundmath.hpp> // (Reverse Endian Function)
#include <radsoundfile.hpp>

void radSoundHalFileHeader::ConvertToPlatformEndian
(
	void
)
{	
	// implementation removed
}

IRadSoundHalAudioFormat::Encoding radSoundHalFileHeader::GetEncodingFromChars( char chars[ 4 ] )
{
	if ( radSoundMemCmp( chars, "VAG ", 4 ) )
    {
        return IRadSoundHalAudioFormat::VAG;
    }
	if ( radSoundMemCmp( chars, "PCM ", 4 ) )
    {
        return IRadSoundHalAudioFormat::PCM;
    }
	if ( radSoundMemCmp( chars, "PCMB", 4 ) )
    {
        return IRadSoundHalAudioFormat::PCM_BIGENDIAN;
    }
	if ( radSoundMemCmp( chars, "XADP", 4 ) )
	{
		return 	IRadSoundHalAudioFormat::XBOXADPCM;
	}
	if ( radSoundMemCmp( chars, "GADP", 4 ) )
	{
		return 	IRadSoundHalAudioFormat::GCNADPCM;
	}
    if ( radSoundMemCmp( chars, "RADP", 4 ) )
    {
        return IRadSoundHalAudioFormat::RadicalAdpcm;
    }

	rAssert( 0 );

	return (IRadSoundHalAudioFormat::Encoding) 0xFFFFFFFF;
}

void radSoundHalFileHeader::InitializeAudioFormat
(
	IRadSoundHalAudioFormat * pIRadSoundHalAudioFormat,
    radMemoryAllocator allocator
)
{
	rAssert( pIRadSoundHalAudioFormat != NULL );
	
	IRadSoundHalAudioFormat::Encoding encoding = GetEncodingFromChars( m_SoundDataType );

    radRef< IRefCount > xIRefCount_CustomData = NULL;

	pIRadSoundHalAudioFormat->Initialize( encoding, xIRefCount_CustomData, m_SamplingRate, m_Channels, m_BitResolution );
}
