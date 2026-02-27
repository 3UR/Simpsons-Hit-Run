//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        softwarelistener.cpp
//
// Subsystem:	Radical Sound System
//
// Description:	Listener object for positional sound
//
// Creation TH
//
//=============================================================================

//============================================================================
// Include Files
//============================================================================
#include "pch.hpp"
#include <float.h>
#include <radobjectlist.hpp>
#include "softwarelistener.hpp"

#include <stdio.h>

#define EPSILON 0.0001 

//============================================================================
// Static definitions
//============================================================================

/* static */ radSoundHalListener * radSoundHalListener::s_pTheRadSoundSoftwareListener = NULL;

//============================================================================
// Component: radSoundHalListener
//============================================================================


//========================================================================
// radSoundHalListener::radSoundHalListener
//========================================================================

radSoundHalListener::radSoundHalListener( radMemoryAllocator allocator )
    :
    m_OrientationFront( 0.0f, 0.0f, 1.0f ),
    m_OrientationTop(0.0f, 1.0f, 0.0f ),
    m_DistanceFactor( 1.0f ), // meters == meters
    m_DoppleFactor( 1.0f ),   // normal doppler
    m_RollOffFactor( 1.0f ),   // normal roll-off
    m_SurroundSupported( false ),
    m_EnvEffectsEnabled( false ),
    m_EnvAuxSend( NULL_ENV_AUX )
{
    s_pTheRadSoundSoftwareListener = this;

    ::radSoundHalCreateRollOffTable( STD_ROLL_OFF_TABLE_MAX_DIST_VOL, s_pRollOffTable, STD_ROLL_OFF_TABLE_NUM_POINTS );
    ::radSoundHalSetRollOffTable( s_pRollOffTable, STD_ROLL_OFF_TABLE_NUM_POINTS );
}

//========================================================================
// radSoundHalListener::~radSoundHalListener
//========================================================================

radSoundHalListener::~radSoundHalListener( void )
{
    s_pTheRadSoundSoftwareListener = NULL;
}

//========================================================================
// radSoundHalListener::SetPosition
//========================================================================

/* virtual */ void  radSoundHalListener::SetPosition( radSoundVector * pP )
{
	m_Position = *pP;
}

//========================================================================
// radSoundHalListener::GetPosition
//========================================================================

/* virtual */ void  radSoundHalListener::GetPosition( radSoundVector * pP)
{
    *pP = m_Position;
}

//========================================================================
// radSoundHalListener::SetVelocity
//========================================================================

/* virtual */ void  radSoundHalListener::SetVelocity( radSoundVector * pV )
{
    m_Velocity = *pV;
}

//========================================================================
// radSoundHalListener::GetVelocity
//========================================================================

/* virtual */ void  radSoundHalListener::GetVelocity( radSoundVector * pV )
{
    *pV = m_Velocity;
}

//========================================================================
// radSoundHalListener::SetOrientation
//========================================================================

/* virtual */ void radSoundHalListener::SetOrientation( radSoundVector * pFront, radSoundVector * pTop )
{
    if ( pFront != NULL )
    {
        m_OrientationFront = *pFront;
    }
    
    if ( pTop != NULL )
    {
        m_OrientationTop = *pTop;
    }    
}

//========================================================================
// radSoundHalListener::GetOrientation
//========================================================================

/* virtual */ void  radSoundHalListener::GetOrientation( radSoundVector * pFront, radSoundVector * pTop )
{
    if ( pFront != NULL )
    {
        *pFront = m_OrientationFront;
    }

    if ( pTop != NULL )
    {
        *pTop = m_OrientationTop;
    }
}

//========================================================================
// radSoundHalListener::SetDistanceFactor
//========================================================================

/* virtual */ void radSoundHalListener::SetDistanceFactor( float df )
{
    m_DistanceFactor = df;
}

//========================================================================
// radSoundHalListener::GetDistanceFactor
//========================================================================

/* virtual */ float radSoundHalListener::GetDistanceFactor( void )
{
    return m_DistanceFactor;
}

//========================================================================
// radSoundHalListener::SetDoppleFactor
//========================================================================

/* virtual */ void  radSoundHalListener::SetDopplerFactor( float df )
{
    m_DoppleFactor = df;
}

//========================================================================
// radSoundHalListener::GetDoppleFactor
//========================================================================

/* virtual */ float radSoundHalListener::GetDopplerFactor( void )
{
    return m_DoppleFactor;
}

