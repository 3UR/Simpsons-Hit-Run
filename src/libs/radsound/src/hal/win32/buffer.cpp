//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//============================================================================
// Include Files
//============================================================================

#include "pch.hpp"
#include <stdio.h>
#include "buffer.hpp"
#include "bufferloader.hpp"
#include "system.hpp"
#include "radinprogext.h"

const unsigned int RADSOUNDHAL_BUFFER_CHANNEL_ALIGNMENT = 1;

//============================================================================
// Static Initialization
//============================================================================

template<> radSoundHalBufferWin * radLinkedClass<radSoundHalBufferWin>::s_pLinkedClassHead = NULL;
template<> radSoundHalBufferWin * radLinkedClass<radSoundHalBufferWin>::s_pLinkedClassTail = NULL;

//========================================================================
// radSoundHalBufferWin::~radSoundHalBufferWin
//========================================================================

radSoundHalBufferWin::~radSoundHalBufferWin( void )
{
    if (m_Buffer)
    {
        alDeleteBuffers(1, &m_Buffer);
    }
}

//========================================================================
// radSoundHalBufferWin::radSoundHalBufferWin
//========================================================================

radSoundHalBufferWin::radSoundHalBufferWin(	void )
    :
    m_SizeInFrames( 0 ),
    m_LoadStartInBytes( 0 ),
    m_pLockedLoadBuffer( NULL ),
    m_LockedLoadBytes( 0 ),
    m_Looping( false ),
    m_Streaming( false ),
    m_Buffer( 0 ),
    m_refIRadSoundHalAudioFormat( NULL ),
    m_refIRadMemoryObject( NULL ),
    m_refIRadSoundHalBufferLoadCallback( NULL )
{
}

//========================================================================
// radSoundHalBufferWin::Initialize
//========================================================================

void radSoundHalBufferWin::Initialize
(
	IRadSoundHalAudioFormat * pIRadSoundHalAudioFormat,
    IRadMemoryObject * pIRadMemoryObject,
    unsigned int sizeInFrames,
	bool looping,
    bool streaming
)
{
    rAssert( pIRadSoundHalAudioFormat != NULL );
    rAssert( pIRadSoundHalAudioFormat->GetEncoding() == IRadSoundHalAudioFormat::PCM );
	rAssert( pIRadMemoryObject->GetMemorySize( ) >= ::radSoundHalBufferCalculateMemorySize( 
        IRadSoundHalAudioFormat::Bytes, sizeInFrames, 
        IRadSoundHalAudioFormat::Frames, pIRadSoundHalAudioFormat ) );

    m_refIRadSoundHalAudioFormat = pIRadSoundHalAudioFormat;
	m_refIRadMemoryObject = pIRadMemoryObject;
	m_Looping = looping;
    m_Streaming = streaming;
    m_SizeInFrames = sizeInFrames;

    alGenBuffers( 1, &m_Buffer );
    rAssert( alGetError() == AL_NO_ERROR );
    if( streaming )
    {
        ALenum format = AL_FORMAT_MONO8;
        if( m_refIRadSoundHalAudioFormat->GetNumberOfChannels() > 1 )
            format = m_refIRadSoundHalAudioFormat->GetBitResolution() == 8 ? AL_FORMAT_STEREO8 : AL_FORMAT_STEREO16;
        else
            format = m_refIRadSoundHalAudioFormat->GetBitResolution() == 8 ? AL_FORMAT_MONO8 : AL_FORMAT_MONO16;

        radBufferStorageSOFT( m_Buffer, format,
            pIRadMemoryObject->GetMemoryAddress(),
            pIRadMemoryObject->GetMemorySize(),
            m_refIRadSoundHalAudioFormat->GetSampleRate(),
            AL_MAP_WRITE_BIT_SOFT | AL_MAP_PERSISTENT_BIT_SOFT
        );
        rAssert( alGetError() == AL_NO_ERROR );
    }
}

//========================================================================
// radSoundHalBufferWin::GetMemoryObject
//========================================================================

IRadMemoryObject * radSoundHalBufferWin::GetMemoryObject( void )
{
	return m_refIRadMemoryObject;
}

//========================================================================
// radSoundHalBufferWin::Clear
//========================================================================

