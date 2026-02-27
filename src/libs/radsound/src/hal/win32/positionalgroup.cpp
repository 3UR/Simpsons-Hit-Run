//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include "pch.hpp"
#include "radsoundwin.hpp"
#include "positionalgroup.hpp"
#include "../common/rolloff.hpp"

//========================================================================
// radSoundHalPositionalGroup::radSoundHalPositionalGroup
//========================================================================

radSoundHalPositionalGroup::radSoundHalPositionalGroup( void )
	:
	m_pRadSoundHalPositionalEntity_Head( NULL ),
	m_Position(),
	m_Velocity(),
	m_Direction( 0.0f, 0.0f, 1.0f ),
	m_ConeInnerAngle( 0.0f ),
	m_ConeOuterAngle( 0.0f ),
	m_ConeOuterGain( 1.0f ),
	m_ReferenceDistance( 1.0f ),
	m_MaxDistance( 100.0f )
{
}

//========================================================================
// radSoundHalPositionalGroup::~radSoundHalPositionalGroup
//========================================================================

radSoundHalPositionalGroup::~radSoundHalPositionalGroup( void )
{
	rAssert( m_pRadSoundHalPositionalEntity_Head == NULL );
}

//========================================================================
// radSoundHalPositionalGroup::SetPosition
//========================================================================

void radSoundHalPositionalGroup::SetPosition
(
    radSoundVector * pPosition
)
{
	rAssert( pPosition != NULL );

	m_Position = *pPosition;
}

//========================================================================
// radSoundHalPositionalGroup::GetPosition
//========================================================================

void radSoundHalPositionalGroup::GetPosition
(
    radSoundVector * pPosition
)
{
    rAssert( pPosition != NULL );

	*pPosition = m_Position;
}

//========================================================================
// radSoundHalPositionalGroup::SetVelocity
//========================================================================

void radSoundHalPositionalGroup::SetVelocity
(
    radSoundVector * pVelocity
)
{
	rAssert( pVelocity != NULL );

	#ifdef RAD_DEBUG
	
		if ( pVelocity->GetLength( ) > 100.0f )
		{
			rDebugPrintf( "This radSoundPositionalGroup is going really really fast\n" );
		}
	
	#endif

	m_Velocity = *pVelocity;


}

//========================================================================
// radSoundHalPositionalGroup::GetVelocity
//========================================================================

void radSoundHalPositionalGroup::GetVelocity
(
    radSoundVector * pVelocity
)
{
    rAssert( pVelocity != NULL );

	*pVelocity = m_Velocity;
}

//========================================================================
// radSoundHalPositionalGroup::SetOrientationFront
//========================================================================

void radSoundHalPositionalGroup::SetOrientation
(
    radSoundVector * pOrientationFront, radSoundVector * pOrientationTop
)
{	
	rAssert( pOrientationFront != NULL );
	rAssert( pOrientationTop != NULL );

	m_Direction = *pOrientationFront;
}
   
//========================================================================
// radSoundHalPositionalGroup::GetOrientationFront
//========================================================================

void radSoundHalPositionalGroup::GetOrientation
(
    radSoundVector * pOrientationFront,
	radSoundVector * pOrientationTop
)
{
	if ( pOrientationTop != NULL )
	{
		pOrientationTop->SetElements( 0.0f, 1.0f, 0.0f );
	}
	
	if ( pOrientationFront != NULL )
	{
		*pOrientationFront = m_Direction;
	}
}

//========================================================================
// radSoundHalPositionalGroup::SetInsideConeAngle
//========================================================================

void radSoundHalPositionalGroup::SetConeAngles
(
    float insideConeAngle,
    float outsideConeAngle
)
{
    m_ConeInnerAngle = insideConeAngle;
	m_ConeOuterAngle = outsideConeAngle;
}

//========================================================================
// radSoundHalPositionalGroup::GetInsideConeAngle
//========================================================================

void radSoundHalPositionalGroup::GetConeAngles
(
    float * pIca,
    float * pOca
)
{
    if ( pIca != NULL )
    {    
        *pIca = m_ConeInnerAngle;
    }
    
    if ( pOca != NULL )
    {
        *pOca = m_ConeOuterAngle;
    }
}

//========================================================================
// radSoundHalPositionalGroup::SetConeOutsideVolume
//========================================================================

