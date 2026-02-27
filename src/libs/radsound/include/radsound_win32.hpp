//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================
// 
// File: radsound_win32.hpp
//
// Description: PC radsound interface and function definitions
//============================================================================

#ifndef RADSOUND_WIN32_HPP
#define RADSOUND_WIN32_HPP

//============================================================================
// Include Files
//============================================================================

#include <radsound_hal.hpp>

//============================================================================
// Forward Declarations
//============================================================================

struct IRadSoundHalEffectEAX2Reverb;

//============================================================================
// Factories and static functions
//============================================================================

IRadSoundHalEffectEAX2Reverb * radSoundHalEffectEAX2ReverbCreate( radMemoryAllocator allocator );

//============================================================================
/// IRadSoundHalEffectEAX2Reverb
///
/// This interface exposes the environmental reverb support of the sound card.
///
/// Will work only for cards that support Creative's Environmental
/// Audio Extensions 2.0 (EAX2.0)
///
/// See Creative documentation for more detailed description of parameters.
///
/// Effect is only active if it is set to the Listener Environment aux send.
///
/// This effect will only apply itself to positional sound that were allocated
/// to the sound card hardware.  You can figure out how many positional hardware
/// voices are supported by your sound card by using the DXDiags application that
/// comes with DirectX.
///
/// Will also only work if you've enabled advanced sound hardware support under
/// you control panel.  This is NOT the default behaviour under Windows2000.
/// You will have to add a special note in you documentation to inform users
/// to enable advanced sound hardware support.  
/// (ControlPanel->Sounds..->Audio->Advanced->Performace  : "Hardware 
/// Accelerations"Slider should be set to "Full"
///
//============================================================================

struct IRadSoundHalEffectEAX2Reverb : public IRadSoundHalEffect
{
    /// The Room property is the master volume control for the reflected sound (both
    /// early reflections and reverberation) that EAX adds to all sound sources. It sets the
    /// maximum amount of reflections and reverberation added to the sound mix in the
    /// primary buffer (the listener). The value of the Room property ranges from 0 dB
    /// (the maximum amount) to -100 dB (no reflected sound at all).
    ///
    /// range -10000 ... 0
    virtual void  SetRoom( int mBValue ) = 0;
    virtual int   GetRoom( void ) = 0;

    /// The Room property is the master volume control for the reflected sound (both
    /// early reflections and reverberation) that EAX adds to all sound sources. It sets the
    /// maximum amount of reflections and reverberation added to the sound mix in the
    /// primary buffer (the listener). The value of the Room property ranges from 0 dB
    /// (the maximum amount) to -100 dB (no reflected sound at all).
    ///
    /// range -10000 ... 0
    virtual void  SetRoomHF( int mBValue ) = 0;
    virtual int   GetRoomHF( void ) = 0;

    /// Rolloff effect intensity vs. distance 
    ///
    /// The Room Rolloff property is one of two methods available in EAX to attenuate
    /// the reflected sound (containing both reflections and reverberation) according to
    /// source-listener distance. It’s defined the same way as DirectSound’s Rolloff
    /// Factor, but operates on reflected sound instead of direct-path sound. Setting the
    /// Room Rolloff Factor value to 1.0 specifies that the reflected sound will decay by 6
    /// dB every time the distance doubles. Any value other than 1.0 is equivalent to a
    /// scaling factor applied to the quantity specified by ((Source listener distance) -
    /// (Minimum Distance)). Minimum distance is a DirectSound sound-source
    /// parameter that specifies the inner border for distance rolloff effects: if the source
    /// comes closer to the listener than the minimum distance, the direct-path sound isn’t
    /// increased as the source comes closer to the listener, and neither is the reflected
    /// sound.
    ///
    /// range 0.0f ... 10.0f
    virtual void  SetRoomRolloffFactor( float value ) = 0;
    virtual float GetRoomRolloffFactor( void ) = 0;


    /// The Decay Time property sets the reverberation decay time. It ranges from 0.1
    /// (typically a small room with very dead surfaces) to 20.0 (typically a large room
    /// with very live surfaces).    
    ///    
    /// range 0.1...20.0 seconds
    virtual void  SetDecayTime( float value ) = 0;
    virtual float GetDecayTime( void ) = 0;

