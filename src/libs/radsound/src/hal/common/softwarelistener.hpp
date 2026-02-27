//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef SOFTWARELISTENER_HPP
#define SOFTWARELISTENER_HPP

#include <float.h>
#include <radsoundobject.hpp>
#include <radsound.hpp>
#include "softwarepositionalgroup.hpp"
#include "rolloff.hpp"

//============================================================================
// Struct radSoundHalListener
//============================================================================

struct radSoundHalListener
	:
	public IRadSoundHalListener,
    public radSoundObject
{
	public:

		IMPLEMENT_REFCOUNTED( "radSoundHalListener" )

		static void Initialize( radMemoryAllocator allocator );
		static radSoundHalListener * GetInstance( void );
		static void Terminate( void );

	    virtual void SetSupportSurround( bool support );
		virtual bool GetSupportSurround( void );

		virtual void SetPosition( radSoundVector * pPos );
		virtual void GetPosition( radSoundVector * pPos );
		virtual void SetVelocity( radSoundVector * pVelocity );
		virtual void GetVelocity( radSoundVector * pVelocity );
		virtual void SetOrientation( radSoundVector * pOrientationFront, radSoundVector * pOrientationTop );
		virtual void GetOrientation( radSoundVector * pOrientationFront, radSoundVector * pOrientationTop );

		virtual void  SetDistanceFactor( float distanceFactor );
		virtual float GetDistanceFactor( void );
		virtual void  SetDopplerFactor( float doppleFactor );
		virtual float GetDopplerFactor( void );
		virtual void  SetRollOffFactor( float rollOffFactor );
		virtual float GetRollOffFactor( void );

        virtual void  SetEnvEffectsEnabled( bool enabled );
        virtual bool  GetEnvEffectsEnabled( void );
        virtual void  SetEnvironmentAuxSend( unsigned int auxsend );
        virtual unsigned int GetEnvironmentAuxSend( void );

		void CalculatePositionalInformation( radSoundHalPositionalInformation * pRadSoundHalPositionalInformation );
		void UpdatePositionalSettings( void );

	private:

		radSoundHalListener( radMemoryAllocator allocator );
		~radSoundHalListener( void );

		float CalculatePositionalPitchFactor( const radSoundHalPositionalInformation * pSpi );
		void  CalculatePositionalPanFactor( const radSoundHalPositionalInformation * pRsspi, float* pLeftRightPan, float* pFrontBackPan );
		float CalculatePositionalVolumeFactor( const radSoundHalPositionalInformation * pSpi );
        
        inline void ClampPan( float * pPan );

	//========================================================================
	// radSoundHalListener:: DataMembers
	//========================================================================

//		ref< IRadObjectList > m_xIObjectList_PositionalVoices;

		radSoundVector m_Position;
		radSoundVector m_Velocity;
		radSoundVector m_OrientationFront;
		radSoundVector m_OrientationTop;
  
		float m_DistanceFactor;
		float m_DoppleFactor;
		float m_RollOffFactor;
    
		bool m_SurroundSupported;

        bool m_EnvEffectsEnabled;
        unsigned int m_EnvAuxSend;

        float s_pRollOffTable[ STD_ROLL_OFF_TABLE_NUM_POINTS ];

		static radSoundHalListener * s_pTheRadSoundSoftwareListener;
};








#endif // SOFTWARELISTENER_HPP