void radSoundHalPositionalGroup::SetConeOutsideVolume
(
    float coneOutsideVolume
)
{
	signed long hardwareVol =
		::radSoundVolumeDbToHardwareWin(
			::radSoundVolumeAnalogToDb( coneOutsideVolume ) );

	m_ConeOuterGain = hardwareVol;
}

//========================================================================
// radSoundHalPositionalGroup::GetConeOutsideVolume
//========================================================================

float radSoundHalPositionalGroup::GetConeOutsideVolume
(
    void
)
{
	return radSoundVolumeDbToAnalog(
		::radSoundVolumeHardwareToDbWin( m_ConeOuterGain ) );
}

//========================================================================
// radSoundHalPositionalGroup::SetMaxDistance
//========================================================================

void radSoundHalPositionalGroup::SetMinMaxDistance
(
    float minDistance,
    float maxDistance
)
{
	// Kept here only for reference.  Windows rolloff calculation sucks
	// so we'll do it ourselves
	m_ReferenceDistance = minDistance;
	m_MaxDistance = maxDistance;
}

//========================================================================
// radSoundHalPositionalGroup::GetMaxDistance
//========================================================================

void radSoundHalPositionalGroup::GetMinMaxDistance
(
    float * pMinDistance,
    float * pMaxDistance

)
{
	if ( pMinDistance != NULL )
	{
		* pMinDistance = m_ReferenceDistance;
	}

	if ( pMaxDistance != NULL )
	{
		* pMaxDistance = m_MaxDistance;
	}
}

//========================================================================
// radSoundHalPositionalGroup::AddPositionalEnity
//========================================================================

void radSoundHalPositionalGroup::AddPositionalEntity
(
	radSoundHalPositionalEntity * pRadSoundHalPositionalEntity
)
{
	rAssert( pRadSoundHalPositionalEntity );

    pRadSoundHalPositionalEntity->m_pNext = m_pRadSoundHalPositionalEntity_Head;

    if ( pRadSoundHalPositionalEntity->m_pNext != NULL )
    {
        pRadSoundHalPositionalEntity->m_pNext->m_pPrev = pRadSoundHalPositionalEntity;
    }

    pRadSoundHalPositionalEntity->m_pPrev = NULL;

	m_pRadSoundHalPositionalEntity_Head = pRadSoundHalPositionalEntity;

}

//========================================================================
// radSoundHalPositionalGroup::RemovePositionalEntity
//========================================================================

void radSoundHalPositionalGroup::RemovePositionalEntity
(
	radSoundHalPositionalEntity * pRadSoundHalPositionalEntity
)
{
	rAssert( pRadSoundHalPositionalEntity );

    if ( pRadSoundHalPositionalEntity->m_pPrev != NULL )
    {
        pRadSoundHalPositionalEntity->m_pPrev->m_pNext =
			pRadSoundHalPositionalEntity->m_pNext;
    }
    else
    {
        m_pRadSoundHalPositionalEntity_Head = pRadSoundHalPositionalEntity->m_pNext;
    }

    if ( pRadSoundHalPositionalEntity->m_pNext != NULL )
    {
        pRadSoundHalPositionalEntity->m_pNext->m_pPrev = pRadSoundHalPositionalEntity->m_pPrev;
    }
}

//========================================================================
// radSoundHalPositionalGroup::UpdatePositionalSettings
//========================================================================

void radSoundHalPositionalGroup::UpdatePositionalSettings( float listenerRolloffFactor )
{
	radSoundHalPositionalEntity * p = m_pRadSoundHalPositionalEntity_Head;

	while ( p != NULL )
	{
		p->OnApplyPositionalInfo(listenerRolloffFactor);

		p = p->m_pNext;
	}
}

//========================================================================
// ::radSoundhalPositionalGroupCreate
//========================================================================

IRadSoundHalPositionalGroup * radSoundHalPositionalGroupCreate( radMemoryAllocator allocator )
{
	return new ( "radSoundHalPositionalGroup", allocator ) radSoundHalPositionalGroup( );
}


//========================================================================
// Static member definitions
//========================================================================

template<> radSoundHalPositionalGroup * radLinkedClass< radSoundHalPositionalGroup >::s_pLinkedClassHead = NULL;
template<> radSoundHalPositionalGroup * radLinkedClass< radSoundHalPositionalGroup >::s_pLinkedClassTail = NULL;
