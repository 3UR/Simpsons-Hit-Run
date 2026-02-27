//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include <radstring.hpp>
#include <radsound.hpp>
#include "radsoundupdatableobject.hpp"

struct radSoundBufferedDataSource
	:
	public IRadSoundBufferedDataSource,
	public IRadSoundHalDataSourceCallback,
	public radSoundUpdatableObject
{

	IMPLEMENT_REFCOUNTED( "radSoundBufferedDataSource" )

	radSoundBufferedDataSource( void );
	virtual ~radSoundBufferedDataSource( void );
	
	// IRadSoundBufferedDataSource
	
	void Initialize( radMemorySpace bufferSpace, IRadMemoryAllocator * pIRadMemoryAllocator,
		unsigned int bufferSize, IRadSoundHalAudioFormat::SizeType sizeType,
        IRadSoundHalAudioFormat * pIRshaf,
        const char * pIdentifier );

	void SetInputDataSource( IRadSoundHalDataSource * pIRadSoundHalDataSource );
	IRadSoundHalDataSource * GetInputDataSource( void );

    virtual void SetLowWaterMark( float lowWaterMark );
    virtual float GetLowWaterMark( void );

	// IRadSoundHalDataSource

	virtual IRadSoundHalDataSource::State GetState( void );
	virtual IRadSoundHalAudioFormat * GetFormat( void );
	virtual unsigned GetRemainingFrames( void );

	virtual void GetFramesAsync( void * pBytes, radMemorySpace destinationMemorySpace, unsigned int size, IRadSoundHalDataSourceCallback * pIRshdsc );

	// radSoundUpdatableObject

	virtual void Update( unsigned int elapsedTime );

	// IRadSoundHalDataSourceCallback

	virtual void OnDataSourceFramesLoaded( unsigned int framesActuallyRead );

	// IRadMemoryOperationCallback

	virtual void OnMemoryCopyAsyncComplete( void * pUserData );
	
    virtual const char * GetName( void ) { return "BufferedDataSource"; }	

    virtual bool IsBufferFull( void );
    		
	private:

        void AllocateResources( void );

		void Service( void );
		void ServiceInitializingSource( void );
		void ServiceCopy( void );
		void ServiceRead( void );

		void GetFramesInternal( void * pBytes, unsigned int size );

		unsigned int GetAvailableFrames( void );
		
		enum { NONE, INITIALIZING, INITIALIZED, REINITIALIZING } m_State;

		// Init info

		unsigned int m_InitSize;
		IRadSoundHalAudioFormat::SizeType m_InitSizeType;
		
		// Buffer info

		radMemorySpace	   m_FrameBufferMemorySpace;
		radRef< IRadMemoryAllocator > m_xIRadMemoryAllocator_FrameBuffer;

		char *			m_pFrameBuffer;
		unsigned int	m_BufferSizeInFrames;
		unsigned int	m_StartOfDataInFrames;
		unsigned int	m_EndOfDataInFrames;
		bool			m_QueueFull;

		// Copy info

		unsigned int	m_FullCopySize;
		unsigned int	m_FramesLeftToCopy;
		unsigned int    m_CurrentFramesToCopy;
		char *			m_pCurrentCopyPointer;
		radMemorySpace	m_CopyMemorySpace;
		radRef< IRadMemorySpaceCopyRequest > m_xICopyRequest;
		radRef< IRadSoundHalDataSourceCallback > m_xIRadSoundHalDataSourceCallback;

		// Read info

		unsigned int m_ReadSizeInFrames;
		bool m_OutOfData;
        float m_LowWaterMark;

		radRef< IRadSoundHalAudioFormat > m_xIRadSoundHalAudioFormat;
		radRef< IRadSoundHalDataSource >  m_xIRadSoundHalDataSource;
        radRef< IRadSoundHalDataSource >  m_xIRadSoundHalDataSource_ReInit;
        radRef< IRadString >              m_xIRadString_Name;
};