//========================================================================
// radSoundHalListener::SetRollOffFactor
//========================================================================

/* virtual */ void  radSoundHalListener::SetRollOffFactor( float rof )
{
    m_RollOffFactor = rof;
}

//========================================================================
// radSoundHalListener::GetRoll
//========================================================================

/* virtual */ float radSoundHalListener::GetRollOffFactor( void )
{
    return m_RollOffFactor;
}

//========================================================================
// radSoundHalListener::SetEnvEffectsEnabled
//========================================================================

/* virtual */ void radSoundHalListener::SetEnvEffectsEnabled( bool enabled )
{
    m_EnvEffectsEnabled = enabled;
}

//========================================================================
// radSoundHalListener::GetEnvEffectsEnabled
//========================================================================

/* virtual */ bool radSoundHalListener::GetEnvEffectsEnabled( void )
{
    return m_EnvEffectsEnabled;
}

//========================================================================
// radSoundHalListener::SetEnvironmentAuxSend
//========================================================================

/* virtual */ void radSoundHalListener::SetEnvironmentAuxSend( unsigned int auxsend )
{
    m_EnvAuxSend = auxsend;
}

//========================================================================
// radSoundHalListener::GetEnvironmentAuxSend
//========================================================================

/* virtual */ unsigned int radSoundHalListener::GetEnvironmentAuxSend( void )
{
    return m_EnvAuxSend;
}

//=========================================================================
// radSoundHalListener::CalculatePositionalPitchFactor
//=========================================================================

float radSoundHalListener::CalculatePositionalPitchFactor
(
    const radSoundHalPositionalInformation * pSpi

)
{
    // The right way...the above calcualtion sounds ok, but is more
    // acureate as t tends towards zero.

    float relativeSpeed;

    if ( m_Position != pSpi->m_Position )
    {
        float a = m_Velocity.m_x - pSpi->m_Velocity.m_x;
        float b = m_Velocity.m_y - pSpi->m_Velocity.m_y;
        float c = m_Velocity.m_z - pSpi->m_Velocity.m_z;
        float d = m_Position.m_x - pSpi->m_Position.m_x;
        float e = m_Position.m_y - pSpi->m_Position.m_y;
        float f = m_Position.m_z - pSpi->m_Position.m_z;

        relativeSpeed = -( ( (a * d) + (b * e) + (c * f) ) / radSoundSqrt( (d * d) + (e * e) + (f * f) ) );
    }
    else
    {
        relativeSpeed = 0.0f;
    }

    //
    // The above calculation assumed meters/second.  Convert this to game units.
    // If the game specified .38 (feet / per meter ) Then the relative speed
    // would be less meters per second, which is correct because a foot is
    // smaller than a meter.
    //

    relativeSpeed *= m_DistanceFactor;
    //
    // The dopple factor exagerates or lessens the doppler effect, so we'll just
    // adjust the speed by that factor.  Thus, we are saying that the 10x dopple
    // effect is like the objects are travelling 10x faster.
    // 

    relativeSpeed *= m_DoppleFactor;

    //
    // Doppler Formula:
    //
    //
    //                         v            f: the frequency detected by
    //                   f=  ------ fs             the listener
    //                        v-vs
    //                                      fs: the actual frequency 
    //                                             produced by the source
    //
    //                                      v: the velocity of sound: 
    //                                      [345 m/sec]
    //
    //                                      vs: the velocity of the source    
    //
    // Proof:
    //
    // So if the relative speeds are 0     then --> 1
    //                               100    then --> 1.4
    //                               200    then --> 2.37
    //                               300    then --> 7.6
    //                               343    then --> 172.5
    //                               344    then --> 345.0
    //                               344.5  then --> 690.0
    //                              >345.0  then --> undefined ( no sound / infinately high )
    //                              -100    then --> 0.71
    //                              -200    then --> 0.42
    //                              -300    then --> 0.13
    //                             <-345.0  then --> undefined ( no sound / infinately low )
    //                              ...etc
    //

    //
    // Now we know how many meters/second the objects are moving away/apart
    //

    float pitchFactor;

    if ( relativeSpeed > 0.0f )
    {
        //
        // Cap it because > 345.0f is undefined
        //

        if ( relativeSpeed >= 345.0f )
        {
            relativeSpeed = 344.99f;
        }

        pitchFactor = 345.0f / ( 345.0f - relativeSpeed );
    }
    else if ( relativeSpeed < 0.0f )
    {
        if ( relativeSpeed <= -345.0f )
        {
            relativeSpeed = -344.99f;
        }
        pitchFactor = 1.0f / ( 345.0f / ( 345.0f + relativeSpeed ) );
    }
    else
    {

        pitchFactor = 1.0f;
    }               

    rAssert( pitchFactor >= 0.0f );

    return pitchFactor;
}

