//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include "pch.hpp"
#include <radsoundfile.hpp>
#include "buffereddatasource.hpp"

//============================================================================
// radSoundBufferedDataSource::radSoundBufferedDataSource
//============================================================================

radSoundBufferedDataSource::radSoundBufferedDataSource( void )
	:
	m_State( NONE ),
	m_pFrameBuffer( NULL ),
	m_BufferSizeInFrames( 0 ),
	m_StartOfDataInFrames( 0 ),
	m_EndOfDataInFrames( 0 ),
	m_QueueFull( false ),
	m_FullCopySize( 0 ),
	m_FramesLeftToCopy( 0 ),
	m_CurrentFramesToCopy( 0 ),
	m_pCurrentCopyPointer( NULL ),
	m_CopyMemorySpace( radMemorySpace_Null ),
	m_xICopyRequest( NULL ),
	m_ReadSizeInFrames( 0 ),
	m_OutOfData( false ),
    m_LowWaterMark( 0.50f )
{
    ::radStringCreate( & m_xIRadString_Name, GetThisAllocator( ) );
}

//============================================================================
// radSoundBufferedDataSource::~radSoundBufferedDataSource
//============================================================================

radSoundBufferedDataSource::~radSoundBufferedDataSource( void )
{
	if ( m_pFrameBuffer != NULL )
	{
        m_xIRadMemoryAllocator_FrameBuffer->FreeMemoryAligned( m_pFrameBuffer );
	}
}

//============================================================================
// radSoundBufferedDataSource::IsInitialized
//============================================================================

IRadSoundHalDataSource::State radSoundBufferedDataSource::GetState( void )
{
	return m_State == INITIALIZED ? IRadSoundHalDataSource::Initialized : IRadSoundHalDataSource::Initializing;
}

//============================================================================
// radSoundBufferedDataSource::SetInputDataSource
//============================================================================

void radSoundBufferedDataSource::SetInputDataSource( IRadSoundHalDataSource * pIRshds )
{
    if ( m_State == NONE )
    {
        if ( pIRshds != NULL )
        {
	        m_State = INITIALIZING;
            m_xIRadSoundHalDataSource = pIRshds;
            AddToUpdateList( );
        }
    }
    else if ( m_State == INITIALIZING )
    {
        m_xIRadSoundHalDataSource = pIRshds;

        if ( pIRshds == NULL )
        {
            m_State = NONE;
            RemoveFromUpdateList( );
        }
    }
    else if ( m_State == REINITIALIZING )
    {
        // Just swap the source "on deck"

        m_xIRadSoundHalDataSource_ReInit = pIRshds;
    }
    else if ( m_State == INITIALIZED )
    {            
        // Here we are in the middle of a copy or a read.
        // We must wait until the outstanding operations have finished, then
        // re-initialize.

        m_State = REINITIALIZING;

        m_xIRadSoundHalDataSource_ReInit = pIRshds;
    }
    else
    {
        rAssert( false );
    }

	Service( ); // Kick start so we init the memory right now (if possible)
}

//============================================================================
// radSoundBufferedDataSource::Initialize
//============================================================================

void radSoundBufferedDataSource::Initialize
(
	radMemorySpace bufferSpace,
	IRadMemoryAllocator * pIRadMemoryAllocator,
	unsigned int milliseconds,
	IRadSoundHalAudioFormat::SizeType sizeType,
    IRadSoundHalAudioFormat * pIRshaf,
    const char * pIdentifier
)
{
    // This function is only called once at the beggining

	rAssert( m_State == NONE );
    rAssert( pIRadMemoryAllocator != NULL );
    rAssertMsg( pIdentifier != NULL,
        "You MUST name all of your buffered data source objects so we can track memory usefully" );

    m_xIRadString_Name->Copy( pIdentifier );

	m_InitSize = milliseconds;
	m_InitSizeType = sizeType;

	m_FrameBufferMemorySpace = bufferSpace;
	m_xIRadMemoryAllocator_FrameBuffer = pIRadMemoryAllocator;

    if ( pIRshaf != NULL )
    {
        m_xIRadSoundHalAudioFormat = pIRshaf;
        AllocateResources( );
    }
};

