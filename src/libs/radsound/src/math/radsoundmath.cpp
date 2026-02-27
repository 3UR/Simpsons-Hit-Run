//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        radsoundmath.cpp
//
// Subsystem:	Foundation Technologies - RadSound
//
// Description:	
//
// The following "analog" calculations are based on the DSL concave transform.  
// It is the standard for MIDI instruments:
//
//			attenuation dB = 20 * log10f( 127^2 / volume^2 )
//
// Where volume is measured between 0 and 127.
// 
//
// In our caculations below, analog values range between 0 and 1. In otherwords
// the above equation can be simplified to:
//
//			amplification dB = 20 * log10f( analog^2 )
//
// where analog is measured between 0 and 1.
// 
//
// Note the following conversions:
//
//		amplitude factor = ( analog factor ) ^ 2
//		power factor	 = ( analog factor ) ^ 4
// 
//
//		deciBels = 10 * log( power factor ) = 10 * log( ( amplitude factor ) ^ 2 )
//				 = 20 * log( amplitude factor )
//				 = 40 * log( analog factor )
//
//
//
// Revisions:  	Dec 4, 2001
//
//=============================================================================

//============================================================================
// Include Files
//============================================================================
#include "pch.hpp"
#include <radsoundmath.hpp>

//============================================================================
// Function: radSoundIsSampleRateOptimal
//============================================================================

float acceptableRatios [] = {
    1.0f,
    1.0f / 2.0f, 
    1.0f / 3.0f, 2.0f / 3.0f,
    1.0f / 4.0f,              3.0f / 4.0f,
    1.0f / 5.0f, 2.0f / 5.0f, 3.0f / 5.0f, 4.0f / 5.0f,
    1.0f / 6.0f,                                        5.0f / 6.0f,
    1.0f / 7.0f, 2.0f / 7.0f, 3.0f / 7.0f, 4.0f / 7.0f, 5.0f / 7.0f, 6.0f / 7.0f,
    1.0f / 8.0f,              3.0f / 8.0f,              5.0f / 8.0f,              7.0f / 8.0f,
    1.0f / 9.0f, 2.0f / 9.0f,              4.0f / 9.0f, 5.0f / 9.0f,              7.0f / 9.0f, 8.0f / 9.0f,
    1.0f /10.0f,              3.0f /10.0f,                                        7.0f /10.0f,              9.0f /10.0f,
    1.0f /11.0f, 2.0f /11.0f, 3.0f /11.0f, 4.0f /11.0f, 5.0f /11.0f, 6.0f /11.0f, 7.0f /11.0f, 8.0f /11.0f, 9.0f /11.0f, 10.0f /11.0f,
    1.0f /12.0f,                                        5.0f /12.0f,              7.0f /12.0f,                                         11.0f/12.0f };


bool radSoundIsSampleRateOptimal( float sourceSampleRate, float playbackSampleRate )
{
    // We just want to keep the numerator and denominator small

    float ratio0 = sourceSampleRate / playbackSampleRate;
    float ratio1 = playbackSampleRate / sourceSampleRate;

    // See if either ratio is contained in the list of acceptable ratios

    for( unsigned int i = 0; i < sizeof( acceptableRatios ) / sizeof( float ); i++ )
    {
        if( acceptableRatios[ i ] == ratio0 || acceptableRatios[ i ] == ratio1 )
        {
            return true;
        }
    }

    return false;
}