inline void radSoundHalListener::ClampPan( float * pPan )
{
    if ( *pPan > 1.0 )
    {
        rAssert( *pPan <= ( 1.0f + EPSILON ) );
        *pPan = 1.0f;
    }
    else if ( *pPan < -1.0f )
    {
        rAssert( *pPan >= ( -1.0f - EPSILON ) );
        *pPan = -1.0f;        
    }
}

//=========================================================================
// radSoundHalListener::CalculatePositionalPanFactor
//=========================================================================
// Description: Calculates the positional panning needed to produce
//              a 3D sound.
//
// Parameters:  pRsspi - a pointer to the positional information
//              pLeftRightPan - (out) the left and right pan value
//              pFrontBackPAn - (out) the front and back pan value
//
// Returns:     n/a
//=========================================================================

void radSoundHalListener::CalculatePositionalPanFactor
(
    const radSoundHalPositionalInformation* pRsspi,
    float* pLeftRightPan,
    float* pFrontBackPan
)
{
    //
    // The pan factor various from -1.0f (left ear) to +1.0f (right ear)
    //
    // In essence, we are getting angle between the front vector and a vector
    // from the listener to the source.  The pan factor is just the cosine
    // of this vector.
    // More specifically, we are projecting the sound source vector onto
    // a vector passing through the ears.
    // eqn.
    //      pan = 1 - { acos[ ( v dot s ) / ( len(v) * len(s) ) ] * 2 / PI }
    //
    float panFactor = 0.0f;

    if ( m_Position != pRsspi->m_Position )
    {
        radSoundVector source = m_Position.GetVectorFrom( pRsspi->m_Position );
        radSoundVector rightEar = m_OrientationFront.Cross( m_OrientationTop );
        float cosPanFactor = source.Dot( rightEar ) / ( source.GetLength( ) * rightEar.GetLength( ) );

        // 
        // Don't need to convert to an angle, we want to pan based on
        // the cos, not the angle (we need the "circular") power curve
        //            
        // panFactor = 1.0f - ( radSoundACos( cosPanFactor ) * 2 / 3.141592655f );
        
        panFactor = cosPanFactor;
    
    }
    else
    {
        panFactor = 0.0f;
    }

    //
    // Set the left and right pan
    //
    (*pLeftRightPan) = panFactor;

    if ( m_SurroundSupported == true )
    {        
        //
        // Now, the front and back pan is done in the same way, but using the front vector
        // instead of the right ear.
        //
        if ( m_Position != pRsspi->m_Position )
        {
            radSoundVector source = m_Position.GetVectorFrom( pRsspi->m_Position );
            float cosPanFactor =
                - source.Dot( m_OrientationFront ) / ( source.GetLength( ) * m_OrientationFront.GetLength( ) );
            // 
            // Don't need to convert to an angle, we want to pan based on
            // the cos, not the angle (we need the "circular") power curve
            //
            
            // panFactor = 1.0f - ( radSoundACos( cosPanFactor ) * 2 / 3.141592655f );
            
            panFactor = cosPanFactor; 
        }
        else
        {
            panFactor = 0.0f;
        }

        //
        // Set the front and back pan
        //
        (*pFrontBackPan) = panFactor;
    }
    else
    {
        *pFrontBackPan = 0.0f;
    }

    ClampPan( pFrontBackPan );
    ClampPan( pLeftRightPan );
    
    radSoundVerifyAnalogPan( *pFrontBackPan );
    radSoundVerifyAnalogPan( *pLeftRightPan );
}
//=========================================================================
// radSoundHalListener::CalculatePositionalVolumeFactor
//=========================================================================