//============================================================================
// radSoundBufferedDataSource::SetLowWaterMark
//============================================================================

void radSoundBufferedDataSource::SetLowWaterMark( float lowWaterMark )
{
    rAssert( lowWaterMark > 0.0f && lowWaterMark <= 1.0f );
    m_LowWaterMark = lowWaterMark;
}

//============================================================================
// radSoundBufferedDataSource::GetLowWaterMark
//============================================================================

float radSoundBufferedDataSource::GetLowWaterMark( void )
{
    return m_LowWaterMark;
}

//============================================================================
// radSoundBufferedDataSource::GetFormat
//============================================================================

IRadSoundHalAudioFormat * radSoundBufferedDataSource::GetFormat( void )
{
    // The format can't change once it has been initialized

	return m_xIRadSoundHalAudioFormat;
}

//============================================================================
// radSoundBufferedDataSource::GetInputDataSource
//============================================================================

IRadSoundHalDataSource * radSoundBufferedDataSource::GetInputDataSource( void )
{
	return m_xIRadSoundHalDataSource_ReInit ?
           m_xIRadSoundHalDataSource_ReInit :
           m_xIRadSoundHalDataSource;
}

//============================================================================
// radSoundBufferedDataSource::GetAvailableFrames
//============================================================================

unsigned int radSoundBufferedDataSource::GetAvailableFrames( void )
{
    rAssert( m_State == INITIALIZED || m_State == REINITIALIZING );

	if ( m_StartOfDataInFrames < m_EndOfDataInFrames )
	{
		return m_EndOfDataInFrames - m_StartOfDataInFrames;
	}
	else if ( m_StartOfDataInFrames > m_EndOfDataInFrames )
	{
		return m_BufferSizeInFrames - ( m_StartOfDataInFrames - m_EndOfDataInFrames );
	}
	else
	{
		if ( m_QueueFull )
		{
			return m_BufferSizeInFrames;
		}
		else
		{
			return 0;
		}
	}		
}

//============================================================================
// radSoundBufferedDataSource::OnDataSourceFramesLoaded
//============================================================================

void radSoundBufferedDataSource::OnDataSourceFramesLoaded( unsigned int framesActuallyRead )
{
	rAssert( m_ReadSizeInFrames > 0 );

	m_EndOfDataInFrames = ( m_EndOfDataInFrames + framesActuallyRead ) % m_BufferSizeInFrames;

	if ( framesActuallyRead < m_ReadSizeInFrames )
	{
		m_OutOfData = true;
	}

	if ( m_EndOfDataInFrames == m_StartOfDataInFrames && ! m_OutOfData )
	{
		m_QueueFull = true;
	}

	m_ReadSizeInFrames = 0;

	if ( framesActuallyRead > 0 )
	{
		Service( );
	}
}

//============================================================================
// radSoundBufferedDataSource::GetRemainingFrames
//============================================================================

unsigned int radSoundBufferedDataSource::GetRemainingFrames( void )
{
	if ( m_xIRadSoundHalDataSource != NULL )
	{
	    if ( m_OutOfData )
	    {
	        return GetAvailableFrames( );
	    }
	    else if ( m_xIRadSoundHalDataSource->GetRemainingFrames( ) == 0xFFFFFFFF )
		{
			return 0xFFFFFFFF;
		}
		else
		{
			return m_xIRadSoundHalDataSource->GetRemainingFrames( ) + GetAvailableFrames( );
		}
	}

	return 0;
}

//============================================================================
// radSoundBufferedDataSource::GetFramesAsync
//============================================================================

