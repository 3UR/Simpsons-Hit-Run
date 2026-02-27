//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//
// File:        softwarepositionalgroup.hpp
//
// Subsystem:	Radical Sound System
//
// Description:	Positional group implimenations for platforms without native 
//				positional sound support
//
//=============================================================================

#ifndef SOFTWAREPOSITIONALGROUP_HPP
#define SOFTWAREPOSITIONALGROUP_HPP

//=============================================================================
// Include files
//=============================================================================

#include <radlinkedclass.hpp>
#include <radsound_hal.hpp>
#include <float.h>
#include <radsoundobject.hpp>

//============================================================================
// Global Definitions
//============================================================================

#define NULL_ENV_AUX 0xFFFFFFFF

//=============================================================================
// Forward Declarations
//=============================================================================

struct radSoundHalPositionalInformation;
struct radSoundHalPositionalGroup;
struct IRadSoundHalPositionalEntity;

//=============================================================================
// Interface: radSoundHalPositionalEntity
//=============================================================================

struct radSoundHalPositionalEntity
{
	public:

		virtual void OnApplyPositionalSettings( void ) = 0;

	private: friend struct radSoundHalPositionalGroup;

		radSoundHalPositionalEntity * m_pNext;
		radSoundHalPositionalEntity * m_pPrev;
};

//============================================================================
// Struct radSoundPositionalInformation
//============================================================================

struct radSoundHalPositionalInformation
    :
    public radSoundObject
{

    radSoundHalPositionalInformation( void )
        :
        m_Position( 0.0f, 0.0f, 0.0f ),
        m_Velocity( 0.0f, 0.0f, 0.0f ),
        m_OrientationTop( 0.0f, 1.0f, 0.0f ),
        m_OrientationFront( 0.0f, 0.0f, 1.0f ), 
        m_ConeInsideAngle( 360.0f ),
        m_ConeOutsideAngle( 360.0f ),
        m_ConeOutsideVolume( 1.0f ),
        m_MinDistance( 0.0f ),
        m_MaxDistance( 100.0f ),
        m_Occlusion( 0.0f ),
        m_Obstruction( 0.0f ),
        m_EnvAuxSend( NULL_ENV_AUX ),
        m_VolumeAdjust( 1.0f ),
        m_PanAdjust( 0.0f ),
        m_FrontBackPanAdjust( 0.0f ),
        m_PitchAdjust( 1.0f )
    {
        ;
    }

    radSoundVector m_Position;
    radSoundVector m_Velocity;
    radSoundVector m_OrientationTop;
    radSoundVector m_OrientationFront;

    float m_ConeInsideAngle;
    float m_ConeOutsideAngle;
    float m_ConeOutsideVolume;
    float m_MinDistance;
    float m_MaxDistance;

    float m_Occlusion;
    float m_Obstruction;

    unsigned int m_EnvAuxSend;

    //
    // Set by listener
    //

    float m_VolumeAdjust;
    float m_PanAdjust;
    float m_FrontBackPanAdjust;
    float m_PitchAdjust;
};


//============================================================================
// Struct radSoundHalPositionalGroup
//============================================================================

struct radSoundHalPositionalGroup
	:
	public IRadSoundHalPositionalGroup,
	public radLinkedClass< radSoundHalPositionalGroup >,
    public radSoundObject
{
	public:

		IMPLEMENT_REFCOUNTED( "radSoundHalPositionalGroup" )
		
		radSoundHalPositionalGroup( void );
		virtual ~radSoundHalPositionalGroup( void );

		void AddPositionalEntity( radSoundHalPositionalEntity * pRadSoundHalPositionalEntity );
		void RemovePositionalEntity( radSoundHalPositionalEntity * pRadSoundHalPositionalEntity );

        // Called by listener after calculations are done

		void UpdatePositionalSettings( void );

        // Called by voice to force a re-calculation

        void ForceUpdate( void );

		// IRadSoundHalPositionalGroup

		virtual void  SetPosition( radSoundVector * pPosition );
		virtual void  GetPosition( radSoundVector * pPosition );
		virtual void  SetVelocity( radSoundVector * pVelocity );
		virtual void  GetVelocity( radSoundVector * pVelocity );
		virtual void  SetOrientation( radSoundVector * pFront, radSoundVector * pTop );
		virtual void  GetOrientation( radSoundVector * pFront, radSoundVector * pTop );
		virtual void  SetMinMaxDistance( float min, float max );
		virtual void  GetMinMaxDistance( float * pMin, float * pMax );
		virtual void  SetConeOutsideVolume( float ov );
		virtual float GetConeOutsideVolume( void );
		virtual void  SetConeAngles( float in, float out );
		virtual void  GetConeAngles( float * pIn, float * pOut );
        virtual void  SetOcclusion( float occl );
        virtual float GetOcclusion( void );
        virtual void  SetObstruction( float obst );
        virtual float GetObstruction( void );

		radSoundHalPositionalInformation m_RadSoundHalPositionalInformation;
		
		radSoundHalPositionalEntity * m_pRadSoundHalPositionalEntity_Head;
};

#endif // SOFTWAREPOSITIONALGROUP_HPP