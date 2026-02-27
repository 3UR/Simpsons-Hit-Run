//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include "pch.hpp"
#include <radsoundfile.hpp>
#include "rsdfiledatasource.hpp"

RadSoundFilePerformanceCallback * gpFpc;

void Log(
    bool start,
    const char * pFile,
    unsigned int bytes )
{
    if ( gpFpc )
    {
        gpFpc( start, pFile, bytes );
    }
}    
    
void RadSoundSetFilePerformanceCallback( RadSoundFilePerformanceCallback * pCallback )
{
    gpFpc = pCallback;
}


radSoundRsdFileDataSource::radSoundRsdFileDataSource( void )
{
	m_StateInfo.m_State = StateInfo::NONE;
}

radSoundRsdFileDataSource::~radSoundRsdFileDataSource( void )
{
}

IRadSoundHalDataSource::State radSoundRsdFileDataSource::GetState( void )
{
    if ( m_StateInfo.m_State == StateInfo::INITIALIZED )
    {
        if ( m_refIRadFile == NULL )
        {            
	        ::radFileOpen(
	            & m_refIRadFile,
	            fileName,
	            false,
	            OpenExisting,
                NormalPriority, // m_StateInfo.m_InitializedInfo.m_HighPriority ? HighPriority : NormalPriority,
                0,
                RADMEMORY_ALLOC_TEMP,
                radMemorySpace_Local );
        }
        	    
	    m_StateInfo.m_State = StateInfo::OPENING_FILE;
	     
        m_refIRadFile->AddCompletionCallback( this, NULL );
 
    }
    else if (
        m_StateInfo.m_State == StateInfo::IDLE ||
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

void radSoundRsdFileDataSource::InitializeFromFile
( 
	IRadFile * pIRadFile,
	unsigned int initialPlaybackPosition,
	IRadSoundHalAudioFormat::SizeType positionType,
    IRadSoundHalAudioFormat * pIRadSoundHalAudioFormat     
)
{
	rAssert( m_StateInfo.m_State == StateInfo::NONE );
	rAssert( pIRadFile != NULL );

    m_refIRadSoundHalAudioFormat = pIRadSoundHalAudioFormat;

    strcpy( fileName, "" );
    
	// We can't convert these values until we an audio format

    m_StateInfo.m_State = StateInfo::INITIALIZED;
    m_StateInfo.m_InitializedInfo.m_InitialPlaybackPos = initialPlaybackPosition;
    m_StateInfo.m_InitializedInfo.m_InitalPlaybackPosUnits = positionType;    
    m_StateInfo.m_InitializedInfo.m_HighPriority = true;
	//
	// We have a file that may or may not be open
	// We'll get the file system to call us back 
	// when the file is open and this will start our
	// state machine
	//

	m_refIRadFile = pIRadFile;

}

void radSoundRsdFileDataSource::InitializeFromFileName
( 
	const char * pFileName,
    bool highPriority,
	unsigned int initialPlaybackPosition,
	IRadSoundHalAudioFormat::SizeType positionType,
    IRadSoundHalAudioFormat * pIRadSoundHalAudioFormat
)
{
    rAssert( pFileName != NULL );
	rAssert( m_StateInfo.m_State == StateInfo::NONE );

    m_refIRadSoundHalAudioFormat = pIRadSoundHalAudioFormat;
    
    strncpy( fileName, pFileName, 64 );
			
	m_StateInfo.m_State = StateInfo::INITIALIZED;
	m_StateInfo.m_InitializedInfo.m_InitalPlaybackPosUnits = positionType;
	m_StateInfo.m_InitializedInfo.m_InitialPlaybackPos = initialPlaybackPosition;
	m_StateInfo.m_InitializedInfo.m_HighPriority = highPriority;
};

IRadSoundHalAudioFormat * radSoundRsdFileDataSource::GetFormat( void )
{
    rAssert( m_StateInfo.m_State == StateInfo::IDLE || m_StateInfo.m_State == StateInfo::READING_DATA );
    rAssert( m_refIRadSoundHalAudioFormat != NULL );

	return m_refIRadSoundHalAudioFormat;
}

unsigned int radSoundRsdFileDataSource::GetRemainingFrames( void )
{
    rAssert( m_StateInfo.m_State == StateInfo::IDLE || m_StateInfo.m_State == StateInfo::READING_DATA );
	return m_FramesLeftInFile;
}

void radSoundRsdFileDataSource::GetFramesAsync
(
	void * pFrameBuffer,
	radMemorySpace destinationMemorySpace,
	unsigned int sizeInFrames,
	IRadSoundHalDataSourceCallback * pIRshdsc
)
{
    rAssert( m_StateInfo.m_State == StateInfo::IDLE );
	rAssert( pIRshdsc != NULL );

    /* rDebugPrintf( "FDS: [%s]\n\tthis: [0x%x] Request for [0x%x] frames from: [0x%x] @ [0x%x]\n",
        this->fileName,
        this,
        sizeInFrames,
        pIRshdsc,
        pFrameBuffer ); */
        
	if ( m_FramesLeftInFile < sizeInFrames )
	{
		sizeInFrames = m_FramesLeftInFile;
	}

	m_StateInfo.m_State = StateInfo::READING_DATA;

	// Get a read going

	m_StateInfo.m_ReadingDataInfo.m_ReadSizeInFrames = sizeInFrames;
	m_refIRadSoundHalDataSourceCallback = pIRshdsc;

	// Figure out how many bytes we'll require
	// (And verify that our conversion is happening correctly)

	unsigned int bytes = m_refIRadSoundHalAudioFormat->FramesToBytes( sizeInFrames );
    
    /* if ( ((unsigned int)pFrameBuffer % radSoundStreamReadAlignmentGet( ) ) != 0 )
    {
        rDebugPrintf( "radSoundRsdFileDataSource: reading non-optimal alignment!\n" );
    } */

    // unsigned int optimalFrameReadMultiple = m_refIRadSoundHalAudioFormat->BytesToFrames( radSoundStreamReadMultipleGet( ) );

    /* if ( m_FramesLeftInFile > sizeInFrames )
    {
        if ( (sizeInFrames % optimalFrameReadMultiple ) != 0 )
        {
            rDebugPrintf( "radSoundRsdFileDataSource:: reading non-optimal size!\n" );
        }
    } */
    
    Log( true, this->fileName, bytes );
    
	m_refIRadFile->ReadAsync(pFrameBuffer, bytes, destinationMemorySpace );
	m_refIRadFile->AddCompletionCallback( this, NULL );

}

void radSoundRsdFileDataSource::OnFileOperationsComplete( void* pUserData )
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
            Log( false, this->fileName, 0 );		
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
            Log( false, this->fileName, 0 );		
			_StateReadingData( );
			break;
		}
        default:
        {
            rAssert( false );
        }
	};
}