void radSoundBufferedDataSource::GetFramesAsync
(
	void * pFrameBuffer,
	radMemorySpace destinationMemorySpace,
	unsigned int sizeInFrames,
	IRadSoundHalDataSourceCallback * pIRshdsc
)
{
	rAssert( m_State == INITIALIZED );
	//rAssert( m_FullCopySize == 0 );

	// Set up the read.

	m_FullCopySize = sizeInFrames;
	m_FramesLeftToCopy = sizeInFrames;
	m_xIRadSoundHalDataSourceCallback = pIRshdsc;
	m_pCurrentCopyPointer = (char*) pFrameBuffer;
	m_CopyMemorySpace = destinationMemorySpace;

    if ( GetAvailableFrames( ) < sizeInFrames && ( false == m_OutOfData ) )
	{
		rReleasePrintf( "AUDIO: Buffer Underrun: [%s]\n", m_xIRadSoundHalDataSource ?
		    m_xIRadSoundHalDataSource->GetName( ) : "NULL" );
	}

	Service( );
}

//============================================================================
// radSoundBufferedDataSource::ServiceCopy
//============================================================================

void radSoundBufferedDataSource::ServiceCopy( void )
{
    // If we are currently copying

	if( m_xICopyRequest != NULL )
	{
        rAssert( m_State == INITIALIZED || m_State == REINITIALIZING );

        // and the request is done

		if( m_xICopyRequest->IsDone( ) )
		{
			// The request is complete so set it to null
			m_xICopyRequest = NULL;
            
			rAssert( m_CurrentFramesToCopy > 0 );

			m_pCurrentCopyPointer += m_xIRadSoundHalAudioFormat->FramesToBytes( m_CurrentFramesToCopy );
			m_StartOfDataInFrames = ( m_StartOfDataInFrames + m_CurrentFramesToCopy ) % m_BufferSizeInFrames;
			m_FramesLeftToCopy -= m_CurrentFramesToCopy;
			m_CurrentFramesToCopy = 0;

			m_QueueFull = false;

			if( m_FramesLeftToCopy == 0 || m_State == REINITIALIZING ) 
			{
				unsigned int framesCopied = m_FullCopySize - m_FramesLeftToCopy;

				radRef< IRadSoundHalDataSourceCallback > xIRshdsc( m_xIRadSoundHalDataSourceCallback );

				// Reset copy info

				m_FullCopySize = 0;
				m_FramesLeftToCopy = 0;
				m_xIRadSoundHalDataSourceCallback = NULL;
				m_pCurrentCopyPointer = NULL;
				m_CopyMemorySpace = radMemorySpace_Null;

				// Callback last thing so we don't blow up.

				Service( );

				xIRshdsc->OnDataSourceFramesLoaded( framesCopied );

				return;
			}
		}
	}
    else if ( m_FramesLeftToCopy > 0 )
	{
		m_CurrentFramesToCopy = GetAvailableFrames( );

		if ( m_CurrentFramesToCopy > 0 )
		{
			// Don't read past the end of the buffer, we will get the wrapped
			// chunk next time.
			
			if ( ( m_StartOfDataInFrames + m_CurrentFramesToCopy ) > m_BufferSizeInFrames )
			{
				m_CurrentFramesToCopy = m_BufferSizeInFrames - m_StartOfDataInFrames;
			}

			if ( m_CurrentFramesToCopy >= m_FramesLeftToCopy )
			{
				m_CurrentFramesToCopy = m_FramesLeftToCopy;
			}

			if ( m_CurrentFramesToCopy > 0 )
			{
				/* rDebugPrintf( "Copy: From: [0x%x] to [0x%x], size: [%d/%d]\n",
					m_pFrameBuffer + m_xIRadSoundHalAudioFormat->FramesToBytes( m_StartOfDataInFrames ),
					m_pCurrentCopyPointer,
					m_CurrentFramesToCopy,
					m_xIRadSoundHalAudioFormat->FramesToBytes( m_CurrentFramesToCopy ) ); */

				m_xICopyRequest = ::radMemorySpaceCopyAsync(
					m_pCurrentCopyPointer,
					m_CopyMemorySpace,
					m_pFrameBuffer + m_xIRadSoundHalAudioFormat->FramesToBytes( m_StartOfDataInFrames ),
					m_FrameBufferMemorySpace, m_xIRadSoundHalAudioFormat->FramesToBytes( m_CurrentFramesToCopy ) );

				Service( );
			}
		}
		else if ( m_OutOfData || m_State == REINITIALIZING )
		{
			radRef< IRadSoundHalDataSourceCallback > xIRshdsc( m_xIRadSoundHalDataSourceCallback );

            unsigned int framesCopiedSoFar = m_FullCopySize - m_FramesLeftToCopy;

			// Reset copy info
            
			m_FullCopySize = 0;
			m_FramesLeftToCopy = 0;
			m_xIRadSoundHalDataSourceCallback = NULL;
			m_pCurrentCopyPointer = NULL;
			m_CopyMemorySpace = radMemorySpace_Null;

			// Callback last thing so we don't blow up.

			Service( );

			xIRshdsc->OnDataSourceFramesLoaded( framesCopiedSoFar );
		}
	}
}

