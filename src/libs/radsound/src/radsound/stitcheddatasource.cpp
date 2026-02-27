//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include "pch.hpp"
#include "stitcheddatasource.hpp"

//============================================================================
// radSoundStitchedDataSource::radSoundStitchedDataSource
//============================================================================

radSoundStitchedDataSource::radSoundStitchedDataSource( void )
	:
    m_FrameCount( 0 ),
    m_ReadMemorySpace( radMemorySpace_Null ),
	m_pCurrentReadPointer( NULL ),
	m_FramesLeftToRead( 0 ),
	m_FullReadSize( 0 ),
    m_LoadOutstanding( false ),
    m_pIRadSoundStitchCallback( NULL )
{

}

//============================================================================
// radSoundStitchedDataSource::~radSoundStitchedDataSource
//============================================================================

radSoundStitchedDataSource::~radSoundStitchedDataSource( void )
{
	rAssert( m_pIRadSoundStitchCallback == NULL );
    rAssert( m_xIRadSoundHalDataSourceCallback == NULL );
    rAssert( m_refIRadSoundHalDataSourceCallback_Cancelled == NULL );

	// Smart pointers take care of all destruction
}

//============================================================================
// radSoundStitchedDataSource::InitializeFromAudioFormat
//============================================================================

void radSoundStitchedDataSource::InitializeFromAudioFormat
( 
	IRadSoundHalAudioFormat * pIRshaf
)
{
	m_xIRadSoundHalAudioFormat = pIRshaf;
}

//============================================================================
// radSoundStitchedDataSource::InitializeFromDataSource
//============================================================================

void radSoundStitchedDataSource::InitializeFromDataSource
(
	IRadSoundHalDataSource * pIRadSoundHalDataSource
)
{
    rAssert( pIRadSoundHalDataSource != NULL );

	m_xIRadSoundHalDataSource = pIRadSoundHalDataSource;

    if ( m_xIRadSoundHalDataSource->GetState( ) == IRadSoundHalDataSource::Initializing )
    {
        AddToUpdateList( );
    }
}

//============================================================================
// radSoundStitchedDataSource::SetStitchCallback
//============================================================================

void radSoundStitchedDataSource::SetStitchCallback(
	IRadSoundStitchCallback * pIRssc,
	void * pUserData
)
{
	m_pIRadSoundStitchCallback = pIRssc;
	m_pRadSoundStitchCallbackUserData = pUserData;
}

//============================================================================
// radSoundStitchedDataSource::IsInitialized
//============================================================================

IRadSoundHalDataSource::State radSoundStitchedDataSource::GetState( void )
{
	// We are ready to go if our format is valid

	return( m_xIRadSoundHalAudioFormat != NULL ) ?
		IRadSoundHalDataSource::Initialized :
			IRadSoundHalDataSource::Initializing;
}

//============================================================================
// radSoundStitchedDataSource::GetFormat
//============================================================================

IRadSoundHalAudioFormat * radSoundStitchedDataSource::GetFormat( void )
{
	// This function should never be called if we are not initialized.

	rAssert( m_xIRadSoundHalAudioFormat != NULL );

	return m_xIRadSoundHalAudioFormat;
}

//============================================================================
// radSoundStitchedDataSource::GetRemainingFrames
//============================================================================

unsigned int radSoundStitchedDataSource::GetRemainingFrames( void )
{
	// This function should never be called if we are not initialized.

	rAssert( m_xIRadSoundHalAudioFormat != NULL );

	// The stitcher always returns infinite data.

	return 0xFFFFFFFF;
}

//============================================================================
// radSoundStitchedDataSource::OnTimerDone
//============================================================================

void radSoundStitchedDataSource::Update( unsigned int elapsedTime )
{
    rAssert( m_xIRadSoundHalDataSource != NULL );

	if ( m_xIRadSoundHalDataSource->GetState( ) != IRadSoundHalDataSource::Initializing )
	{
		m_xIRadSoundHalAudioFormat = m_xIRadSoundHalDataSource->GetFormat( );
	
		RemoveFromUpdateList( );

		Service( );
	}
}

//============================================================================
// radSoundStitchedDataSource::Reset
//============================================================================

