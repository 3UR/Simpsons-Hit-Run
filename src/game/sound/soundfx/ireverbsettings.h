//=============================================================================
// Copyright (C) 2002 Radical Entertainment Ltd.  All rights reserved.
//
// File:        ireverbsettings.h
//
// Description: Declaration for IReverbSettings class, which is the interface
//              visible in RadTuner for creating and storing a set of reverb 
//              parameters.
//
// History:     11/3/2002 + Created -- Darren
//
//=============================================================================

#ifndef IREVERBSETTINGS_H
#define IREVERBSETTINGS_H

//========================================
// Nested Includes
//========================================
#include <radobject.hpp>

//========================================
// Forward References
//========================================

//=============================================================================
//
// Synopsis:    IReverbSettings
//
//=============================================================================

struct IReverbSettings : public IRefCount
{
    virtual IReverbSettings& SetGain( float gain ) = 0;
    virtual IReverbSettings& SetFadeInTime( float milliseconds ) = 0;
    virtual IReverbSettings& SetFadeOutTime( float milliseconds ) = 0;

    //
    // See radsound_<platform name>.hpp for details on this stuff
    //
    virtual IReverbSettings& SetXboxRoom( int mBvalue ) = 0;
    virtual IReverbSettings& SetXboxRoomHF( int mBvalue ) = 0;
    virtual IReverbSettings& SetXboxRoomRolloffFactor( float value ) = 0;
    virtual IReverbSettings& SetXboxDecayTime( float value ) = 0;
    virtual IReverbSettings& SetXboxDecayHFRatio( float value ) = 0;
    virtual IReverbSettings& SetXboxReflections( int mBvalue ) = 0;
    virtual IReverbSettings& SetXboxReflectionsDelay( float value ) = 0;
    virtual IReverbSettings& SetXboxReverb( int mBvalue ) = 0;
    virtual IReverbSettings& SetXboxReverbDelay( float value ) = 0;
    virtual IReverbSettings& SetXboxDiffusion( float value ) = 0;
    virtual IReverbSettings& SetXboxDensity( float value ) = 0;
    virtual IReverbSettings& SetXboxHFReference( float value ) = 0;

    // No RadTuner interface for enumerations as far as I know, so
    // we'll have to cast whatever integer we get here
    virtual IReverbSettings& SetPS2ReverbMode( int mode ) = 0;

    virtual IReverbSettings& SetPS2Delay( float delayTime ) = 0;
    virtual IReverbSettings& SetPS2Feedback( float feedback ) = 0;

    virtual IReverbSettings& SetGCPreDelay( float milliseconds ) = 0;
    virtual IReverbSettings& SetGCReverbTime( float milliseconds ) = 0;
    virtual IReverbSettings& SetGCColoration( float coloration ) = 0;
    virtual IReverbSettings& SetGCDamping( float damping ) = 0;

    // Must be defined for all platforms cause of the script.
    virtual IReverbSettings& SetWinEnvironmentDiffusion( float diffusion ) = 0;
    virtual IReverbSettings& SetWinAirAbsorptionHF( float value ) = 0;
};


#endif // IREVERBSETTINGS_H