//============================================================================
// radSoundBufferedDataSource::OnMemoryCopyAsyncComplete
//============================================================================

void radSoundBufferedDataSource::OnMemoryCopyAsyncComplete( void * pUserData )
{

}

//============================================================================
// radSoundBufferedDataSource::Update
//============================================================================

void radSoundBufferedDataSource::Update( unsigned int elapsedTime )
{
	Service( );
}

//============================================================================
// radSoundBufferedDataSource::Service
//============================================================================

void radSoundBufferedDataSource::Service( void )
{
	switch ( m_State )
	{
		case NONE:
		{
			// do nothing
			break;
		}
		case INITIALIZING:
		{            
			ServiceInitializingSource( );

			break;
		}
		case INITIALIZED:
		{
			ServiceRead( );
			ServiceCopy( );

			break;
		}
        case REINITIALIZING:
        {
            if ( m_ReadSizeInFrames == 0 && m_FramesLeftToCopy == 0 )
            {
	            m_EndOfDataInFrames = 0;
                m_StartOfDataInFrames = 0;
	            m_QueueFull = false;
	            m_OutOfData = false;	            

                if ( m_xIRadSoundHalDataSource_ReInit != NULL )
                {
                    m_State = INITIALIZING;
                }
                else
                {
                    m_State = NONE;
                    RemoveFromUpdateList( );
                }

                m_xIRadSoundHalDataSource = m_xIRadSoundHalDataSource_ReInit;                    
                m_xIRadSoundHalDataSource_ReInit = NULL;
            }
            else
            {
                ServiceCopy( );
            }
        }
	}
}

//============================================================================
// radSoundBufferedDataSource::ServiceInitializingSource
//============================================================================

void radSoundBufferedDataSource::ServiceInitializingSource( void )
{
	if ( m_xIRadSoundHalDataSource->GetState( ) == IRadSoundHalDataSource::Initialized )
	{
        //
        // Check if we already have initialized the buffer memory and format.
        // This is true if we were REINITIALIZING
        //

        if ( m_xIRadSoundHalAudioFormat == NULL )
        {
            m_xIRadSoundHalAudioFormat = m_xIRadSoundHalDataSource->GetFormat( );
            AllocateResources( );
        }
        else
        {
            rAssert( m_xIRadSoundHalAudioFormat->Matches( m_xIRadSoundHalDataSource->GetFormat( ) ) );
            rAssert( m_pFrameBuffer != NULL );
            rAssert( m_BufferSizeInFrames > 0 );
        }

		m_State = INITIALIZED;

		Service( );
	}
}

//============================================================================
// radSoundBufferedDataSource::AllocateResources
//============================================================================

