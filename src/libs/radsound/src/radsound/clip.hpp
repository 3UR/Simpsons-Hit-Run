//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//
// File:        clip.cpp
//
// Subsystem:	Radical Sound System
//
// Description:	Gamecube implementation of the clip component.
//
//=============================================================================

#ifndef CLIP_HPP
#define CLIP_HPP

//======================================================================
// class: Include Files
//======================================================================

#include <radstring.hpp>
#include <radsound_hal.hpp>
#include <radsound.hpp>
#include <radlinkedclass.hpp>
#include "radsoundupdatableobject.hpp"

//======================================================================
// class: radSoundClip
//======================================================================

class radSoundClip
	:
    public IRadSoundClip,
	public IRadSoundHalBufferLoadCallback,
	public IRadSoundHalBufferClearCallback,
	public radLinkedClass< radSoundClip >,
	public radSoundUpdatableObject
{
    public:

	    IMPLEMENT_REFCOUNTED( "radSoundClip" )

        // IRadSoundClip

		virtual void Initialize(
			IRadSoundHalDataSource * pIRshds,
            IRadSoundHalMemoryRegion * pIRshmr,
            bool looping,
            const char * pIdentifier );

		virtual IRadSoundHalMemoryRegion * GetMemoryRegion( void );
	    virtual bool  GetLooping( void );

		virtual IRadSoundHalAudioFormat * GetFormat( void );

	    virtual void  SetTrim( float trim );
	    virtual float GetTrim( void );

 	    virtual IRadSoundClip::State GetState( void );

	    virtual IRadSoundHalBuffer * GetBuffer( void );

        virtual bool WillRelease( void );  
    
    private: friend IRadSoundClip * radSoundClipCreate( radMemoryAllocator allocator );

    	radSoundClip( void );
	    ~radSoundClip( void );

        void SetState( IRadSoundClip::State state );

        // radSoundUpdatableObject

		virtual void Update( unsigned int elapsedTime );

        // IRadSoundHalBufferLoadCallback

	    virtual void OnBufferLoadComplete( unsigned int actualBytesRead );

        // IRadSoundHalBufferClearCallback

	    virtual void OnBufferClearComplete( void );

        // Data Members

	    bool m_Looping;
	    float m_Trim;
	    IRadSoundClip::State m_State;
	    
	    radRef< IRadSoundHalMemoryRegion > m_xIRadSoundHalMemoryRegion;
	    radRef< IRadSoundHalBuffer >		m_xIRadSoundHalBuffer;
		radRef< IRadSoundHalDataSource >   m_xIRadSoundHalDataSource;
        radRef< IRadString >               m_xIRadString_Name;
};

#endif // CLIP_HPP