void radSoundStitchedDataSource::ResetAudioFormat( IRadSoundHalAudioFormat * pAf )
{
    m_xIRadSoundHalAudioFormat = pAf;
}

void radSoundStitchedDataSource::Reset( void )
{
/*    rDebugPrintf( "STITCHER: Reset, this [0x0x] Load Outstanding: [0x%x] Callback: [0x%x], CallbackCancelled: [0x%x]\n",
        this,
        m_LoadOutstanding,
        m_xIRadSoundHalDataSourceCallback,
        m_refIRadSoundHalDataSourceCallback_Cancelled );  */      
        
   /*  if ( m_xIRadSoundHalDataSource != NULL )
    {
        rDebugPrintf( "\tpreviously had datasource: [%s]\n", this,
            m_xIRadSoundHalDataSource->GetName( ) );
    }
    else
    {
        rDebugPrintf( "\tpreviously had no datasource\n" );
    } */
    
    unsigned int framesLoaded = m_FullReadSize - m_FramesLeftToRead;

    m_FrameCount = 0;
    m_xIRadSoundHalDataSource = NULL;
    
    RemoveFromUpdateList( );

    m_FullReadSize = 0;
    m_FramesLeftToRead = 0;
    m_pCurrentReadPointer = 0;
    

    
    if ( m_xIRadSoundHalDataSourceCallback != NULL )
    {
        rAssert( m_refIRadSoundHalDataSourceCallback_Cancelled == NULL );

        radRef< IRadSoundHalDataSourceCallback > xIRshdsc = m_xIRadSoundHalDataSourceCallback;
        m_xIRadSoundHalDataSourceCallback = NULL;

        if ( m_LoadOutstanding == true && m_refIRadSoundHalDataSourceCallback_Cancelled == NULL )
        {
            // We MUST hold on to the callback object until the load is
            // complete, otherwise the memory being loaded into might be
            // freed while it is being used!

            m_refIRadSoundHalDataSourceCallback_Cancelled = xIRshdsc;
        }
        else
        {
            // Here we are either cancelling a previous request, or are
            // not in the middle of a load.

		   /*  rDebugPrintf( "STITCHER: reset, callingback, this [0x%x],  [%0x%x] frames, callback: [0x%x]\n",
		        this,
		        framesLoaded,
		        xIRshdsc.m_pInterface ); */
		        
            xIRshdsc->OnDataSourceFramesLoaded( framesLoaded );    
        }
    }
}

//============================================================================
// radSoundStitchedDataSource::Service
//============================================================================

void radSoundStitchedDataSource::Service( void )
{
    if( m_LoadOutstanding == false )
    {
	    if ( OnUpdateList( ) == false ) // Initializing ?
	    {
		    if ( m_FramesLeftToRead > 0 )
		    {
                // We'll be calling out so let's protect ourselves from destruction
                AddRef( );

			    if ( m_xIRadSoundHalDataSource == NULL )
			    {
				    // Oops, out of data ask for more

                    if ( m_pIRadSoundStitchCallback != NULL )
                    {
				        m_pIRadSoundStitchCallback->OnStitch( & m_xIRadSoundHalDataSource,
                            m_FrameCount, m_pRadSoundStitchCallbackUserData );
                    }

				    if( m_xIRadSoundHalDataSource == NULL )
				    {
					    // Client has signalled end of stream time to reset ourselves
                        Reset( );
  				    }
				    else if ( m_xIRadSoundHalDataSource->GetState( ) == IRadSoundHalDataSource::Initializing )
				    {
                        AddToUpdateList( );
				    }
			    }

			    if ( m_xIRadSoundHalDataSource != NULL && m_xIRadSoundHalDataSource->GetState( ) == IRadSoundHalDataSource::Initialized )
			    {
                    m_xIRadSoundHalAudioFormat = m_xIRadSoundHalDataSource->GetFormat( );

				    rAssert( m_pCurrentReadPointer != NULL );
                    
                    m_LoadOutstanding = true;

				    m_xIRadSoundHalDataSource->GetFramesAsync(
                        m_pCurrentReadPointer,
                        m_ReadMemorySpace,
                        m_FramesLeftToRead,
                        this );
			    }

                // Now we're safe
                Release( );
		    }
	    }
    }
}