void radSoundHalBufferWin::ClearAsync
( 
	unsigned int startPositionInFrames,
	unsigned int numberOfFrames,
	IRadSoundHalBufferClearCallback * pIRadSoundHalBufferClearCallback
)
{
    if ( m_Buffer != 0 )
    {
        rAssert(startPositionInFrames < m_SizeInFrames );
        rAssert( ( startPositionInFrames + numberOfFrames ) <= m_SizeInFrames );

        unsigned int offsetInBytes = m_refIRadSoundHalAudioFormat->FramesToBytes( startPositionInFrames );
        unsigned int sizeInBytes = m_refIRadSoundHalAudioFormat->FramesToBytes( numberOfFrames );  

        unsigned char fillChar = ( m_refIRadSoundHalAudioFormat->GetBitResolution( ) == 8 ) ? 128 : 0;

        if( m_Streaming == true )
        {
            void* dataPtr = radMapBufferSOFT( m_Buffer, offsetInBytes, sizeInBytes, AL_MAP_WRITE_BIT_SOFT | AL_MAP_PERSISTENT_BIT_SOFT );

            rAssertMsg( alGetError() == AL_NO_ERROR, "radSoundHalBufferWin::Clear - Lock Failed.\n" );

            if( dataPtr )
            {
                ::memset( dataPtr, fillChar, sizeInBytes );

                radUnmapBufferSOFT( m_Buffer );

                rAssertMsg( alGetError() == AL_NO_ERROR, "radSoundHalBufferWin::Clear - UnLock Failed.\n" );
            }
        }
        else
        {

            ::memset( static_cast<char*>(m_refIRadMemoryObject->GetMemoryAddress()) + offsetInBytes, fillChar, sizeInBytes );

            ALenum format = AL_FORMAT_MONO8;
            if( m_refIRadSoundHalAudioFormat->GetNumberOfChannels() > 1 )
                format = m_refIRadSoundHalAudioFormat->GetBitResolution() == 8 ? AL_FORMAT_STEREO8 : AL_FORMAT_STEREO16;
            else
                format = m_refIRadSoundHalAudioFormat->GetBitResolution() == 8 ? AL_FORMAT_MONO8 : AL_FORMAT_MONO16;

            alBufferData( m_Buffer, format,
                m_refIRadMemoryObject->GetMemoryAddress(),
                m_refIRadMemoryObject->GetMemorySize(),
                m_refIRadSoundHalAudioFormat->GetSampleRate()
            );

            rAssertMsg( alGetError() == AL_NO_ERROR, "radSoundHalBufferWin::Clear Failed.\n" );
        }
    }

	if ( pIRadSoundHalBufferClearCallback != NULL )
	{
		pIRadSoundHalBufferClearCallback->OnBufferClearComplete( );
	}
}

//========================================================================
// radSoundHalBufferWin::GetSizeInBytes
//========================================================================

unsigned int radSoundHalBufferWin::GetSizeInBytes( void )
{
    return m_refIRadSoundHalAudioFormat->FramesToBytes( m_SizeInFrames );
}

//========================================================================
// radSoundHalBufferWin::IsStreaming
//========================================================================

bool radSoundHalBufferWin::IsStreaming( void )
{
    return m_Streaming;
}

//========================================================================
// radSoundHalBufferWin::GetBuffer
//========================================================================

ALuint radSoundHalBufferWin::GetBuffer( void )
{
    rAssert(m_Buffer != 0);
    return m_Buffer;
}

//========================================================================
// radSoundHalBufferWin::GetSizeInFrames
//========================================================================

unsigned int radSoundHalBufferWin::GetSizeInFrames( void )
{
    return m_SizeInFrames;
}

//========================================================================
// radSoundHalBufferWin::GetSoundSampleFormat
//========================================================================
    
IRadSoundHalAudioFormat * radSoundHalBufferWin::GetFormat( void )
{
    return m_refIRadSoundHalAudioFormat;
}

//========================================================================
// radSoundHalBufferWin::LoadAsync
//========================================================================

void radSoundHalBufferWin::LoadAsync
(
	IRadSoundHalDataSource * pIRadSoundHalDataSource,
	unsigned int bufferStartInFrames,
	unsigned int numberOfFrames,
	IRadSoundHalBufferLoadCallback * pIRadSoundHalBufferLoadCallback 
)
{
    // At this time there should really only be one direct sound buffer created and 
    // it should be busy.  Let's verify that.

    rAssert( m_refIRadSoundHalBufferLoadCallback == NULL );

    m_LoadStartInBytes = m_refIRadSoundHalAudioFormat->FramesToBytes( bufferStartInFrames );
    m_refIRadSoundHalBufferLoadCallback = pIRadSoundHalBufferLoadCallback;

    if( m_Streaming == true )
    {
        rAssert( m_pLockedLoadBuffer == NULL );

        m_LockedLoadBytes = m_refIRadSoundHalAudioFormat->FramesToBytes( numberOfFrames );
        m_pLockedLoadBuffer = radMapBufferSOFT( m_Buffer, m_LoadStartInBytes, m_LockedLoadBytes,
            AL_MAP_WRITE_BIT_SOFT | AL_MAP_PERSISTENT_BIT_SOFT );
        ALenum error = alGetError();
        rAssert( error == AL_NO_ERROR );
    }
    else
    {
        m_pLockedLoadBuffer = static_cast<char*>(m_refIRadMemoryObject->GetMemoryAddress()) + m_LoadStartInBytes;
        m_LockedLoadBytes = m_refIRadSoundHalAudioFormat->FramesToBytes( numberOfFrames );
    }

    new( "radSoundBufferLoaderWin", RADMEMORY_ALLOC_TEMP ) radSoundBufferLoaderWin(
        static_cast< IRadSoundHalBuffer * >( this ),
        m_pLockedLoadBuffer,
        pIRadSoundHalDataSource,
        m_refIRadSoundHalAudioFormat,
        m_refIRadSoundHalAudioFormat->BytesToFrames( m_LockedLoadBytes ),
        this );
}

