//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//
// File:        softwarepositionalgroup.cpp
//
// Subsystem:	Radical Sound System
//
// Description:	Positional group implimenations for platforms without native 
//				positional sound support
//
//=============================================================================

//============================================================================
// Include Files
//============================================================================

#include "pch.hpp"
#include "softwarelistener.hpp"
#include "softwarepositionalgroup.hpp"

//========================================================================
// Static member definitions
//========================================================================

template<> radSoundHalPositionalGroup * radLinkedClass< radSoundHalPositionalGroup >::s_pLinkedClassHead = NULL;
template<> radSoundHalPositionalGroup * radLinkedClass< radSoundHalPositionalGroup >::s_pLinkedClassTail = NULL;
//========================================================================
// ::radSoundhalPositionalGroupCreate
//========================================================================

IRadSoundHalPositionalGroup * radSoundHalPositionalGroupCreate( radMemoryAllocator allocator )
{
	return new ( "radSoundHalPositionalGroup", allocator ) radSoundHalPositionalGroup( );
}

//========================================================================
// radSoundHalPositionalGroup::radSoundHalPositionalGroup
//========================================================================

radSoundHalPositionalGroup::radSoundHalPositionalGroup( void )
	:
	m_pRadSoundHalPositionalEntity_Head( NULL )
{
    ::radSoundHalSystemGet( )->AddRef( );
}

//========================================================================
// radSoundHalPositionalGroup::~radSoundHalPositionalGroup
//========================================================================

radSoundHalPositionalGroup::~radSoundHalPositionalGroup( void )
{
    ::radSoundHalSystemGet( )->Release( );

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

	m_RadSoundHalPositionalInformation.m_Position = * pPosition;
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

	*pPosition = m_RadSoundHalPositionalInformation.m_Position;
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

	m_RadSoundHalPositionalInformation.m_Velocity = * pVelocity;


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

	*pVelocity = m_RadSoundHalPositionalInformation.m_Velocity;
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

	m_RadSoundHalPositionalInformation.m_OrientationFront = * pOrientationFront;
	m_RadSoundHalPositionalInformation.m_OrientationTop = * pOrientationTop;
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
		*pOrientationTop = m_RadSoundHalPositionalInformation.m_OrientationTop;
	}
	
	if ( pOrientationFront != NULL )
	{
		*pOrientationFront = m_RadSoundHalPositionalInformation.m_OrientationFront;
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
    m_RadSoundHalPositionalInformation.m_ConeInsideAngle =  insideConeAngle;
    m_RadSoundHalPositionalInformation.m_ConeOutsideAngle = outsideConeAngle;
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
        *pIca = m_RadSoundHalPositionalInformation.m_ConeInsideAngle;
    }
    
    if ( pOca != NULL )
    {
        *pOca = m_RadSoundHalPositionalInformation.m_ConeOutsideAngle;
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
	m_RadSoundHalPositionalInformation.m_ConeOutsideVolume = coneOutsideVolume;
}

//========================================================================
// radSoundHalPositionalGroup::GetConeOutsideVolume
//========================================================================

float radSoundHalPositionalGroup::GetConeOutsideVolume
(
    void
)
{
	return m_RadSoundHalPositionalInformation.m_ConeOutsideVolume;
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
	m_RadSoundHalPositionalInformation.m_MinDistance = minDistance;
	m_RadSoundHalPositionalInformation.m_MaxDistance = maxDistance;
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
        *pMinDistance = m_RadSoundHalPositionalInformation.m_MinDistance;
	}

	if ( pMaxDistance != NULL )
	{
        *pMaxDistance = m_RadSoundHalPositionalInformation.m_MaxDistance;
	}
}

//========================================================================
// radSoundHalPositionalGroup::SetOcclusion
//========================================================================

void radSoundHalPositionalGroup::SetOcclusion( float occl )
{
    m_RadSoundHalPositionalInformation.m_Occlusion = occl;
}

//========================================================================
// radSoundHalPositionalGroup::GetOcclusion
//========================================================================

float radSoundHalPositionalGroup::GetOcclusion( void )
{
    return m_RadSoundHalPositionalInformation.m_Occlusion;
}

//========================================================================
// radSoundHalPositionalGroup::SetObstruction
//========================================================================

void radSoundHalPositionalGroup::SetObstruction( float obst )
{
    m_RadSoundHalPositionalInformation.m_Obstruction = obst;
}

//========================================================================
// radSoundHalPositionalGroup::GetObstruction
//========================================================================

float radSoundHalPositionalGroup::GetObstruction( void )
{
    return m_RadSoundHalPositionalInformation.m_Obstruction;;
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

void radSoundHalPositionalGroup::UpdatePositionalSettings( void )
{
	radSoundHalPositionalEntity * p = m_pRadSoundHalPositionalEntity_Head;

	while ( p != NULL )
	{
		p->OnApplyPositionalSettings( );

		p = p->m_pNext;
	}
}

//========================================================================
// radSoundHalPositionalGroup::ForceUpdate
//========================================================================

void radSoundHalPositionalGroup::ForceUpdate( void )
{
    radSoundHalListener::GetInstance( )->CalculatePositionalInformation( & m_RadSoundHalPositionalInformation );
}