//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef MEMORYDATASOURCE_HPP
#define MEMORYDATASOURCE_HPP

#include <radsound.hpp>
#include <radtime.hpp>
#include <radsoundupdatableobject.hpp>

struct radSoundMemoryDataSource
	:
	public IRadSoundMemoryDataSource,
	public radSoundUpdatableObject
{
	IMPLEMENT_REFCOUNTED( "radSoundMemoryBuffer" )

	radSoundMemoryDataSource( void );
	virtual ~radSoundMemoryDataSource( void );

	// IRadSoundMemoryDataSource

    virtual void Initialize(
        IRadSoundHalAudioFormat * pIRshaf,
        IRadSoundMemorySpaceObject * pIRmso,
        unsigned int startOffset,
        unsigned int size,   
        IRadSoundHalAudioFormat::SizeType st);

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
		
    virtual const char * GetName( void ) { return "MemoryDataSource"; }		

	private:

        // radSoundUpdatableObject

        virtual void Update( unsigned int elapsedTime );

        unsigned int m_StartOffsetInBytes;
        unsigned int m_LengthInFrames;
        unsigned int m_NumberOfFramesRead;
        unsigned int m_FramesToRead;

        radRef< IRadSoundMemorySpaceObject >       m_xIRadSoundMemorySpaceObject;
        radRef< IRadSoundHalAudioFormat >          m_xIRadSoundHalAudioFormat;
        radRef< IRadMemorySpaceCopyRequest >       m_xIRadMemorySpaceCopyRequest;
        radRef< IRadSoundHalDataSourceCallback >   m_xIRadSoundHalDataSourceCallback;

};

#endif // MEMORYDATASOURCE_HPP