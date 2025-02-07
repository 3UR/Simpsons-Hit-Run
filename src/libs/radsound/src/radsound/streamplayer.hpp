//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include <radstring.hpp>
#include <radsound.hpp>
#include <radobjectlist.hpp>
#include <radsoundupdatableobject.hpp>

#define RSD_STREAM_NUM_BUFFERS 2

struct radSoundStreamPlayer 
	:
	public IRadSoundStreamPlayer,
	public IRadSoundHalBufferLoadCallback,
    public IRadSoundHalBufferClearCallback,
	public radSoundUpdatableObject
{
	IMPLEMENT_REFCOUNTED( "radSoundStreamPlayer" )

	radSoundStreamPlayer( void );
	virtual ~radSoundStreamPlayer( void );

	// IRadSoundPlayer

    virtual void  Play( void );
    virtual void  Stop( void );
	virtual bool  IsPlaying( void );

	virtual void SetPriority( unsigned int priority );
	virtual unsigned int GetPriority( void );

    virtual void  SetMuted( bool );
    virtual bool  GetMuted( void );
    virtual void  SetVolume( float );
    virtual float GetVolume( void );
    virtual void  SetTrim( float );
    virtual float GetTrim( void ); 
    virtual void  SetPitch( float );
    virtual float GetPitch( void );
    
    virtual void  SetPan( float );
    virtual float GetPan( void );

	virtual void  SetAuxMode( unsigned int aux, radSoundAuxMode mode );
	virtual radSoundAuxMode GetAuxMode( unsigned int aux );

	virtual void  SetAuxGain( unsigned int aux, float gain );
	virtual float GetAuxGain( unsigned int aux );

	virtual void SetPositionalGroup( IRadSoundHalPositionalGroup * pIRshpg );
	virtual IRadSoundHalPositionalGroup * GetPositionalGroup( void );
	
	virtual unsigned int GetPlaybackTimeInSamples( void );	 

	// IRadSoundStreamPlayer

	virtual void Initialize(
		IRadSoundHalAudioFormat * pIRadSoundHalAudioFormat,
		unsigned int size, IRadSoundHalAudioFormat::SizeType,
		IRadSoundHalMemoryRegion * pIRadSoundHalMemoryRegion,
        const char * pIdentifier );

	virtual void InitializeAsync(
		unsigned int size, IRadSoundHalAudioFormat::SizeType sizeType,
		IRadSoundHalMemoryRegion * pIRadSoundHalMemoryRegion,
        const char * pIdentifier );

    IRadSoundHalAudioFormat * GetFormat( void );

	virtual void SetDataSource( IRadSoundHalDataSource * pIRadSoundHalDataSource );
	virtual IRadSoundHalDataSource * GetDataSource( void );

    virtual State GetState( );

	// radSoundUpdatableObject

	virtual void Update( unsigned int elapsed );

	// IRadSoundHalBufferLoadCallback

	virtual void OnBufferLoadComplete(
		IRadSoundHalBuffer * pIRadSoundHalBuffer,
		unsigned int dataSourceFrames );
    virtual void OnBufferClearComplete( IRadSoundHalBuffer * pIRadSoundHalBuffer );

	private:

        void StopVoice( bool playingSilence );
        void ServiceLoad( void );
        bool ServicePlay( void );
        void ServiceStateMachine( void );
		void SetState( IRadSoundStreamPlayer::State state );
		void AllocateResources( IRadSoundHalAudioFormat * pIRadSoundHalAudioFormat );

		IRadSoundStreamPlayer::State m_State;

        enum QueueingSubState { Queueing_None,
               Queueing_WaitingForDataSourceToInitialize,
               Queueing_LoadingFirstBlock } m_QueueingSubState;

		unsigned int m_LastPlaybackPositionInSamples;
		unsigned int m_OutstandingLoadSize;
        unsigned int m_OutstandingClearSize;
		unsigned int m_SourceFramesRead;
		unsigned int m_EndFrameCounter;
		unsigned int m_CurrentLoadBuffer;
        bool m_LoadSkipLastFrame;
        bool m_PollSkipLastFrame;
        	
		unsigned int m_InitializeInfo_Size;
		IRadSoundHalAudioFormat::SizeType m_InitializeInfo_SizeType;

		radRef< IRadSoundHalMemoryRegion > m_xInitializeInfo_MemRegion;

		radRef< IRadSoundHalDataSource >  m_xIRadSoundHalDataSource;
		radRef< IRadSoundHalAudioFormat > m_xIRadSoundHalAudioFormat;
		radRef< IRadSoundHalBuffer >      m_xIRadSoundHalBuffers[RSD_STREAM_NUM_BUFFERS];
		radRef< IRadSoundHalVoice >       m_xIRadSoundHalVoice;
        radRef< IRadString >              m_xIRadString_Name;
};