float radSoundHalListener::CalculatePositionalVolumeFactor
(
    const radSoundHalPositionalInformation * pSpi
)
{
    float coneVolumeFactor = 1.0f;

    //==================================
    // C O N E  V O L U M E  F A C T O R
    //==================================

    float angleToCone = 0.0f;

    // Avoid divide by zero

    if ( m_Position != pSpi->m_Position )
    {
        angleToCone = pSpi->m_OrientationFront.GetAngleBetween( m_Position.GetVectorFrom( pSpi->m_Position ) );
    }
    else
    {
        angleToCone = 0.0f;
    }

    if ( angleToCone <= pSpi->m_ConeInsideAngle )
    {
        //
        // Here we are inside the inner cone, so we don't scale the volume
        //
        coneVolumeFactor = 1.0f;
    }
    else if ( ( angleToCone > pSpi->m_ConeInsideAngle ) &&
              ( angleToCone < pSpi->m_ConeOutsideAngle ) )
    {
        //
        // here we are between the inner and outer cones so we scale the volume
        // from 1.0f --> coneOutsideVolume

        float percentThroughTransition = ( angleToCone - pSpi->m_ConeInsideAngle ) / 
                                         ( pSpi->m_ConeOutsideAngle - pSpi->m_ConeInsideAngle );    

        coneVolumeFactor = percentThroughTransition * ( 1.0f - pSpi->m_ConeOutsideVolume );
    }
    else
    {
        //
        // Here we are outside the outer cone, so the volume is just the
        // outside cone volume.
        //

        coneVolumeFactor = pSpi->m_ConeOutsideVolume;
    }     

    //==========================================
    // D I S T A N C E  V O L U M E  F A C T O R
    //==========================================

    //
    // We now use a simple fall-off between min and max distance.
    //

    float distanceToObject = m_Position.GetDistanceBetween( pSpi->m_Position );

    float distanceVolumeFactor = radSoundHalCalcualteRollOff(
        pSpi->m_MinDistance, pSpi->m_MaxDistance, distanceToObject );

    float finalVol = distanceVolumeFactor * coneVolumeFactor;

    //======================================
    // B E H I N D  V O L U M E  F A C T O R
    //======================================

    const float InFrontVolumeFactor = 1.0f;
    const float BehindVolumeFactor = 0.5f;

    const float CosBehindListenerAngle = -0.58f;

    //
    // Calculate the cos of the angle between the source and the listener
    // in the plane of the listener's head (normal is top vector).
    //
    radSoundVector source = m_Position.GetVectorFrom( pSpi->m_Position );
    float cosAngle = - source.Dot( m_OrientationFront ) / ( source.GetLength( ) * m_OrientationFront.GetLength( ) );

    //
    // Depending on the angle (we keep the cosine to avoid extra computation, nd becuase it is semetric),
    // we should compute the volume.
    // If the disance is very small, we cannot pretend to know the direction of the object, so we
    // can assume it is in front of us.
    //
    if( distanceToObject == 0.0f || cosAngle > 0.0f )
    {
        //
        // In front of listener
        //
        finalVol *= InFrontVolumeFactor;
    }
    else if( cosAngle > CosBehindListenerAngle )
    {
        //
        // Not quite behind the listener
        //
        float change = InFrontVolumeFactor - ( ( InFrontVolumeFactor - BehindVolumeFactor ) * cosAngle / CosBehindListenerAngle );
        finalVol *= change;
    }
    else
    {
        //
        // Behind the listener
        //
        finalVol *= BehindVolumeFactor;
    }

    //
    // Return the volume
    //

    radSoundVerifyAmplitudeVolume( finalVol );

    return finalVol;
}

//=========================================================================
// radSoundHalListener::AddPositionalEntity
//=========================================================================

///* virtual */ void radSoundHalListener::AddSoftwarePositionalEntity
//(
//    IRadSoundSoftwarePositionalEntity * pIRsspe
//)
//{
//   radRef< IRadWeakInterfaceWrapper > xIRadWeakInterfaceWrapper;
//
//    ::radWeakInterfaceWrapperCreate( & xIRadWeakInterfaceWrapper, GetThisAllocator( ) );
//    
//    xIRadWeakInterfaceWrapper->SetWeakInterface( pIRsspe );
//
//    m_xIObjectList_PositionalVoices->AddObject( xIRadWeakInterfaceWrapper );
//}

//=========================================================================
// radSoundHalListener::RemovePositionalEntity
//=========================================================================

