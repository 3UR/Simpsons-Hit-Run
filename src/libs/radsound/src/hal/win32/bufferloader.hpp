//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef BUFFERLOADER_HPP
#define BUFFERLOADER_HPP

#include <AL/al.h>
#include <radfile.hpp>
#include <radlinkedclass.hpp>
#include <radsound_hal.hpp>  
#include <radsoundobject.hpp>

//=============================================================================
// Component: radSoundBufferLoaderWin
//=============================================================================

class radSoundBufferLoaderWin
	:
    public IRadSoundHalDataSourceCallback,
    public radLinkedClass< radSoundBufferLoaderWin >,
    public radSoundObject
{
    public:

        IMPLEMENT_REFCOUNTED( "radSoundBufferLoaderWin" )

        radSoundBufferLoaderWin(
            IRefCount * pIRefCount_Owner,
            void * pBuffer,
		    IRadSoundHalDataSource * pIRadSoundHalDataSource,
            IRadSoundHalAudioFormat * pIRadSoundHalAudioFormat,
            unsigned int numberOfFrames,
            IRadSoundHalBufferLoadCallback * pISoundBufferCallback );

        virtual void OnDataSourceFramesLoaded( unsigned int framesActuallyRead );

    static void CancelOperations( IRefCount * pIRefCount_Owner );

    void Cancel( void );

    private:
        
        void Start( void );
        void Finish( void );

        radRef< IRadSoundHalBufferLoadCallback >	m_xIRadSoundHalBufferLoadCallback;
        radRef< IRadSoundHalDataSource >			m_xIRadSoundHalDataSource;
        radRef< IRefCount >                        m_xIRefCount_Owner;

        unsigned int m_NumberOfFrames;
        void * m_pBuffer;
        bool m_Cancelled;
};

#endif // BUFFERLOADER_HPP