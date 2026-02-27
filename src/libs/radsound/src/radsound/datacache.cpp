//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include "pch.hpp"
#include "datacache.hpp"

radSoundDataCache::radSoundDataCache( void )
    :
    m_State( IRadSoundDataCache::Idle )
{
    ::radStringCreate( & m_xIRadString_Name, GetThisAllocator( ) );
}

radSoundDataCache::~radSoundDataCache( void )
{
}


void radSoundDataCache::InitializeAsync(
        radMemorySpace memorySpace,
        IRadMemoryAllocator * pIRadMemoryAllocator,
        IRadSoundHalDataSource * pIRshds,
        const char * pIdentifier )
{
    rAssert( m_State == IRadSoundDataCache::Idle );
    rAssert( pIRshds != NULL );
    rAssert( pIRadMemoryAllocator != NULL );
    rAssertMsg( pIdentifier != NULL, "You MUST name your sound data cache objects so we can track memory" );

    m_State = IRadSoundDataCache::Initializing;

    m_xIRadSoundHalDataSource = pIRshds;
    m_xIRadMemoryAllocator = pIRadMemoryAllocator;
    m_RadMemorySpace = memorySpace;
    m_xIRadString_Name->Copy( pIdentifier );

    // Wait for source to initialize

    AddToUpdateList( );

    Update( 0 );
}

void radSoundDataCache::Update( unsigned int elapsed  )
{
    if ( m_xIRadSoundHalDataSource->GetState( ) == IRadSoundHalDataSource::Initialized )
    {
        rAssert( m_xIRadSoundHalDataSource->GetRemainingFrames( ) != 0xFFFFFFFF );

        m_xIRadSoundMemorySpaceObject = ::radSoundMemorySpaceObjectCreate(
            GetThisAllocator( ) );

        m_xIRadSoundHalAudioFormat = m_xIRadSoundHalDataSource->GetFormat( );

        m_xIRadSoundMemorySpaceObject->Initialize(
            m_RadMemorySpace,
            m_xIRadMemoryAllocator,
            m_xIRadSoundHalAudioFormat->FramesToBytes( m_xIRadSoundHalDataSource->GetRemainingFrames( ) ),
            radSoundHalDataSourceReadAlignmentGet( ),
            m_xIRadString_Name->GetChars( ) );

        m_xIRadSoundHalDataSource->GetFramesAsync(
            m_xIRadSoundMemorySpaceObject->GetAddress( ),
            m_xIRadSoundMemorySpaceObject->GetMemorySpace( ),
            m_xIRadSoundHalDataSource->GetRemainingFrames( ),
            this );

        RemoveFromUpdateList( );
 
    }
    else if ( m_xIRadSoundHalDataSource->GetState( ) == IRadSoundHalDataSource::Error )
    {
        m_State = IRadSoundDataCache::Error;
 
        m_xIRadSoundHalDataSource = NULL;
        RemoveFromUpdateList( );
    }
}

void radSoundDataCache::OnDataSourceFramesLoaded( unsigned int framesActuallyRead )
{
    m_State = IRadSoundDataCache::Initialized;
 
    m_xIRadSoundHalDataSource = NULL;
}

IRadSoundHalDataSource * radSoundDataCache::CreateDataSource(
    unsigned int startOnFrame,
    radMemoryAllocator allocator )
{    
    // This can be called at any time, the data source doesn't have to be initialized,
    // yet.

    char name[ 256 ];
    strcpy( name, "radSoundDataCacheDataSource - " );
    strcat( name, m_xIRadString_Name->GetChars( ) );
    
    return new ( name, allocator ) radSoundDataCacheDataSource( this, startOnFrame );
}

IRadSoundDataCache::State radSoundDataCache::GetState( void )
{
    return m_State;
}

IRadSoundDataCache * radSoundDataCacheCreate( radMemoryAllocator a )
{
    return new ( "radSoundDataCache", a ) radSoundDataCache( );
}

//=============================================================================
//
//=============================================================================

radSoundDataCacheDataSource::radSoundDataCacheDataSource(
    radSoundDataCache * pRadSoundDataCache,
    unsigned int startOnFrame )
    :
    m_StartFrame( startOnFrame ),
    m_xRadSoundDataCache( pRadSoundDataCache )
{
    rAssert( pRadSoundDataCache != NULL );
}

IRadSoundHalDataSource::State radSoundDataCacheDataSource::GetState( void )
{
    if ( m_xIRadSoundMemoryDataSource != NULL )
    {
        return m_xIRadSoundMemoryDataSource->GetState( );
    }
    else if ( m_xRadSoundDataCache->GetState( ) == IRadSoundDataCache::Initialized )
    {
        unsigned int startBytes = m_xRadSoundDataCache->GetAudioFormat( )->FramesToBytes( m_StartFrame );
        unsigned int sizeInBytes = m_xRadSoundDataCache->GetMemorySpaceObject( )->GetSizeInBytes( );
        unsigned int bytesToEnd = sizeInBytes - startBytes;

        m_xIRadSoundMemoryDataSource = ::radSoundMemoryDataSourceCreate( GetThisAllocator( ) );
        m_xIRadSoundMemoryDataSource->Initialize(
            m_xRadSoundDataCache->GetAudioFormat( ),
            m_xRadSoundDataCache->GetMemorySpaceObject( ),
            startBytes,
            bytesToEnd,
            IRadSoundHalAudioFormat::Bytes );

        return m_xIRadSoundMemoryDataSource->GetState( );
    }

    return IRadSoundHalDataSource::Initializing;
}

IRadSoundHalAudioFormat * radSoundDataCacheDataSource::GetFormat( void )
{
    return m_xIRadSoundMemoryDataSource->GetFormat( );
}

unsigned int radSoundDataCacheDataSource::GetRemainingFrames( void )
{
    return m_xIRadSoundMemoryDataSource->GetRemainingFrames( );
}

void radSoundDataCacheDataSource::GetFramesAsync( 
		void * pBytes, 
		radMemorySpace destinationMemorySpace, 
		unsigned int numberOfFrames,
		IRadSoundHalDataSourceCallback * pCallback )
{
    m_xIRadSoundMemoryDataSource->GetFramesAsync(
        pBytes,
        destinationMemorySpace,
        numberOfFrames,
        pCallback );
}


