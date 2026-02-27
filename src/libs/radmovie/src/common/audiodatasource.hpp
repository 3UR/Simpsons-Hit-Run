//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        audiodatasource.cpp
//
// Subsystem:	Foundation Technologies - Movie
//
// Description:	This file contains the declaration of the Foundation 
//              Technologies movie player audio file data source
//
// Date:    	June 26, 2002
//
//=============================================================================

#ifndef AUDIODATASOURCE_HPP
#define AUDIODATASOURCE_HPP

//=============================================================================
// Include Files
//=============================================================================

#include <radoptions.hpp>
#include <radobject.hpp>
#include <radsound_hal.hpp>
#include <radsoundfile.hpp>
#include <radfile.hpp>

//=============================================================================
// Forward Declarations
//=============================================================================

class radMovieAudioDataSource;

//=============================================================================
// Factories
//=============================================================================

radMovieAudioDataSource * radMovieAudioDataSourceCreate( radMemoryAllocator allocator );

//=============================================================================
// class radMovieAudioDataSource
//=============================================================================

class radMovieAudioDataSource
	:
	public IRadSoundHalDataSource,
	public IRadFileCompletionCallback,
	public radRefCount
{
    public:

	    IMPLEMENT_REFCOUNTED( "radMovieAudioDataSource" )

	    radMovieAudioDataSource( void );
	    virtual ~radMovieAudioDataSource( void );
	    
        void Initialize( IRadFile * pIRadFile, unsigned int startPosition, unsigned int size );

	    // IRadSoundHalDataSource

	    virtual IRadSoundHalDataSource::State GetState( void );
	    virtual IRadSoundHalAudioFormat * GetFormat( void );
	    virtual unsigned int GetRemainingFrames( void );

	    virtual void GetFramesAsync(
		    void * pBytes, 
		    radMemorySpace destinationMemorySpace, 
		    unsigned int size, 
		    IRadSoundHalDataSourceCallback * pIRshdsc );
		    
		virtual const char * GetName( void ) { return "MovieAudioDataSource"; }

	    // IRadFileCompletionCallback

	    virtual void OnFileOperationsComplete( void * pUserData );

	private:

        void InitFile( void );

		void _StateOpeningFile( void );
		void _StateReadingHeader( void );
		void _StateIdle( void );
		void _StateReadingData( void );

		unsigned int GetAvailableFrames( void ) { return 0xFFFFFFFF; }

        unsigned int m_Size;
		unsigned int m_FramesLeftInFile;
		unsigned int m_StartPosition;

		struct StateInfo
		{
			enum State { NONE, OPENING_FILE, FILE_ERROR, READING_HEADER, IDLE, READING_DATA } m_State;

			union
			{
				struct
				{
					radSoundHalFileHeader * m_pRadSoundHalFileHeader;
				} m_ReadingHeaderInfo;

				struct
				{
					unsigned int m_ReadSizeInFrames;
				} m_ReadingDataInfo;

			};

		} m_StateInfo;

		radRef< IRadSoundHalDataSourceCallback > m_xIRadSoundHalDataSourceCallback;

		radRef< IRadSoundHalAudioFormat > m_xIRadSoundHalAudioFormat;
		radRef< IRadFile >        m_xIRadFile;
};

#endif // AUDIODATASOURCE_HPP