//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//============================================================================
// File: radsoundfile.hpp
//
// Description: This file contains the definition of the .rsd file header.
//============================================================================

#ifndef RADSOUNDFILE_HPP
#define RADSOUNDFILE_HPP

//=============================================================================
// Build Configuration Check
//=============================================================================

#if !defined(RAD_UWP) && !defined(RAD_WIN32)
    #error 'FTech requires definition of RAD_UWP or RAD_WIN32'
#endif

//=============================================================================
// Include Files
//=============================================================================

#include <radsound_hal.hpp>

//=============================================================================
// Types
//=============================================================================

#define RADSOUNDHAL_FILE_VERSION "RSD4"

//=============================================================================
// Struct: radSoundAdpcmInfoGcn
//
// Description: This structure contains the sample-specific coefficient and
//		loop-point information for gamecube ADPCM data.
//=============================================================================

#define GCN_ADPCM_MAX_CHANNELS      2       // We only support mono/stereo for now
#define GCN_ADPCM_SAMPLES_PER_FRAME 14      // GCN DSP-ADPCM has 14 sample/frame
#define GCN_ADPCM_BYTES_PER_FRAME   8       // GCN DSP-ADPCM has 8 bytes/frame
#define GCN_ADPCM_FRAME_HEADER_SIZE 2       // GCN DSP-ADPCM frame header is 2 nibble

struct radSoundHalAdpcmInfoGcn
{
    // start context
    unsigned short coef[8][2];
    unsigned short gain;
    unsigned short pred_scale;
    short yn1;
    short yn2;
    // loop context
    unsigned short loop_pred_scale;
    short loop_yn1;
    short loop_yn2;
};

//=============================================================================
// Struct: RsdHeader
//
// Description: This the fixed-size file header of an .rsd file.  Use
//		wavtorsd.exe to build .rsd files.
//=============================================================================

struct radSoundHalFileHeader
{
    char m_RsdDataTag[4];    // RADSOUND_HAL_FILEVERSION
    char m_SoundDataType[4]; // "VAG ", "PCM ", "GADP", "XADP"
    unsigned int m_Channels;
    unsigned int m_BitResolution;
    unsigned int m_SamplingRate;

    union
    {
        radSoundHalAdpcmInfoGcn m_gcnAdpcmInfo[ GCN_ADPCM_MAX_CHANNELS ];
        char m_Padding[ sizeof( radSoundHalAdpcmInfoGcn) * GCN_ADPCM_MAX_CHANNELS ]; // Pad other platforms
    };

    // Round the size of this structure to 128 bytes

    char m_ReservedForFutureUse[ 16 ];

	// Helper functions

	void ConvertToPlatformEndian( void );

	// Initializes an audio format component based on this file header.

	void InitializeAudioFormat( IRadSoundHalAudioFormat * pIRadSoundHalAudioFormat,
        radMemoryAllocator allocatorCustomInfo );

    static IRadSoundHalAudioFormat::Encoding GetEncodingFromChars( char chars[ 4 ] );
};

#define RSD_FILE_DATA_OFFSET  2048 // sector size

#endif // RSDFILE_HPP