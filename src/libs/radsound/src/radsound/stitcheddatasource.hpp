//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef STITCHEDDATASOURCE_HPP
#define STITCHEDDATASOURCE_HPP

#include <radsound.hpp>
#include <radsoundupdatableobject.hpp>

class radSoundStitchedDataSource
	:
	public IRadSoundStitchedDataSource,
	public IRadSoundHalDataSourceCallback,
	public radSoundUpdatableObject
{
	public:

		IMPLEMENT_REFCOUNTED( "radSoundStitchedDataSource" )

		radSoundStitchedDataSource( void );
		virtual ~radSoundStitchedDataSource( void );

		// IRadSoundStitchedDataSource

		virtual void InitializeFromAudioFormat(
			IRadSoundHalAudioFormat * pIRshaf );

		virtual void InitializeFromDataSource( IRadSoundHalDataSource *
			pIRadSoundHalDataSource );

		// Make sure you set this to NULL when you release the
		// stiched data source.

		virtual void SetStitchCallback( IRadSoundStitchCallback * pStitchCallback,
			void * pUserData );

        virtual void ResetAudioFormat( IRadSoundHalAudioFormat * );
		virtual void Reset( void );

		// IRadSoundHalDataSource

		virtual IRadSoundHalDataSource::State GetState( void );

		virtual IRadSoundHalAudioFormat * GetFormat( void );

		virtual unsigned int GetRemainingFrames( void );
		
		virtual unsigned int GetAvailableFrames( void ) { return 0xFFFFFFFF; }

		virtual void GetFramesAsync( 
			void * pBytes, 
			radMemorySpace destinationMemorySpace,
			unsigned int numberOfFrames,
			IRadSoundHalDataSourceCallback * pCallback );

		// radSoundUpdatableObject

		virtual void Update( unsigned int elapsedTime );
		
		virtual const char * GetName( void ) { return "MemoryDataSource"; }

		// IRadSoundHalDataSourceCallback

		virtual void OnDataSourceFramesLoaded( unsigned int framesRead );


	private:

		void Service( void );


        unsigned int m_FrameCount;
        radMemorySpace m_ReadMemorySpace;
		void *		 m_pCurrentReadPointer;	 // currently reading to this addr
		unsigned int m_FramesLeftToRead;     // frames left of full read
		unsigned int m_FullReadSize;         // the complete read size.
        bool         m_LoadOutstanding;      // DataSourceCallback currently pending         

        radRef< IRadSoundHalDataSourceCallback > m_refIRadSoundHalDataSourceCallback_Cancelled;

		void * m_pRadSoundStitchCallbackUserData;

		IRadSoundStitchCallback	*			  m_pIRadSoundStitchCallback; // Weak
		radRef< IRadSoundHalAudioFormat >        m_xIRadSoundHalAudioFormat;
		radRef< IRadSoundHalDataSourceCallback > m_xIRadSoundHalDataSourceCallback;
		radRef< IRadSoundHalDataSource >         m_xIRadSoundHalDataSource;
};

#endif