void radSoundRsdFileDataSource::_StateOpeningFile( void )
{
	if( m_refIRadFile->IsOpen( ) == true )
	{
        m_refIRadFile->SetBufferedRead( IRadFile::BufferedReadOn );
        if ( m_refIRadSoundHalAudioFormat == NULL )
        {
            rDebugPrintf(
                "radSoundRsdFileDataSource: WARNING reading header for: [%s], extra seek incurred\n",
                fileName );
               
		    //
		    // The file was opened correctly.  We can read in the
		    // header and find out what kind of a file this is.
		    //

            unsigned int initialPlaybackPos = m_StateInfo.m_InitializedInfo.m_InitialPlaybackPos;
            IRadSoundHalAudioFormat::SizeType initialPlaybackPosUnits
                = m_StateInfo.m_InitializedInfo.m_InitalPlaybackPosUnits;
            
		    m_StateInfo.m_State = StateInfo::READING_HEADER;
            m_StateInfo.m_ReadingHeaderInfo.m_InitialPlaybackPos = initialPlaybackPos;
            m_StateInfo.m_ReadingHeaderInfo.m_InitalPlaybackPosUnits = initialPlaybackPosUnits;
            
		    // Prepare a temporary structure to hold the header info

		    m_StateInfo.m_ReadingHeaderInfo.m_pRadSoundHalFileHeader =
			    static_cast< radSoundHalFileHeader * >(
				    ::radMemoryAllocAligned(
					    RADMEMORY_ALLOC_TEMP,
					    ::radMemoryRoundUp( sizeof( radSoundHalFileHeader ), radFileMaxSectorSize ),
					    radFileOptimalMemoryAlignment ) );

            ::radMemoryMonitorIdentifyAllocation(
                m_StateInfo.m_ReadingHeaderInfo.m_pRadSoundHalFileHeader,
                radSoundDebugChannel,
                "radSoundRsdFileDataSource::m_StateInfo.m_ReadingHeaderInfo.m_pRadSoundHalFileHeader" );                    

		    // Set the read position to the start of the file and
		    // read in the header
		    

            Log( true, this->fileName, sizeof( radSoundHalFileHeader ) );
    		    
		    m_refIRadFile->SetPositionAsync( 0 );
		    m_refIRadFile->ReadAsync(
			    m_StateInfo.m_ReadingHeaderInfo.m_pRadSoundHalFileHeader,
			    sizeof( radSoundHalFileHeader ) );
		    
		    m_refIRadFile->AddCompletionCallback( this, NULL );
        }
        else
        {
            // We already know the format (was passed in from client), just
            // go directly to setting things up

            InitFile( );
        }
	}
	else
	{
		rTunePrintf( "Failed to open file: [%s]\n", m_refIRadFile->GetFilename( ) );

		//
		// The file system didn't find our file.  Say that the
		// size of the file is zero and move into the idle state.
		// We don't need to fail because of this problem.
		//

		m_StateInfo.m_State = StateInfo::FILE_ERROR;
		m_FramesLeftInFile = 0;
		m_refIRadFile = NULL;
	}
}

