//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include "pch.hpp"
#include "radsoundwin.hpp"
#include <radsound_hal.hpp>

//============================================================================
// ::radSoundFloatAngleToULongWin
//============================================================================

unsigned int radSoundFloatAngleToULongWin
(
    float angle
)
{
    unsigned int uangle = static_cast< unsigned int >( angle );

    if ( uangle > 360 )
    {
        uangle = 360;
    }

    return uangle;
}

//============================================================================
// ::radSoundULongAngleToFloat
//============================================================================

float radSoundULongAngleToFloatWin
(
    unsigned int angle
)
{
    float fangle = static_cast< float >( angle );

    if ( fangle > 360.0f)
    {
        fangle = 360.0f;
    }
    else if ( fangle <= 0.0f )
    {
        fangle = 0.0f;
    }

    return fangle;
}

//============================================================================
// ::radSoundVolumeDbToHardwareWin
//============================================================================

float radSoundVolumeDbToHardwareWin
(
    float decibels
)
{
	radSoundVerifyDbVolume( decibels );

	// We don't want too many bytes or we will overflow.  It's 
	// safe to cut this value off at -100
	
	if( decibels < -100.0f )
	{
        return 0.0f;
	}

	return powf(10.0f, decibels / 20.0f);
}

//============================================================================
// ::radSoundVolumeHardwareToDbWin
//============================================================================

float radSoundVolumeHardwareToDbWin
( 
    float hardwareVolume
)
{
	// Just scale the value by 100

    float decibels = log10f(hardwareVolume) * 20.0f;

	::radSoundVerifyDbVolume( decibels );

	return decibels;
}