    /// The Decay HF Ratio property sets the spectral quality of the Decay Time
    /// parameter. It is the ratio of high-frequency decay time relative to the time set by
    /// Decay Time. The Decay HF Ratio value 1.0 is neutral: the decay time is equal for
    /// all frequencies. As Decay HF Ratio increases above 1.0, the high-frequency decay
    /// time increases so it’s longer than the decay time at low frequencies. You hear a
    /// more brilliant reverberation with a longer decay at high frequencies. As the Decay
    ///(HF Ratio value decreases below 1.0, the high-frequency decay time decreases so
    /// it’s shorter than the decay time of the low frequencies. You hear a more natural
    /// reverberation.
    ///
    /// range 0.1 ... 2.0f
    virtual void  SetDecayHFRatio( float value ) = 0;
    virtual float GetDecayHFRatio( void ) = 0;

    /// The Reflections property controls the overall amount of initial reflections relative
    /// to the Room property. (The Room property sets the overall amount of reflected
    /// sound: both initial reflections and later reverberation.) The value of Reflections
    /// ranges from a maximum of 1000 mB to a minimum of -10000 mB (no initial reflections
    /// at all), and is corrected by the value of the Room property. The Reflections
    /// property does not affect the subsequent reverberation decay.
    ///
    /// range -10000 ... 1000 mB
    virtual void  SetReflections( int mBValue ) = 0;
    virtual int   GetReflections( void ) = 0;

    /// The Reflections Delay property is the amount of delay between the arrival time of
    ///the direct path from the source to the first reflection from the source. It ranges
    /// from 0 to 300 milliseconds. You can reduce or increase Reflections Delay to
    /// simulate closer or more distant reflective surfaces—and therefore control the
    /// perceived size of the room.
    /// 
    /// range 0.0f ... 0.3f
    virtual void  SetReflectionsDelay( float value ) = 0;
    virtual float GetReflectionsDelay( void ) = 0;

    /// The Reverb property controls the overall amount of later reverberation relative to
    /// the Room property. (The Room property sets the overall amount of both initial
    /// reflections and later reverberation.) The value of Reverb ranges from a maximum
    /// of 20 dB to a minimum of -100 dB (no late reverberation at all).
    ///
    /// range -10000 ... 2000
    virtual void  SetReverb( int mBValue ) = 0;
    virtual int   GetReverb( void ) = 0;

    /// The Reverb Delay property defines the begin time of the late reverberation relative
    /// to the time of the initial reflection (the first of the early reflections). It ranges from
    /// 0 to 100 milliseconds. Reducing or increasing Reverb Delay is useful for
    /// simulating a smaller or larger room.
    ///
    /// range 0.0f ... 1.0f
    virtual void  SetReverbDelay( float value ) = 0;
    virtual float GetReverbDelay( void ) = 0;

    /// The Environment Diffusion property controls the echo density in the reverberation
    /// decay. It’s set by default to 1.0, which provides the highest density. Reducing
    /// diffusion gives the reverberation a more “grainy” character that is especially
    /// noticeable with percussive sound sources. If you set a diffusion value of 0.0, the
    /// later reverberation sounds like a succession of distinct echoes.
    ///
    /// range 0.0f ... 1.0f
    virtual void  SetEnvironmentDiffusion( float value ) = 0;
    virtual float GetEnvironmentDiffusion( void ) = 0;

    /// The Air Absorption HF property controls the distance-dependent attenuation at
    /// high frequencies caused by the propagation medium. It applies to both the direct
    /// path and reflected sound. You can use Air Absorption HF to simulate sound
    /// transmission through foggy air, dry air, smoky atmosphere, and so on. The
    /// default value is -5 mB per meter, which roughly corresponds to typical
    /// condition of atmospheric humidity, temperature, and so on. Lowering the value
    /// simulates a more absorbent medium (more humidity in the air, for example);
    /// raising the value simulates a less absorbent medium (dry desert air, for example).
    ///
    /// -100.0f ... 0.0f mB per meter
    virtual void  SetAirAbsorptionHF( float value ) = 0;       
    virtual float GetAirAbsorptionHF( void ) = 0;       
};

#endif // RADSOUND_WIN32_HPP