///* virtual */ void radSoundHalListener::RemoveSoftwarePositionalEntity
//(
//    IRadSoundSoftwarePositionalEntity * pIRsspe
//)
//{
//    IRadWeakInterfaceWrapper * pIRadWeakInterfaceWrappper;
//
//    m_xIObjectList_PositionalVoices->Reset( );
//    
//    while ( pIRadWeakInterfaceWrappper = reinterpret_cast< IRadWeakInterfaceWrapper * >( m_xIObjectList_PositionalVoices->GetNext( ) ) )
//    {
//        IRadSoundSoftwarePositionalEntity * pTest = reinterpret_cast< IRadSoundSoftwarePositionalEntity * >( pIRadWeakInterfaceWrappper->GetWeakInterface( ) );
//        
//        if ( pTest == pIRsspe )
//        {
//            m_xIObjectList_PositionalVoices->RemoveObject( pIRadWeakInterfaceWrappper );
//            return;
//        }
//    }
//
//    rAssert( false );
//}

//=========================================================================
// radSoundHalListener::UpdatePositionalSettings
//=========================================================================

/* virtual */ void radSoundHalListener::UpdatePositionalSettings
(
    void
)
{
	radSoundHalPositionalInformation * pRadSoundHalPositionalInformation = NULL;
	
	radSoundHalPositionalGroup * pRadSoundHalPositionalGroup =
		radSoundHalPositionalGroup::GetLinkedClassHead( );

    //
    // Loop through the positional group and calculate
    // new pan, volume and pitch settings.
    //
	
	while ( pRadSoundHalPositionalGroup != NULL )
	{
        //
        // Get a positional information structure pointer, this is for speed
        // so we don't have to call GetThis(), GetThat(), etc. NULL if 
        // the item is not active
        //

		CalculatePositionalInformation( & pRadSoundHalPositionalGroup->m_RadSoundHalPositionalInformation );

		//
		// Inform the positional group of our calculations
		//

		pRadSoundHalPositionalGroup->UpdatePositionalSettings(  );

		pRadSoundHalPositionalGroup = pRadSoundHalPositionalGroup->GetLinkedClassNext( );
	}
}


//=========================================================================
// radSoundHalListener::CalculatePositionalInformation
//=========================================================================

/* virtual */ void radSoundHalListener::CalculatePositionalInformation( radSoundHalPositionalInformation * pRsspi )
{
    //
    // Call our helper functions to calculate each attribute
    // 
    pRsspi->m_VolumeAdjust = CalculatePositionalVolumeFactor( pRsspi );
    CalculatePositionalPanFactor
    (
        pRsspi,
        &pRsspi->m_PanAdjust,
        &pRsspi->m_FrontBackPanAdjust
    );
    pRsspi->m_PitchAdjust = CalculatePositionalPitchFactor( pRsspi );
    pRsspi->m_EnvAuxSend = ( m_EnvEffectsEnabled ? m_EnvAuxSend : NULL_ENV_AUX );
}

//=========================================================================
// radSoundHalListener::SetSupportSurround
//=========================================================================

/* virtual */ void radSoundHalListener::SetSupportSurround( bool support )
{
    m_SurroundSupported = support;
}

//=========================================================================
// radSoundHalListener::GetSupportSurround
//=========================================================================

/* virtual */ bool radSoundHalListener::GetSupportSurround( void )
{
    return m_SurroundSupported;
}

//=========================================================================
// radSoundHalListener::GetInstance
//=========================================================================

/* static */ radSoundHalListener * radSoundHalListener::GetInstance( void )
{
	rAssert( s_pTheRadSoundSoftwareListener != NULL );

	return s_pTheRadSoundSoftwareListener;
}

//=========================================================================
// radSoundHalListener::Initialize
//=========================================================================

/* static */ void radSoundHalListener::Initialize( radMemoryAllocator allocator )
{
    rAssert( s_pTheRadSoundSoftwareListener == NULL );

    new( "radSoundHalListener", allocator ) radSoundHalListener( allocator );
    s_pTheRadSoundSoftwareListener->AddRef( );
}

//=========================================================================
// radSoundHalListener::Termintate
//=========================================================================

/* static */ void radSoundHalListener::Terminate( void )
{
    rAssert( s_pTheRadSoundSoftwareListener != NULL );

    radSoundHalListener::s_pTheRadSoundSoftwareListener->Release( );
}

//=========================================================================
// ::radSoundHalListenerGet
//=========================================================================

/* static */ IRadSoundHalListener * radSoundHalListenerGet( void )
{
	return radSoundHalListener::GetInstance( );
}