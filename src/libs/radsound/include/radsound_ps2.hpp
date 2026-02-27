//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef RADSOUND_PS2_HPP
#define RADSOUND_PS2_HPP

#include <radsound_hal.hpp>

struct IRadSoundEffectPs2;

IRadSoundEffectPs2 * radSoundCreateEffectPs2( radMemoryAllocator allocator = RADMEMORY_ALLOC_DEFAULT );

struct IRadSoundEffectPs2
	:
	public IRadSoundHalEffect
{
	enum Mode
	{
		Off,
		Room,
		StudioA,
		StudioB,
		StudioC,
		Hall,
		Space,
		Echo,
		Delay,
		Pipe
	};

	virtual void SetEnabled( bool enabled ) = 0;
	virtual bool GetEnabled( void ) = 0;

	virtual void SetGain( float gain ) = 0;
	virtual float GetGain( void ) = 0;

	virtual void SetMode( IRadSoundEffectPs2::Mode mode ) = 0;
	virtual IRadSoundEffectPs2::Mode GetMode( void ) = 0;

    //
    // Delay and Feedback are active only in Echo and Delay mode.
    // Both are measured from 0.0f to 1.0f
    //
 
	virtual void SetDelay( float delayTime ) = 0;
	virtual float GetDelayTime( void ) = 0;

	virtual void SetFeedback( float feedBack ) = 0;
	virtual float GetFeedback( void ) = 0;
};

#endif