//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        radmoviefile.hpp
//
// Subsystem:	Foundation Technologies - Movie Player
//
// Description:	This file contains header declarations for the FTech movie
//              file format
//
// Date:    	May 16, 2002
//
//=============================================================================

#ifndef RADMOVIEFILE_HPP
#define RADMOVIEFILE_HPP

//=============================================================================
// Definitions
//=============================================================================

#define RAD_MOVIE_FILE_MAX_AUDIO_TRACKS    16
#define RAD_MOVIE_FILE_DATA_POSITION       2048

//=============================================================================
// struct RadMovieFileHeader
//=============================================================================

struct RadMovieFileHeader
{
    char m_Id[ 8 ];  // 'r' 'm' 'v' 'p' 's' '2' '1' '2'
                     // 'r' 'm' 'v' 'g' 'c' 'n' '1' '0'
                     // 'r' 'm' 'v' 'x' 'b' 'x' '1' '0'
                     // 'r' 'm' 'v' 'p' 'c' 'c' '1' '0'
    unsigned short m_Width;
    unsigned short m_Height;
    unsigned short m_NumFrames;
    unsigned int   m_DataSize;
    unsigned int   m_DataPosition;  // Relative to beginning of file
    unsigned int   m_NumAudioTracks;
    unsigned int   m_pAudioTrackOffsets[ RAD_MOVIE_FILE_MAX_AUDIO_TRACKS ];
    unsigned int   m_pAudioTrackSizes[ RAD_MOVIE_FILE_MAX_AUDIO_TRACKS ];
    unsigned int   m_FrameRate; // in frames per Hectoseconds (i.e. frames / 100 seconds)
};

#endif // RADMOVIEFILE_HPP