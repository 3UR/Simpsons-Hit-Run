//=============================================================================
// Copyright (C) 2002 Radical Entertainment Ltd.  All rights reserved.
//
// File:        icarsoundparameters.h
//
// Description: Interface declaration for RadScript-created class for 
//              tunable car sound parameters
//
// History:     30/06/2002 + Created -- Darren
//
//=============================================================================

#ifndef ICARSOUNDPARAMETERS_H
#define ICARSOUNDPARAMETERS_H

//========================================
// Nested Includes
//========================================

#include <radobject.hpp>

//========================================
// Forward References
//========================================

//=============================================================================
//
// Synopsis:    ICarSoundParameters
//
//=============================================================================

struct ICarSoundParameters : public IRefCount
{
    //
    // Vehicle RPM at which the engine sound clip should be played unchanged
    //
    virtual ICarSoundParameters& SetClipRPM( float rpm ) = 0;

    //
    // Name of clip to use for engine sound
    //
    virtual ICarSoundParameters& SetEngineClipName( const char* clipName ) = 0;

    //
    // Name of clip to use for engine idle sound
    //
    virtual ICarSoundParameters& SetEngineIdleClipName( const char* clipName ) = 0;

    //
    // Name of clip to use for damaged engine sound
    //
    virtual ICarSoundParameters& SetDamagedEngineClipName( const char* clipName ) = 0;

    //
    // Horn sound
    //
    virtual ICarSoundParameters& SetHornClipName( const char* clipName ) = 0;

    //
    // Skid sounds
    //
    virtual ICarSoundParameters& SetRoadSkidClipName( const char* clipName ) = 0;
    virtual ICarSoundParameters& SetDirtSkidClipName( const char* clipName ) = 0;

    //
    // Backup sound
    //
    virtual ICarSoundParameters& SetBackupClipName( const char* clipName ) = 0;

    //
    // Car door entry/exit sounds
    //
    virtual ICarSoundParameters& SetCarDoorOpenClipName( const char* clipName ) = 0;
    virtual ICarSoundParameters& SetCarDoorCloseClipName( const char* clipName ) = 0;

    //
    // Percentages of top speed for shift points
    //
    virtual ICarSoundParameters& SetShiftPoint( unsigned int gear, float percent ) = 0;

    //
    // Percentage of top speed that we'll use as a range for dropping
    // below the shift point to avoid downshifting
    //
    virtual ICarSoundParameters& SetDownshiftDamperSize( float percent ) = 0;

    //
    // Pitch range for a particular gear
    //
    virtual ICarSoundParameters& SetGearPitchRange( unsigned int gear, float min, float max ) = 0;

    //
    // Number of gears
    //
    virtual ICarSoundParameters& SetNumberOfGears( unsigned int gear ) = 0;

    //
    // Attack/delay/decay
    //
    virtual ICarSoundParameters& SetAttackTimeMsecs( float msecs ) = 0;
    virtual ICarSoundParameters& SetDelayTimeMsecs( unsigned int msecs ) = 0;
    virtual ICarSoundParameters& SetDecayTimeMsecs( float msecs ) = 0;

    virtual ICarSoundParameters& SetDecayFinishTrim( float trim ) = 0;

    //
    // Top speed for which we'll increase pitch in reverse
    //
    virtual ICarSoundParameters& SetReversePitchCapKmh( float speed ) = 0;

    //
    // Reverse gear pitch range
    //
    virtual ICarSoundParameters& SetReversePitchRange( float min, float max ) = 0;

    //
    // Amount of pitch drop and time to apply in gear shifts
    //
    virtual ICarSoundParameters& SetGearShiftPitchDrop( unsigned int gear, float drop ) = 0;

    //
    // Percentage damage when we play damage sound
    //
    virtual ICarSoundParameters& SetDamageStartPcnt( float damagePercent ) = 0;
    virtual ICarSoundParameters& SetDamageMaxVolPcnt( float percent ) = 0;

    //
    // Volume control on damage sounds
    //
    virtual ICarSoundParameters& SetDamageStartTrim( float trim ) = 0;
    virtual ICarSoundParameters& SetDamageMaxTrim( float trim ) = 0;

    //
    // Pitch for idle engine clip (separate from sound resource in case
    // we want to share resource between cars.  Strictly a convenience,
    // since multiple sound resources with the same sound file could be
    // created instead).
    //
    virtual ICarSoundParameters& SetIdleEnginePitch( float pitch ) = 0;

    //
    // In-air sound characteristics
    //
    virtual ICarSoundParameters& SetInAirThrottlePitch( float pitch ) = 0;
    virtual ICarSoundParameters& SetInAirIdlePitch( float pitch ) = 0;
    virtual ICarSoundParameters& SetInAirThrottleResponseTimeMsecs( unsigned int msecs ) = 0;

    //
    // Burnout sound characteristics
    //
    virtual ICarSoundParameters& SetBurnoutMinPitch( float pitch ) = 0;
    virtual ICarSoundParameters& SetBurnoutMaxPitch( float pitch ) = 0;

    //
    // Powerslide sound characteristics
    //
    virtual ICarSoundParameters& SetPowerslideMinPitch( float pitch ) = 0;
    virtual ICarSoundParameters& SetPowerslideMaxPitch( float pitch ) = 0;

    //
    // Playing an overlay clip
    //
    virtual ICarSoundParameters& SetOverlayClipName( const char* clipName ) = 0;
};


#endif // ICARSOUNDPARAMETERS_H

