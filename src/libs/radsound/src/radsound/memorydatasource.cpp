//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include "pch.hpp"
#include "memorydatasource.hpp"

radSoundMemoryDataSource::radSoundMemoryDataSource( void )
    :
    m_StartOffsetInBytes( 0 ),
    m_LengthInFrames( 0 ),
    m_NumberOfFramesRead( 0 ),
    m_FramesToRead( 0 )
{
}

radSoundMemoryDataSource::~radSoundMemoryDataSource( void )
{
    rDebugChannelPrintf(
        radSoundDebugChannel,
        "radsound: radSoundMemoryDataSource destroyed: [%s]\n",
        m_xIRadSoundMemorySpaceObject->GetIdentifier( ) );
}

void radSoundMemoryDataSource::Initialize(
    IRadSoundHalAudioFormat * pIRshaf,  
    IRadSoundMemorySpaceObject   * pIRmso,   
    unsigned int startOffset,           
    unsigned int size,                  
    IRadSoundHalAudioFormat::SizeType st)
{
    rAssert( pIRshaf != NULL );
    rAssert( pIRmso != NULL );

    m_xIRadSoundHalAudioFormat = pIRshaf;
    m_xIRadSoundMemorySpaceObject = pIRmso;
    m_StartOffsetInBytes       = m_xIRadSoundHalAudioFormat->ConvertSizeType(
        IRadSoundHalAudioFormat::Bytes, startOffset, st );
    m_LengthInFrames            = m_xIRadSoundHalAudioFormat->ConvertSizeType(
        IRadSoundHalAudioFormat::Frames, size, st );

    rAssert( m_StartOffsetInBytes < m_xIRadSoundMemorySpaceObject->GetSizeInBytes( ) );

    rAssert( (m_xIRadSoundHalAudioFormat->FramesToBytes( m_LengthInFrames ) + m_StartOffsetInBytes) <=
        m_xIRadSoundMemorySpaceObject->GetSizeInBytes( ) );
}

IRadSoundHalDataSource::State radSoundMemoryDataSource::GetState( void )
{
    return IRadSoundHalDataSource::Initialized;
}

IRadSoundHalAudioFormat * radSoundMemoryDataSource::GetFormat( void )
{
    return m_xIRadSoundHalAudioFormat;
}

unsigned int radSoundMemoryDataSource::GetRemainingFrames( void )
{
    return m_LengthInFrames - m_NumberOfFramesRead;
}

void radSoundMemoryDataSource::Update( unsigned int elapsedTime )
{
    rAssert( m_xIRadMemorySpaceCopyRequest != NULL );
    rAssert( m_xIRadSoundHalDataSourceCallback != NULL );
   
    if ( m_xIRadMemorySpaceCopyRequest->IsDone( ) )
    {
        RemoveFromUpdateList( );

        m_NumberOfFramesRead += m_FramesToRead;

        radRef< IRadSoundHalDataSourceCallback > xIRshdsc( m_xIRadSoundHalDataSourceCallback );
        m_xIRadSoundHalDataSourceCallback = NULL;
        m_xIRadMemorySpaceCopyRequest = NULL;
        unsigned int framesToRead = m_FramesToRead;
        m_FramesToRead = 0;

        xIRshdsc->OnDataSourceFramesLoaded( framesToRead );

        Release( ); // Undo the addref in GetFramesAsync
    }
}

void radSoundMemoryDataSource::GetFramesAsync( 
	void * pBytes, 
	radMemorySpace destinationMemorySpace, 
	unsigned int numberOfFrames,
	IRadSoundHalDataSourceCallback * pCallback )
{
    rAssert( m_xIRadMemorySpaceCopyRequest == NULL );
    rAssert( m_xIRadSoundHalDataSourceCallback == NULL );

    if ( GetRemainingFrames() == 0 )
    {
        // Empty, just callback with zero.

        pCallback->OnDataSourceFramesLoaded( 0 );
    }
    else
    {
        m_xIRadSoundHalDataSourceCallback = pCallback;

        if ( numberOfFrames > GetRemainingFrames( ) )
        {
            m_FramesToRead = GetRemainingFrames( );
        }
        else
        {
            m_FramesToRead = numberOfFrames;
        }
   
        uintptr_t startAddress = (uintptr_t) m_xIRadSoundMemorySpaceObject->GetAddress( );

        uintptr_t memoryCopyStart = startAddress + m_StartOffsetInBytes
            + m_xIRadSoundHalAudioFormat->FramesToBytes( m_NumberOfFramesRead );
    
        unsigned bytesToCopy = m_xIRadSoundHalAudioFormat->FramesToBytes( m_FramesToRead );

        rAssert( (memoryCopyStart + bytesToCopy) <= (startAddress + m_xIRadSoundMemorySpaceObject->GetSizeInBytes( ) ) );

        AddRef( );
                
        m_xIRadMemorySpaceCopyRequest = ::radMemorySpaceCopyAsync(
            pBytes, destinationMemorySpace,
            (void*) memoryCopyStart,
            m_xIRadSoundMemorySpaceObject->GetMemorySpace( ),
            bytesToCopy );
       
        AddToUpdateList( );
    }
}

IRadSoundMemoryDataSource * radSoundMemoryDataSourceCreate( radMemoryAllocator a )
{
    return new ( "radSoundMemoryDataSource", a ) radSoundMemoryDataSource( );
}
