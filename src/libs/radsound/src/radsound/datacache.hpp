//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef RADSOUNDDATACACHE_HPP
#define RADSOUNDDATACACHE_HPP

#include <radstring.hpp>
#include <radsound.hpp>
#include <radsoundupdatableobject.hpp>

struct radSoundDataCache
	:
	public IRadSoundDataCache,
    public IRadSoundHalDataSourceCallback,
	public radSoundUpdatableObject
{
	IMPLEMENT_REFCOUNTED( "radSoundDataCache" )

	radSoundDataCache( void );
	virtual ~radSoundDataCache( void );

    // IRadSoundDataCache

    virtual void InitializeAsync(
        radMemorySpace memorySpace,
        IRadMemoryAllocator * pIRadMemoryAllocator,
        IRadSoundHalDataSource * pIRshds,
        const char * pIdentifier );

    virtual IRadSoundHalDataSource * CreateDataSource(
        unsigned int startOnFrame,
        radMemoryAllocator a );

    IRadSoundDataCache::State GetState( void );

    virtual bool WillRelease( void ) { return GetRefCount( ) <= 1; }
    
    inline IRadSoundHalAudioFormat       * GetAudioFormat( void );
    inline IRadSoundMemorySpaceObject    * GetMemorySpaceObject( void );
    
    

    private:

        virtual void OnDataSourceFramesLoaded( unsigned int framesActuallyRead );

        // IRadTimerCallback

        virtual void Update( unsigned int elapsed );

        IRadSoundDataCache::State m_State;
        radMemorySpace                    m_RadMemorySpace;
        radRef< IRadMemoryAllocator >        m_xIRadMemoryAllocator;
        radRef< IRadSoundHalAudioFormat >    m_xIRadSoundHalAudioFormat;
        radRef< IRadSoundMemorySpaceObject > m_xIRadSoundMemorySpaceObject;
        radRef< IRadSoundHalDataSource >     m_xIRadSoundHalDataSource;
        radRef< IRadString >                 m_xIRadString_Name;
};

inline IRadSoundHalAudioFormat * radSoundDataCache::GetAudioFormat( void )
{
    rAssert( m_State == IRadSoundDataCache::Initialized );
    
    return m_xIRadSoundHalAudioFormat;
}

inline IRadSoundMemorySpaceObject * radSoundDataCache::GetMemorySpaceObject( void )
{
    rAssert( m_State == IRadSoundDataCache::Initialized );

    return m_xIRadSoundMemorySpaceObject;
}


struct radSoundDataCacheDataSource
    :
    public IRadSoundHalDataSource,
    public radSoundObject
{
    IMPLEMENT_REFCOUNTED( "radSoundDataCacheDataSource" )

    radSoundDataCacheDataSource(
        radSoundDataCache * pRadSoundDataCache,
        unsigned int startOnFrame );

    // IRadSoundHalDataSource

	virtual State GetState( void );
	virtual IRadSoundHalAudioFormat * GetFormat( void );
	virtual unsigned int GetRemainingFrames( void );
	virtual unsigned int GetAvailableFrames( void ) { return GetRemainingFrames( ); }
	virtual void GetFramesAsync( 
		void * pBytes, 
		radMemorySpace destinationMemorySpace, 
		unsigned int numberOfFrames,
		IRadSoundHalDataSourceCallback * pCallback );

    virtual const char * GetName( void ) { return "DataCacheDataSource"; }		

    public:

        unsigned int m_StartFrame;

        radRef< radSoundDataCache >         m_xRadSoundDataCache;
        radRef< IRadSoundMemoryDataSource > m_xIRadSoundMemoryDataSource;

};

#endif