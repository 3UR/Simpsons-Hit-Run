//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include <radsoundobject.hpp>
#include <radsound.hpp>
#include <radfile.hpp>

struct radSoundRsdFileDataSource
	:
	public IRadSoundRsdFileDataSource,
	public IRadFileCompletionCallback,
	public radSoundObject
{

	IMPLEMENT_REFCOUNTED( "radSoundRsdFileDataSource" )

	radSoundRsdFileDataSource( void );
	virtual ~radSoundRsdFileDataSource( void );
	
	// IRadSoundHalFileDataSource
	
	virtual void InitializeFromFile( 
		IRadFile * pIRadFile,
		unsigned int initialPlaybackPosition,
		IRadSoundHalAudioFormat::SizeType positionType,
        IRadSoundHalAudioFormat * pIRadSoundHalAudioFormat = NULL );

	virtual void InitializeFromFileName( 
		const char * pFileName,
		bool highPriority,
		unsigned int initialPlaybackPosition,
		IRadSoundHalAudioFormat::SizeType positionType,
        IRadSoundHalAudioFormat * pIRadSoundHalAudioFormat = NULL );

	// IRadSoundHalDataStream

	virtual IRadSoundHalDataSource::State GetState( void );
	virtual IRadSoundHalAudioFormat * GetFormat( void );
	virtual unsigned GetRemainingFrames( void );

	virtual void GetFramesAsync(
		void * pBytes, 
		radMemorySpace destinationMemorySpace, 
		unsigned int size, 
		IRadSoundHalDataSourceCallback * pIRshdsc );

	// IRadFileCompletionCallback

	virtual void OnFileOperationsComplete( void * pUserData );

    virtual const char* GetName( void ) { return fileName; }
    
	private:

        void InitFile( void );

		void _StateOpeningFile( void );
		void _StateReadingHeader( void );
		void _StateIdle( void );
		void _StateReadingData( void );

		unsigned int GetAvailableFrames( void ) { return 0xFFFFFFFF; }

		unsigned int m_FramesLeftInFile;

		struct StateInfo
		{
			enum State { NONE, INITIALIZED, OPENING_FILE, FILE_ERROR, READING_HEADER, IDLE, READING_DATA } m_State;

			union
			{
			    struct
			    {
			        unsigned int                      m_InitialPlaybackPos;
			        IRadSoundHalAudioFormat::SizeType m_InitalPlaybackPosUnits;
			        bool                              m_HighPriority;
			    } m_InitializedInfo;
                			    			    
				struct
				{
			        unsigned int                      m_InitialPlaybackPos;
			        IRadSoundHalAudioFormat::SizeType m_InitalPlaybackPosUnits;				
					radSoundHalFileHeader * m_pRadSoundHalFileHeader;
				} m_ReadingHeaderInfo;

				struct
				{
					unsigned int m_ReadSizeInFrames;
				} m_ReadingDataInfo;

			};

		} m_StateInfo;

		radRef< IRadSoundHalDataSourceCallback > m_refIRadSoundHalDataSourceCallback;
		radRef< IRadSoundHalAudioFormat >        m_refIRadSoundHalAudioFormat;
		radRef< IRadFile >                       m_refIRadFile;
		
		char fileName[ 64 ];
};