//========================================================================
// radSoundHalBufferWin::IsLooping
//========================================================================

bool radSoundHalBufferWin::IsLooping
(
    void
)
{
    return m_Looping;
}   

//========================================================================
// radSoundHalBufferWin::OnBufferLoadComplete
//========================================================================

void radSoundHalBufferWin::OnBufferLoadComplete( unsigned int dataSourceFrames )
{
    rAssert( m_refIRadSoundHalBufferLoadCallback != NULL );

    ALenum format = AL_FORMAT_MONO8;
    if (m_refIRadSoundHalAudioFormat->GetNumberOfChannels() > 1)
        format = m_refIRadSoundHalAudioFormat->GetBitResolution() == 8 ? AL_FORMAT_STEREO8 : AL_FORMAT_STEREO16;
    else
        format = m_refIRadSoundHalAudioFormat->GetBitResolution() == 8 ? AL_FORMAT_MONO8 : AL_FORMAT_MONO16;

    if( m_Streaming == false )
    {
        alBufferData( m_Buffer, format,
            m_pLockedLoadBuffer,
            m_LockedLoadBytes,
            m_refIRadSoundHalAudioFormat->GetSampleRate()
        );
        rAssert( alGetError() == AL_NO_ERROR );
    }
    else
    {
        // For streaming sounds we'll have to unlock the direct sound buffer before calling back

        radUnmapBufferSOFT( m_Buffer );
        rAssert( alGetError() == AL_NO_ERROR );

        m_pLockedLoadBuffer = NULL;
        m_LockedLoadBytes = 0;
    }

    m_pLockedLoadBuffer = NULL;
    m_LockedLoadBytes = 0;

    if( m_refIRadSoundHalBufferLoadCallback != NULL )
    {
        m_refIRadSoundHalBufferLoadCallback->OnBufferLoadComplete( dataSourceFrames );
        m_refIRadSoundHalBufferLoadCallback = NULL;
    }
}

//========================================================================
// radSoundHalBufferWin::CancelAsyncOperations
//========================================================================

void radSoundHalBufferWin::CancelAsyncOperations( void )
{
    radSoundBufferLoaderWin::CancelOperations( static_cast< IRadSoundHalBuffer * >( this ) );    

    if( m_refIRadSoundHalBufferLoadCallback != NULL )
    {
        if( m_Streaming == true )
        {
            radUnmapBufferSOFT( m_Buffer );
            rAssert( alGetError() == AL_NO_ERROR );
        }

        m_pLockedLoadBuffer = NULL;
        m_LockedLoadBytes = 0;
        m_refIRadSoundHalBufferLoadCallback = NULL;
    }
}

//========================================================================
// radSoundHalBufferWin::GetMinTransferSizeInFrames
//========================================================================

unsigned int radSoundHalBufferWin::GetMinTransferSize( IRadSoundHalAudioFormat::SizeType sizeType )
{
    rAssert( m_refIRadSoundHalAudioFormat != NULL );

    //
    // Channels of data are eventually dma'd seperately to spu.
    // Therefore, transfers must occur in multiples of the optimal
    // dma multiple * the number of channels.
    //

    return m_refIRadSoundHalAudioFormat->ConvertSizeType( sizeType, 
        radMemorySpace_OptimalMultiple * m_refIRadSoundHalAudioFormat->GetNumberOfChannels( ),
        IRadSoundHalAudioFormat::Bytes );
}

//========================================================================
// ::radSoundCreateBufferWin
//========================================================================

IRadSoundHalBuffer * radSoundHalBufferCreate( radMemoryAllocator allocator )
{
	return new ( "radSoundHalBufferWin", allocator ) radSoundHalBufferWin( );
}