//============================================================================
// radSoundStitchedDataSource::OnDataSourceFramesLoaded
//============================================================================

void radSoundStitchedDataSource::OnDataSourceFramesLoaded( unsigned int actuallyRead )
{
    rAssert( m_LoadOutstanding == true );

    m_LoadOutstanding = false;

    if ( m_refIRadSoundHalDataSourceCallback_Cancelled == NULL ) // was this the cancelled load?
    {
	    // Here we have finisehd reading a chunk, adjust our current read
	    // pointers

        m_FrameCount += actuallyRead;

        rAssert( actuallyRead <= m_FramesLeftToRead );

	    // rDebugPrintf( "Stitch: Read: [%d] Frames\n", m_CurrentReadSize );

	    m_pCurrentReadPointer = static_cast< char*>(m_pCurrentReadPointer) +
		    m_xIRadSoundHalAudioFormat->FramesToBytes( actuallyRead );

	    m_FramesLeftToRead -= actuallyRead;

	    if ( m_FramesLeftToRead == 0 )
	    {
		    radRef< IRadSoundHalDataSourceCallback > xIRshdsc = m_xIRadSoundHalDataSourceCallback;

		    unsigned int fullReadSize = m_FullReadSize;

		    m_xIRadSoundHalDataSourceCallback = NULL;
		    m_FullReadSize = 0;
		    m_pCurrentReadPointer = 0;
            m_ReadMemorySpace = radMemorySpace_Null;

		    /* rDebugPrintf( "STITCHER: read complete, this [0x%x], [0x%x] frames, callback: [0x%x]\n",
		        this,
		        fullReadSize,
		        xIRshdsc.m_pInterface ); */
		        
		    xIRshdsc->OnDataSourceFramesLoaded( fullReadSize );
	    }
	    else
	    {
		    // We have more to read which means the old source couldn't
		    // supply everything--clear it out.
		    
		    m_xIRadSoundHalDataSource = NULL;
		    Service( );
	    }
    }
    else
    {
        // Release the cancelled callback, this might free the memory we
        // are reading into.

        radRef< IRadSoundHalDataSourceCallback > refIRadSoundHalDataSourceCallback_Temp = m_refIRadSoundHalDataSourceCallback_Cancelled;
        m_refIRadSoundHalDataSourceCallback_Cancelled = NULL;
        
		/* rDebugPrintf( "STITCHER: reset, callingback cancelled with zero, this [0x%x],  [0x%x] frames, callback: [0x%x]\n",
		    this,
		    0,
		    refIRadSoundHalDataSourceCallback_Temp.m_pInterface ); */
		                
        refIRadSoundHalDataSourceCallback_Temp->OnDataSourceFramesLoaded( 0 );

        Service( );
    }
}

//============================================================================
// radSoundStitchedDataSource::GetFramesAsync
//============================================================================

void radSoundStitchedDataSource::GetFramesAsync
(
	void * pBytes,
	radMemorySpace destinationMemorySpace,
	unsigned int numberOfFrames,
	IRadSoundHalDataSourceCallback * pCallback
)
{
    rAssert( m_xIRadSoundHalDataSourceCallback == NULL );
    
   /* rDebugPrintf(
        "STITCHER: this [0x0x], Request for frames [0x%x] from [0x%x]",
        this,
        numberOfFrames,
        pCallback ); */
        
	// Initialize the read, it does get chunked up if we need to stitch.

    m_ReadMemorySpace = destinationMemorySpace;
	m_pCurrentReadPointer = pBytes;
	m_FullReadSize = numberOfFrames;
	m_FramesLeftToRead = numberOfFrames;
	m_xIRadSoundHalDataSourceCallback = pCallback;

	Service( );
}

//============================================================================
// ::radSoundStitchedDataSourceCreate
//============================================================================

IRadSoundStitchedDataSource * radSoundStitchedDataSourceCreate( radMemoryAllocator a )
{
	return new ( "radSoundStitchedDataSource", a ) radSoundStitchedDataSource( );
}