void radSoundBufferedDataSource::AllocateResources( void )
{
    rAssert( m_BufferSizeInFrames == 0 );
    rAssert( m_pFrameBuffer == NULL );

    unsigned int buffersizeInBytes = 
		m_xIRadSoundHalAudioFormat->ConvertSizeType(
			IRadSoundHalAudioFormat::Bytes, // target
			m_InitSize,
			m_InitSizeType );
    //
    // Round up the buffersize in bytes for optimal disk access
    //

    buffersizeInBytes = ::radMemoryRoundUp( 
            buffersizeInBytes, 
            radSoundHalDataSourceReadMultipleGet( ) * 2 );

    m_BufferSizeInFrames = m_xIRadSoundHalAudioFormat->ConvertSizeType(
			IRadSoundHalAudioFormat::Frames, // target
			buffersizeInBytes,
			IRadSoundHalAudioFormat::Bytes );

	m_pFrameBuffer = (char*) m_xIRadMemoryAllocator_FrameBuffer->GetMemoryAligned(
		buffersizeInBytes,
		radSoundHalDataSourceReadAlignmentGet( ) );

    rAssert( m_pFrameBuffer != NULL );

    ::radMemoryMonitorIdentifyAllocation(
        m_pFrameBuffer, radSoundDebugChannel,
        m_xIRadString_Name->GetChars( ),
        NULL,
        m_FrameBufferMemorySpace );
}

//============================================================================
// radSoundBufferedDataSource::ServiceRead
//============================================================================

void radSoundBufferedDataSource::ServiceRead( void )
{
    if ( ! m_OutOfData )
    {
	    if ( m_ReadSizeInFrames == 0 )
	    {
            unsigned int lowWaterMarkInFrames = radSoundFloatToUInt(
                    (radSoundUIntToFloat( m_BufferSizeInFrames ) * m_LowWaterMark ) );

            unsigned int optimalReadMultipleInFrames =
                m_xIRadSoundHalAudioFormat->BytesToFrames( radSoundHalDataSourceReadMultipleGet( ) );

		    if ( GetAvailableFrames( ) <= lowWaterMarkInFrames )
		    {
			    unsigned int frames;

			    if ( m_QueueFull )
			    {
				    frames = 0;
			    }
			    else if ( m_StartOfDataInFrames <= m_EndOfDataInFrames )
			    {
				    // This handles !queueFull and start==end
				    frames = m_BufferSizeInFrames - m_EndOfDataInFrames;
			    }
			    else
			    {
				    frames = m_StartOfDataInFrames - m_EndOfDataInFrames;
			    }

                frames = ::radMemoryRoundDown( frames, optimalReadMultipleInFrames );

			    rAssert( frames + m_EndOfDataInFrames <= m_BufferSizeInFrames );

			    if ( frames > 0 )
			    {
				    m_ReadSizeInFrames = frames;

				    /* rDebugPrintf( "Buffer: Reading: [%d] frames at: [0x%x]\n", frames,
					    m_pFrameBuffer + m_xIRadSoundHalAudioFormat->FramesToBytes( m_EndOfDataInFrames ) ); */

				    m_xIRadSoundHalDataSource->GetFramesAsync( 
					    m_pFrameBuffer + m_xIRadSoundHalAudioFormat->FramesToBytes( m_EndOfDataInFrames ),
					    m_FrameBufferMemorySpace,
					    frames, 
					    this );
			    }
		    }
        }
	}
}

bool radSoundBufferedDataSource::IsBufferFull( void )
{
    return m_OutOfData;
}
    
//============================================================================
// ::radSoundBufferedDataSourceCreate
//============================================================================

IRadSoundBufferedDataSource * radSoundBufferedDataSourceCreate( radMemoryAllocator allocator )
{
	radSoundBufferedDataSource * pRsbds = new ( "radSoundBufferedDataSource", allocator ) radSoundBufferedDataSource( );
   
    return pRsbds;
}
