//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include "pch.hpp"
#include "bufferloader.hpp"

template<> radSoundBufferLoaderWin * radLinkedClass< radSoundBufferLoaderWin >::s_pLinkedClassHead = NULL;
template<> radSoundBufferLoaderWin * radLinkedClass< radSoundBufferLoaderWin >::s_pLinkedClassTail = NULL;

//=========================================================================
// radSoundBufferLoaderWin::radSoundBufferLoaderWin
//=========================================================================

radSoundBufferLoaderWin::radSoundBufferLoaderWin
(
    IRefCount * pIRefCount_Owner,
    void * pBuffer,	
    IRadSoundHalDataSource * pIRadSoundHalDataSource,
    IRadSoundHalAudioFormat * pIRadSoundHalAudioFormat,
    unsigned int numberOfFrames,
    IRadSoundHalBufferLoadCallback * pISoundBufferCallback
)
    :
    m_xIRefCount_Owner( pIRefCount_Owner ),
    m_xIRadSoundHalBufferLoadCallback( pISoundBufferCallback ),
    m_xIRadSoundHalDataSource( pIRadSoundHalDataSource ),
    m_NumberOfFrames( numberOfFrames ),
    m_pBuffer( pBuffer ),
    m_Cancelled( false )
{
    rAssert( m_xIRadSoundHalDataSource != NULL );

    AddRef( );

    if ( GetLinkedClassHead( ) == this )
    {
        Start( );
    }
}

void radSoundBufferLoaderWin::Finish( void )
{
    if ( GetLinkedClassNext( ) )
    {
        GetLinkedClassNext( )->Start( );
    }

    Release( );
}

void radSoundBufferLoaderWin::Start( void )
{
    if ( m_Cancelled == true )
    {
        Finish( );
    }
    else
    {      
        //
        // Make sure they passed us valid objects
        //
        rAssert( m_xIRadSoundHalDataSource != NULL );
        rAssert( m_xIRadSoundHalBufferLoadCallback != NULL );      

 		m_xIRadSoundHalDataSource->GetFramesAsync( 
			( char * ) m_pBuffer,
			radMemorySpace_Local,
			m_NumberOfFrames,
			this
		);
    }
}
//=========================================================================
// radSoundBufferLoaderWin::OnFileOperationsComplete
//=========================================================================

void radSoundBufferLoaderWin::OnDataSourceFramesLoaded( unsigned int framesActuallyRead )
{
    if( framesActuallyRead < m_NumberOfFrames )
    {
        unsigned int offsetInBytes = m_xIRadSoundHalDataSource->GetFormat( )->FramesToBytes( framesActuallyRead );
        unsigned int sizeInBytes = m_xIRadSoundHalDataSource->GetFormat( )->FramesToBytes( m_NumberOfFrames - framesActuallyRead );  
        unsigned char fillChar = ( m_xIRadSoundHalDataSource->GetFormat( )->GetBitResolution( ) == 8 ) ? 128 : 0;

        ::memset(
  		(char*) m_pBuffer + offsetInBytes,
  		    fillChar, sizeInBytes );

    }
  
    
	m_pBuffer = NULL;
    m_NumberOfFrames = 0;	

    if ( m_Cancelled == false )
    {
        m_xIRadSoundHalBufferLoadCallback->OnBufferLoadComplete( framesActuallyRead );
    }

    Finish( );
}

void radSoundBufferLoaderWin::Cancel( void )
{
    m_Cancelled = true;
}

void radSoundBufferLoaderWin::CancelOperations( IRefCount * pIRefCount_Owner )
{
    radSoundBufferLoaderWin * pSearch = radSoundBufferLoaderWin::GetLinkedClassHead( );

    while ( pSearch != NULL )
    {
        if ( pSearch->m_xIRefCount_Owner == pIRefCount_Owner )
        {
            pSearch->Cancel( );
        }
        pSearch = pSearch->GetLinkedClassNext( );
    }

}
