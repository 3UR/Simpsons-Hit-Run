//=============================================================================
// Copyright (C) 2002 Radical Entertainment Ltd.  All rights reserved.
//
// File:        reverbsettings.h
//
// Description: Declaration for the reverbSettings class, which stores sets
//              of reverb parameters to be applied whenever we want that
//              reverby sound thing happening.  NOTE: lower-case "r" needed
//              to make RadScript happy.
//
// History:     11/5/2002 + Created -- Darren
//
//=============================================================================

#ifndef REVERBSETTINGS_H
#define REVERBSETTINGS_H

//========================================
// Nested Includes
//========================================
#include <radobject.hpp>
#include <radlinkedclass.hpp>

#include <sound/soundfx/ireverbsettings.h>

//========================================
// Forward References
//========================================

//=============================================================================
//
// Synopsis:    reverbSettings
//
//=============================================================================

class reverbSettings : public IReverbSettings,
                       public radLinkedClass< reverbSettings >,
                       public radRefCount
{
    public:
        IMPLEMENT_REFCOUNTED( "reverbSettings" );

        reverbSettings();
        virtual ~reverbSettings();

        IReverbSettings& SetGain( float gain ) { m_gain = gain; return *this; }
        float GetGain() { return( m_gain ); }
        IReverbSettings& SetFadeInTime( float milliseconds ) { m_fadeInTime = milliseconds; return *this; }
        float GetFadeInTime() { return( m_fadeInTime ); }
        IReverbSettings& SetFadeOutTime( float milliseconds ) { m_fadeOutTime = milliseconds; return *this; }
        float GetFadeOutTime() { return( m_fadeOutTime ); }

        //
        // See radsound_<platform name>.hpp for details on this stuff
        //
        IReverbSettings& SetXboxRoom( int mBvalue ) { m_xboxRoom = mBvalue; return *this; }
        int GetXboxRoom() { return( m_xboxRoom ); }
        IReverbSettings& SetXboxRoomHF( int mBvalue ) { m_xboxRoomHF = mBvalue; return *this; }
        int GetXboxRoomHF() { return( m_xboxRoomHF ); }
        IReverbSettings& SetXboxRoomRolloffFactor( float value ) { m_xboxRoomRolloff = value; return *this; }
        float GetXboxRoomRolloffFactor() { return( m_xboxRoomRolloff ); }
        IReverbSettings& SetXboxDecayTime( float value ) { m_xboxDecay = value; return *this; }
        float GetXboxDecayTime() { return( m_xboxDecay ); }
        IReverbSettings& SetXboxDecayHFRatio( float value ) { m_xboxDecayHFRatio = value; return *this; }
        float GetXboxDecayHFRatio() { return( m_xboxDecayHFRatio ); }
        IReverbSettings& SetXboxReflections( int mBvalue ) { m_xboxReflections = mBvalue; return *this; }
        int GetXboxReflections() { return( m_xboxReflections ); }
        IReverbSettings& SetXboxReflectionsDelay( float value ) { m_xboxReflectionsDelay = value; return *this; }
        float GetXboxReflectionsDelay() { return( m_xboxReflectionsDelay ); }
        IReverbSettings& SetXboxReverb( int mBvalue ) { m_xboxReverb = mBvalue; return *this; }
        int GetXboxReverb() { return( m_xboxReverb ); }
        IReverbSettings& SetXboxReverbDelay( float value ) { m_xboxReverbDelay = value; return *this; }
        float GetXboxReverbDelay() { return( m_xboxReverbDelay ); }
        IReverbSettings& SetXboxDiffusion( float value ) { m_xboxDiffusion = value; return *this; }
        float GetXboxDiffusion() { return( m_xboxDiffusion ); }
        IReverbSettings& SetXboxDensity( float value ) { m_xboxDensity = value; return *this; }
        float GetXboxDensity() { return( m_xboxDensity ); }
        IReverbSettings& SetXboxHFReference( float value ) { m_xboxHFReference = value; return *this; }
        float GetXboxHFReference() { return( m_xboxHFReference ); }

        // No RadTuner interface for enumerations as far as I know, so
        // we'll have to cast whatever integer we get here
        IReverbSettings& SetPS2ReverbMode( int mode ) { m_ps2ReverbMode = mode; return *this; }
        int GetPS2ReverbMode() { return( m_ps2ReverbMode ); }

        IReverbSettings& SetPS2Delay( float delayTime ) { m_ps2Delay = delayTime; return *this; }
        float GetPS2Delay() { return( m_ps2Delay ); }
        IReverbSettings& SetPS2Feedback( float feedback ) { m_ps2Feedback = feedback; return *this; }
        float GetPS2Feedback() { return( m_ps2Feedback ); }

        IReverbSettings& SetGCPreDelay( float milliseconds ) { m_gcPreDelay = milliseconds; return *this; }
        float GetGCPreDelay() { return( m_gcPreDelay ); }
        IReverbSettings& SetGCReverbTime( float milliseconds ) { m_gcReverbTime = milliseconds; return *this; }
        float GetGCReverbTime() { return( m_gcReverbTime ); }
        IReverbSettings& SetGCColoration( float coloration ) { m_gcColoration = coloration; return *this; }
        float GetGCColoration() { return( m_gcColoration ); }
        IReverbSettings& SetGCDamping( float damping ) { m_gcDamping = damping; return *this; }
        float GetGCDamping() { return( m_gcDamping ); }

        // Must be defined for all platforms cause of the script.
        IReverbSettings& SetWinEnvironmentDiffusion( float diffusion ) { m_winEnvironmentDiffusion = diffusion; return *this; }
        float GetWinEnvironmentDiffusion() const { return m_winEnvironmentDiffusion; }
        IReverbSettings& SetWinAirAbsorptionHF( float value ) { m_winAirAbsorptionHF = value; return *this; }
        float GetWinAirAbsorptionHF() const { return m_winAirAbsorptionHF; }

        //
        // Create a reverbSettings object
        //
        static reverbSettings* ObjCreate( radMemoryAllocator allocator );

    private:
        //Prevent wasteful constructor creation.
        reverbSettings( const reverbSettings& original );
        reverbSettings& operator=( const reverbSettings& rhs );

        //
        // Reverb parameters
        //
        float m_gain;
        float m_fadeInTime;
        float m_fadeOutTime;

        int m_xboxRoom;
        int m_xboxRoomHF;
        float m_xboxRoomRolloff;
        float m_xboxDecay;
        float m_xboxDecayHFRatio;
        int m_xboxReflections;
        float m_xboxReflectionsDelay;
        int m_xboxReverb;
        float m_xboxReverbDelay;
        float m_xboxDiffusion;
        float m_xboxDensity;
        float m_xboxHFReference;

        int m_ps2ReverbMode;
        float m_ps2Delay;
        float m_ps2Feedback;

        float m_gcPreDelay;
        float m_gcReverbTime;
        float m_gcColoration;
        float m_gcDamping;

        float m_winEnvironmentDiffusion;
        float m_winAirAbsorptionHF;
};

#endif // REVERBSETTINGS_H

