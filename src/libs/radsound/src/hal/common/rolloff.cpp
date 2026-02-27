//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include "pch.hpp"
#include <radsoundmath.hpp>

float * g_pRotPoints = NULL;
unsigned int g_NumRotPoints = 0;

// #define SPEW_TABLE

void radSoundHalCreateRollOffTable( float inaudible, float * pArray, unsigned int numElements )
{
	rAssert( inaudible > 0 );
	rAssert( inaudible <= 1.0f );
	rAssert( numElements >= 1 );
	rAssert( pArray != 0 );

	for( unsigned int i = 0; i < numElements; i ++ )
	{
		float distance = ( radSoundUIntToFloat( i + 1 ) / radSoundUIntToFloat( numElements ) );

		pArray[ i ] = radSoundPow( inaudible, distance );
	}

	pArray[ numElements - 1 ] = 0.0f;

    #ifdef SPEW_TABLE

        for( unsigned i = 0; i < numElements; i ++ )
        {
            rDebugPrintf( "%3d:[%1.3f] ", i, pArray[ i ] );

            if ( i % 10 == 0 )
            {
                rDebugPrintf( "\n" );
            }
        }

        rDebugPrintf( "\n" );

    #endif // SPEW_TABLE
}