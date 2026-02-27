//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================

#include "pch.hpp"
#include <radoptions.hpp>
#include <radmovie2.hpp>
#include <radmemorymonitor.hpp>
#include "audiodatasource.hpp"

radMovieAudioDataSource::radMovieAudioDataSource( void )
	:
    radRefCount( 0 ),
    m_Size( 0 ),
	m_FramesLeftInFile( 0 ),
	m_StartPosition( 0 )
{
	m_StateInfo.m_State = StateInfo::NONE;
}

radMovieAudioDataSource::~radMovieAudioDataSource( void )
{
}

IRadSoundHalDataSource::State radMovieAudioDataSource::GetState( void )
{
	if ( m_StateInfo.m_State == StateInfo::IDLE ||
	     m_StateInfo.m_State == StateInfo::READING_DATA )
	{
		return IRadSoundHalDataSource::Initialized;
	}
	else if ( m_StateInfo.m_State == StateInfo::FILE_ERROR )
	{
		return IRadSoundHalDataSource::Error;
	}
	
	return IRadSoundHalDataSource::Initializing;
}

void radMovieAudioDataSource::Initialize
( 
	IRadFile * pIRadFile,
	unsigned int startPosition,
	unsigned int size
)
{
	rAssert( m_StateInfo.m_State == StateInfo::NONE );
	rAssert( pIRadFile != NULL );

    m_xIRadSoundHalAudioFormat = NULL;
    m_StartPosition = startPosition;
    m_Size = size;
	m_xIRadFile = pIRadFile;

	//
	// We have a file that may or may not be open
	// We'll get the file system to call us back 
	// when the file is open and this will start our
	// state machine
	//

	m_StateInfo.m_State = StateInfo::OPENING_FILE;
    m_xIRadFile->AddCompletionCallback( this, NULL );
}

IRadSoundHalAudioFormat * radMovieAudioDataSource::GetFormat( void )
{
    rAssert( m_StateInfo.m_State == StateInfo::IDLE || m_StateInfo.m_State == StateInfo::READING_DATA );
    rAssert( m_xIRadSoundHalAudioFormat != NULL );

	return m_xIRadSoundHalAudioFormat;
}

unsigned int radMovieAudioDataSource::GetRemainingFrames( void )
{
    rAssert( m_StateInfo.m_State == StateInfo::IDLE || m_StateInfo.m_State == StateInfo::READING_DATA );
	return m_FramesLeftInFile;
}

void radMovieAudioDataSource::GetFramesAsync
(
	void * pFrameBuffer,
	radMemorySpace destinationMemorySpace,
	unsigned int sizeInFrames,
	IRadSoundHalDataSourceCallback * pIRshdsc
)
{
    rAssert( m_StateInfo.m_State == StateInfo::IDLE );
	rAssert( pIRshdsc != NULL );

	if ( m_FramesLeftInFile < sizeInFrames )
	{
		sizeInFrames = m_FramesLeftInFile;
	}

	if( sizeInFrames > 0 && m_StateInfo.m_State == StateInfo::IDLE )
	{
		m_StateInfo.m_State = StateInfo::READING_DATA;

		// Get a read going

		m_StateInfo.m_ReadingDataInfo.m_ReadSizeInFrames = sizeInFrames;
		m_xIRadSoundHalDataSourceCallback = pIRshdsc;

		// Figure out how many bytes we'll require
		// (And verify that our conversion is happening correctly)

		unsigned int bytes = m_xIRadSoundHalAudioFormat->FramesToBytes( sizeInFrames );

		m_xIRadFile->ReadAsync(pFrameBuffer, bytes, destinationMemorySpace );
		m_xIRadFile->AddCompletionCallback( this, NULL );
	}
	else
	{
		// Don't call us until we're ready
		rWarningMsg( m_StateInfo.m_State == StateInfo::IDLE, "FileDataSource: Get Frames called while not in the IDLE state" );  

		pIRshdsc->OnDataSourceFramesLoaded( 0 );
	}
}

void radMovieAudioDataSource::OnFileOperationsComplete( void* pUserData )
{
	StateInfo::State state = m_StateInfo.m_State;

	switch ( m_StateInfo.m_State )
	{
		case StateInfo::NONE:
		{
			// do nothing
			break;
		}
		case StateInfo::OPENING_FILE:
		{
			_StateOpeningFile( );
			break;
		}
		case StateInfo::READING_HEADER:
		{
			_StateReadingHeader( );
			break;
		}
		case StateInfo::IDLE:
		{
			_StateIdle( );
			break;
		}
		case StateInfo::READING_DATA:
		{
			_StateReadingData( );
			break;
		}
        default:
        {
            break;
        }
	};
}

