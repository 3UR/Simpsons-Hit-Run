//=============================================================================
// Copyright (C) 2002 Radical Entertainment Ltd.  All rights reserved.
//
// File:        carsoundparameters.h
//
// Description: Declaration for RadScript-created class for tunable car sound
//              parameters
//
// History:     30/06/2002 + Created -- Darren
//
//=============================================================================

#ifndef CARSOUNDPARAMETERS_H
#define CARSOUNDPARAMETERS_H

//========================================
// Nested Includes
//========================================

#include <radobject.hpp>
#include <radlinkedclass.hpp>

#include <sound/avatar/icarsoundparameters.h>

//========================================
// Forward References
//========================================

//=============================================================================
//
// Synopsis:    carSoundParameters
//
//=============================================================================

class carSoundParameters: public ICarSoundParameters,
                          public radLinkedClass< carSoundParameters >,
                          public radRefCount
{
    public:
        IMPLEMENT_REFCOUNTED( "carSoundParameters" );

        static const int REVERSE_GEAR = -1;
        
        carSoundParameters();
        virtual ~carSoundParameters();

        void SetWatcherName( const char* name );

        ICarSoundParameters& SetClipRPM( float rpm ) { m_clipRPM = rpm; return *this; }
        float GetClipRPM() { return( m_clipRPM ); }

        ICarSoundParameters& SetEngineClipName( const char* clipName );
        const char* GetEngineClipName() { return( m_engineClipName ); }

        ICarSoundParameters& SetEngineIdleClipName( const char* clipName );
        const char* GetEngineIdleClipName() { return( m_idleClipName ); }

        ICarSoundParameters& SetDamagedEngineClipName( const char* clipName );
        const char* GetDamagedEngineClipName() { return( m_damagedClipName ); }

        ICarSoundParameters& SetHornClipName( const char* clipName );
        const char* GetHornClipName() { return( m_hornClipName ); }

        ICarSoundParameters& SetCarDoorOpenClipName( const char* clipName );
        const char* GetCarDoorOpenClipName() { return( m_carOpenClipName ); }

        ICarSoundParameters& SetCarDoorCloseClipName( const char* clipName );
        const char* GetCarDoorCloseClipName() { return( m_carCloseClipName ); }

        ICarSoundParameters& SetOverlayClipName( const char* clipName );
        const char* GetOverlayClipName() { return( m_overlayClipName ); }

        ICarSoundParameters& SetRoadSkidClipName( const char* clipName );
        const char* GetRoadSkidClipName() { return( m_roadSkidClipName ); }

        ICarSoundParameters& SetDirtSkidClipName( const char* clipName );
        const char* GetDirtSkidClipName() { return( m_dirtSkidClipName ); }

        ICarSoundParameters& SetBackupClipName( const char* clipName );
        const char* GetBackupClipName() { return( m_backupClipName ); }

        ICarSoundParameters& SetShiftPoint( unsigned int gear, float percent );
        float GetShiftPoint( int gear );

        ICarSoundParameters& SetDownshiftDamperSize( float percent );
        float GetDownshiftDamperSize() { return( m_downshiftDamper ); }

        ICarSoundParameters& SetGearPitchRange( unsigned int gear, float min, float max );

        ICarSoundParameters& SetNumberOfGears( unsigned int gear );

        float GetMsecsPerOctaveCap() { return( m_msecsPerOctave ); }

        //
        // Attack/delay/decay
        //
        ICarSoundParameters& SetAttackTimeMsecs( float msecs );
        float GetAttackTimeMsecs() { return( m_attackTime ); }

        ICarSoundParameters& SetDelayTimeMsecs( unsigned int msecs );
        unsigned int GetDelayTimeMsecs() { return( m_delayTime ); }

        ICarSoundParameters& SetDecayTimeMsecs( float msecs );
        float GetDecayTimeMsecs() { return( m_decayTime ); }

        ICarSoundParameters& SetDecayFinishTrim( float trim );
        float GetDecayFinishTrim() { return( m_decayFinishTrim ); }

        //
        // Reverse
        //
        ICarSoundParameters& SetReversePitchCapKmh( float speed );
        float GetReversePitchCapKmh() { return( m_maxReverseKmh ); }

        ICarSoundParameters& SetReversePitchRange( float min, float max );
        void GetReversePitchRange( float& min, float& max ) 
            { min = m_minReversePitch; max = m_maxReversePitch; }

        //
        // Gear shifts
        //
        ICarSoundParameters& SetGearShiftPitchDrop( unsigned int gear, float drop );
        float GetGearShiftPitchDrop( unsigned int gear );

        //
        // Damage
        //
        ICarSoundParameters& SetDamageStartPcnt( float damagePercent );
        float GetDamageStartPcnt() { return( m_damageStartPcnt ); }

        ICarSoundParameters& SetDamageMaxVolPcnt( float percent );
        float GetDamageVolumeRange() { return( m_damageVolumeRange ); }

        ICarSoundParameters& SetDamageStartTrim( float trim );
        float GetDamageStartTrim() { return( m_damageStartTrim ); }
        ICarSoundParameters& SetDamageMaxTrim( float trim );
        float GetDamageMaxTrim() { return( m_damageMaxTrim ); }

        //
        // Idle
        //
        ICarSoundParameters& SetIdleEnginePitch( float pitch );
        float GetIdleEnginePitch() { return( m_idleEnginePitch ); }

        //
        // In-air sound characteristics
        //
        ICarSoundParameters& SetInAirThrottlePitch( float pitch );
        float GetInAirThrottlePitch();

        ICarSoundParameters& SetInAirIdlePitch( float pitch );
        float GetInAirIdlePitch();

        ICarSoundParameters& SetInAirThrottleResponseTimeMsecs( unsigned int msecs );
        unsigned int GetInAirThrottleResponseTimeMsecs() { return( m_inAirResponseMsecs ); }

        //
        // Burnout sound characteristics
        //
        ICarSoundParameters& SetBurnoutMinPitch( float pitch );
        float GetBurnoutMinPitch() { return( m_burnoutMinPitch ); }

        ICarSoundParameters& SetBurnoutMaxPitch( float pitch );
        float GetBurnoutMaxPitch() { return( m_burnoutMaxPitch ); }

        //
        // Powerslide sound characteristics
        //
        ICarSoundParameters& SetPowerslideMinPitch( float pitch );
        float GetPowerslideMinPitch();

        ICarSoundParameters& SetPowerslideMaxPitch( float pitch );
        float GetPowerslideMaxPitch();

        //
        // Given a vehicle's current speed and max speed, figure out the
        // gear it's in
        //
        int CalculateCurrentGear( float currentSpeed, float previousSpeed, 
                                  float topSpeed, int previousGear );

        //
        // Given a vehicle's current gear, speed, and max speed, figure out the
        // pitch for the engine clip
        //
        float CalculateEnginePitch( int gear, float currentSpeed, float topSpeed );

        float GetRevLimit();

        //
        // Create a CarSoundParameters object
        //
        static carSoundParameters* ObjCreate( radMemoryAllocator allocator );

    private:
        //Prevent wasteful constructor creation.
        carSoundParameters( const carSoundParameters& original );
        carSoundParameters& operator=( const carSoundParameters& rhs );

        static const unsigned int MAX_GEARS = 6;

        float m_clipRPM;

        float m_minPitch[MAX_GEARS];
        float m_maxPitch[MAX_GEARS];

        char* m_engineClipName;
        char* m_idleClipName;
        char* m_damagedClipName;
        char* m_hornClipName;
        char* m_carOpenClipName;
        char* m_carCloseClipName;
        char* m_overlayClipName;
        char* m_roadSkidClipName;
        char* m_dirtSkidClipName;
        char* m_backupClipName;

        //
        // Percentages of top speed at which we shift gears.
        // E.g. m_shiftPoints[0] == 0.01f means that we shift from idle
        // to first at 1% of top speed.  m_shiftPoints[MAX_GEARS+1] is
        // 1.0f to simplify math.
        //

        // Used for shifting
        float m_shiftPoints[MAX_GEARS+1];
        float m_downshiftDamper;

        //
        // Attack/delay/decay
        //
        float m_attackTime;
        unsigned int m_delayTime;
        float m_decayTime;

        float m_decayFinishTrim;

        //
        // Reverse
        //
        float m_maxReverseKmh;
        float m_minReversePitch;
        float m_maxReversePitch;

        //
        // Gear shifts
        //
        float m_gearShiftPitchDrop[MAX_GEARS];
        unsigned int m_gearShiftTimeMsecs[MAX_GEARS];

        //
        // Damage
        //
        float m_damageStartPcnt;
        float m_damageVolumeRange;

        float m_damageStartTrim;
        float m_damageMaxTrim;

        //
        // Idle
        //
        float m_idleEnginePitch;

        //
        // Airborne
        //
        float m_inAirThrottlePitch;
        float m_inAirIdlePitch;
        unsigned int m_inAirResponseMsecs;

        //
        // Burnout
        //
        float m_burnoutMinPitch;
        float m_burnoutMaxPitch;

        //
        // Powerslide
        //
        float m_powerslideMinPitch;
        float m_powerslideMaxPitch;

        //
        // Makes transitions a little less abrupt
        //
        float m_msecsPerOctave;
};

#endif // CARSOUNDPARAMETERS_H