void radSoundRsdFileDataSource::_StateReadingHeader( void )
{
	rAssert(  m_refIRadFile->IsOpen( ) );  // If the file isn't open, we shouldn't be here

	// Get the data under control for strange platforms

	m_StateInfo.m_ReadingHeaderInfo.m_pRadSoundHalFileHeader->ConvertToPlatformEndian( );

	// Create a new audio format object

	m_refIRadSoundHalAudioFormat = ::radSoundHalAudioFormatCreate( GetThisAllocator( ) );

	m_StateInfo.m_ReadingHeaderInfo.m_pRadSoundHalFileHeader->InitializeAudioFormat(
		m_refIRadSoundHalAudioFormat, GetThisAllocator( ) );
	
	// Release the tempory memory

	::radMemoryFreeAligned( RADMEMORY_ALLOC_TEMP, m_StateInfo.m_ReadingHeaderInfo.m_pRadSoundHalFileHeader );
	m_StateInfo.m_ReadingHeaderInfo.m_pRadSoundHalFileHeader = NULL;

    InitFile( );
}

void radSoundRsdFileDataSource::InitFile( void )
{
	// Figure out where to start reading in the file

	unsigned int initialPlaybackOffsetInBytes =
	    m_refIRadSoundHalAudioFormat->ConvertSizeType(
		    IRadSoundHalAudioFormat::Bytes,
		    m_StateInfo.m_ReadingHeaderInfo.m_InitialPlaybackPos,
		    m_StateInfo.m_ReadingHeaderInfo.m_InitalPlaybackPosUnits );

	// Ensure that the offset is a multiple of the framesize

	initialPlaybackOffsetInBytes = m_refIRadSoundHalAudioFormat->FramesToBytes(
		m_refIRadSoundHalAudioFormat->BytesToFrames( initialPlaybackOffsetInBytes ) );

	// Calculate the amount of data contained in the file

	m_FramesLeftInFile = m_refIRadSoundHalAudioFormat->BytesToFrames(
		m_refIRadFile->GetSize( ) - RSD_FILE_DATA_OFFSET );

	rAssert( m_refIRadSoundHalAudioFormat->BytesToFrames( initialPlaybackOffsetInBytes ) < m_FramesLeftInFile );
	m_FramesLeftInFile -= m_refIRadSoundHalAudioFormat->BytesToFrames( initialPlaybackOffsetInBytes );

	//
	// An important assertion: The filesize needs to be a multiple of the framesize
	//
	unsigned int fileDataSize = m_refIRadFile->GetSize( ) - RSD_FILE_DATA_OFFSET - initialPlaybackOffsetInBytes;
	unsigned int roundedBytes = m_refIRadSoundHalAudioFormat->FramesToBytes( m_FramesLeftInFile );
	rAssertMsg( fileDataSize == roundedBytes, "File Data is not block aligned (corrupted )" );

	// Move into the idle state before possibly getting called back

	m_StateInfo.m_State = StateInfo::IDLE;

	// Skip to the data and include the clients requested start position

	m_refIRadFile->SetPositionAsync( RSD_FILE_DATA_OFFSET + initialPlaybackOffsetInBytes );

    // Bit of a hack here, service the sound system so streamers pool the IsInitialized()
    // flag right now.

    ::radSoundHalSystemGet( )->Service( );
}

void radSoundRsdFileDataSource::_StateIdle( void )
{
}

void radSoundRsdFileDataSource::_StateReadingData( void )
{
	// Update our position info and callback the client

	m_FramesLeftInFile -= m_StateInfo.m_ReadingDataInfo.m_ReadSizeInFrames;
	m_StateInfo.m_State = StateInfo::IDLE;

	radRef< IRadSoundHalDataSourceCallback > xIRadSoundHalDataSourceCallback = m_refIRadSoundHalDataSourceCallback;
	m_refIRadSoundHalDataSourceCallback = NULL;

    /* rDebugPrintf( "FDS:[%s]\n\tthis:[0x%x] Calling back for [0x%x] frames callback: [0x%x]\n",
        this->fileName,
        this,
        m_StateInfo.m_ReadingDataInfo.m_ReadSizeInFrames,
        xIRadSoundHalDataSourceCallback.m_pInterface ); */
        
	xIRadSoundHalDataSourceCallback->OnDataSourceFramesLoaded( m_StateInfo.m_ReadingDataInfo.m_ReadSizeInFrames );
}

IRadSoundRsdFileDataSource * radSoundRsdFileDataSourceCreate( radMemoryAllocator allocator )
{
	return new ( "radSoundRsdFileDataSource", allocator ) radSoundRsdFileDataSource( );
}
