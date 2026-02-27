//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef ROLLOFF_HPP
#define ROLLOFF_HPP

#include <radsoundmath.hpp>

extern float * g_pRotPoints;
extern unsigned int g_NumRotPoints;

#define STD_ROLL_OFF_TABLE_NUM_POINTS 1000
#define STD_ROLL_OFF_TABLE_MAX_DIST_VOL 0.01f

//
// Rolloff Table contains multipliers for the volume amplitude
//

void radSoundHalCreateRollOffTable( float volAtMaxDistance, float * pPoints, unsigned int numPoints );

inline void radSoundHalSetRollOffTable( float * pPoints, unsigned int numPoints )
{
    g_pRotPoints = pPoints;
    g_NumRotPoints = numPoints;
}
inline float radSoundHalCalcualteRollOff( float minDistance, float maxDistance, float distanceToObject )
{
    float distanceVolumeFactor;

    if( distanceToObject <= minDistance )
    {
        distanceVolumeFactor = 1.0f;
    }
    else if ( distanceToObject >= maxDistance )
    {
        distanceVolumeFactor = g_pRotPoints[ g_NumRotPoints - 1 ];
    }
    else
    {
        float range = ( maxDistance - minDistance );
        float distanceThroughRange = ( distanceToObject - minDistance ) / range;

        float index = distanceThroughRange * radSoundUIntToFloat( g_NumRotPoints - 2 );
        
        float whole = radSoundFloor( index );
        float decimal = index - whole;

        unsigned index1 = radSoundFloatToUInt( whole );
        unsigned index2 = index1 + 1;

        rAssert( index1 < g_NumRotPoints );
        rAssert( index2 < g_NumRotPoints );

        float val1 = g_pRotPoints[ index1 ];
        float val2 = g_pRotPoints[ index2 ];

        distanceVolumeFactor = val1 + ( ( val2 - val1 ) * decimal );
    }

    rAssert( distanceVolumeFactor >= 0.0f );
    rAssert( distanceVolumeFactor <= 1.0f );

    return distanceVolumeFactor; // amplitude multiplier
}

#endif // ROLLOFF_HPP