void radMovieAudioDataSource::_StateOpeningFile( void )
{
	if( m_xIRadFile->IsOpen( ) == true )
	{
		//
		// The file was opened correctly.  We can read in the
		// header and find out what kind of a file this is.
		//

		m_StateInfo.m_State = StateInfo::READING_HEADER;

		// Prepare a temporary structure to hold the header info

		m_StateInfo.m_ReadingHeaderInfo.m_pRadSoundHalFileHeader =
			static_cast< radSoundHalFileHeader * >(
				::radMemoryAllocAligned(
					RADMEMORY_ALLOC_TEMP,
					::radMemoryRoundUp( sizeof( radSoundHalFileHeader ), radFileMaxSectorSize ),
					radFileOptimalMemoryAlignment ) );

        ::radMemoryMonitorIdentifyAllocation(
            m_StateInfo.m_ReadingHeaderInfo.m_pRadSoundHalFileHeader,
            radMovieDebugChannel2,
            "radMovieAudioDataSource::m_StateInfo.m_ReadingHeaderInfo.m_pRadSoundHalFileHeader" );                    

		// Set the read position to the start of the file and
		// read in the header
		
		m_xIRadFile->SetPositionAsync( m_StartPosition );
		m_xIRadFile->ReadAsync(
			m_StateInfo.m_ReadingHeaderInfo.m_pRadSoundHalFileHeader,
			sizeof( radSoundHalFileHeader ) );
		
		m_xIRadFile->AddCompletionCallback( this, NULL );
	}
	else
	{
		rDebugPrintf( "Failed to open file: [%s]\n", m_xIRadFile->GetFilename( ) );

		//
		// The file system didn't find our file.  Say that the
		// size of the file is zero and move into the idle state.
		// We don't need to fail because of this problem.
		//

		m_StateInfo.m_State = StateInfo::FILE_ERROR;
		m_FramesLeftInFile = 0;
		m_xIRadFile = NULL;
	}
}

void radMovieAudioDataSource::_StateReadingHeader( void )
{
	rAssert(  m_xIRadFile->IsOpen( ) );  // If the file isn't open, we shouldn't be here

	// Get the data under control for strange platforms

	m_StateInfo.m_ReadingHeaderInfo.m_pRadSoundHalFileHeader->ConvertToPlatformEndian( );

	// Create a new audio format object

	m_xIRadSoundHalAudioFormat = ::radSoundHalAudioFormatCreate( GetThisAllocator( ) );

	m_StateInfo.m_ReadingHeaderInfo.m_pRadSoundHalFileHeader->InitializeAudioFormat(
		m_xIRadSoundHalAudioFormat, GetThisAllocator( ) );
	
	// Release the tempory memory

	::radMemoryFreeAligned( RADMEMORY_ALLOC_TEMP, m_StateInfo.m_ReadingHeaderInfo.m_pRadSoundHalFileHeader );
	m_StateInfo.m_ReadingHeaderInfo.m_pRadSoundHalFileHeader = NULL;

    InitFile( );
}

void radMovieAudioDataSource::InitFile( void )
{
	// Calculate the amount of data contained in the file

	m_FramesLeftInFile = m_xIRadSoundHalAudioFormat->BytesToFrames(
		m_Size - RSD_FILE_DATA_OFFSET );

	//
	// An important assertion: The filesize needs to be a multiple of the framesize
	//
	unsigned int fileDataSize = m_Size - RSD_FILE_DATA_OFFSET;
	unsigned int roundedBytes = m_xIRadSoundHalAudioFormat->FramesToBytes( m_FramesLeftInFile );
	rAssertMsg( fileDataSize == roundedBytes, "File Data is not block aligned (corrupted )" );

	// Move into the idle state before possibly getting called back

	m_StateInfo.m_State = StateInfo::IDLE;

	// Skip to the data and include the clients requested start position

	m_xIRadFile->SetPositionAsync( RSD_FILE_DATA_OFFSET + m_StartPosition );

    // Bit of a hack here, service the sound system so streamers pool the IsInitialized()
    // flag right now.

    ::radSoundHalSystemGet( )->Service( );
}

void radMovieAudioDataSource::_StateIdle( void )
{
}

void radMovieAudioDataSource::_StateReadingData( void )
{
	// Update our position info and callback the client

	m_FramesLeftInFile -= m_StateInfo.m_ReadingDataInfo.m_ReadSizeInFrames;
	m_StateInfo.m_State = StateInfo::IDLE;

	radRef< IRadSoundHalDataSourceCallback > xIRadSoundHalDataSourceCallback = m_xIRadSoundHalDataSourceCallback;
	m_xIRadSoundHalDataSourceCallback = NULL;

	xIRadSoundHalDataSourceCallback->OnDataSourceFramesLoaded( m_StateInfo.m_ReadingDataInfo.m_ReadSizeInFrames );
}

radMovieAudioDataSource * radMovieAudioDataSourceCreate( radMemoryAllocator allocator )
{
	return new ( allocator ) radMovieAudioDataSource( );
}