//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================
//
// File:        effect.cpp
//
// Subsystem:	Radical Sound System
//
// Description:	Effects on the PC
//
// Creation:    RWS
//
//=============================================================================

//============================================================================
// Include Files
//============================================================================

#include "pch.hpp"
#include "effect.hpp"
#include "system.hpp"
#include "listener.hpp"

//============================================================================
// radSoundHalEffectEAX2Reverb::radSoundHalEffectEAX2Reverb
//============================================================================

radSoundHalEffectEAX2Reverb::radSoundHalEffectEAX2Reverb( void )
    :
    m_AuxSlot( AL_EFFECTSLOT_NULL )
{
    alGenEffects = (LPALGENEFFECTS)alGetProcAddress("alGenEffects");
    alDeleteEffects = (LPALDELETEEFFECTS)alGetProcAddress("alDeleteEffects");
    alEffecti = (LPALEFFECTI)alGetProcAddress("alEffecti");
    alEffectf = (LPALEFFECTF)alGetProcAddress("alEffectf");
    alGetEffecti = (LPALGETEFFECTI)alGetProcAddress("alGetEffecti");
    alGetEffectf = (LPALGETEFFECTF)alGetProcAddress("alGetEffectf");
    alAuxiliaryEffectSloti = (LPALAUXILIARYEFFECTSLOTI)alGetProcAddress("alAuxiliaryEffectSloti");

    alGenEffects(1, &m_Effect);
    alEffecti(m_Effect, AL_EFFECT_TYPE, AL_EFFECT_REVERB);

    // I think it's better to default to reverb that you can really hear
    SetRoom(-1000);
    SetRoomHF(-500);
    SetRoomRolloffFactor(0.0f);
    SetDecayTime(3.92f);
    SetDecayHFRatio(0.70f);
    SetReflections(-1230);
    SetReflectionsDelay(0.020f);
    SetReverb(-2);
    SetReverbDelay(0.029f);
}

//============================================================================
// radSoundHalEffectEAX2Reverb::radSoundHalEffectEAX2Reverb
//============================================================================

radSoundHalEffectEAX2Reverb::~radSoundHalEffectEAX2Reverb( void )
{
}

//============================================================================
// radSoundHalEffectEAX2Reverb::Attach
//============================================================================

void radSoundHalEffectEAX2Reverb::Attach( unsigned int auxSend )
{
    ALuint auxSlot = radSoundHalSystem::GetInstance()->GetOpenALAuxSlot(auxSend);
    alAuxiliaryEffectSloti(auxSlot, AL_EFFECTSLOT_EFFECT, m_Effect);
    m_AuxSlot = auxSlot;
}

//============================================================================
// radSoundHalEffectEAX2Reverb::Detach
//============================================================================

void radSoundHalEffectEAX2Reverb::Detach( void )
{
    alAuxiliaryEffectSloti(m_AuxSlot, AL_EFFECTSLOT_EFFECT, AL_EFFECT_NULL);
    m_AuxSlot = AL_EFFECTSLOT_NULL;
}

//============================================================================
// radSoundHalEffectEAX2Reverb::Update
//============================================================================

void radSoundHalEffectEAX2Reverb::Update( void )
{
}

void radSoundHalEffectEAX2Reverb::SetRoom( int mBValue )
{
    float value = ::radSoundVolumeDbToHardwareWin(mBValue / 100.0f);
    rAssert(value >= AL_REVERB_MIN_GAIN && value <= AL_REVERB_MAX_GAIN);
    alEffectf(m_Effect, AL_REVERB_GAIN, value);
    OnParameterUpdated();
}
int   radSoundHalEffectEAX2Reverb::GetRoom( void )
{
    float value;
    alGetEffectf(m_Effect, AL_REVERB_GAIN, &value);
    return static_cast<int>(::radSoundVolumeHardwareToDbWin(value) * 100.0f);
}
void  radSoundHalEffectEAX2Reverb::SetRoomHF( int mBValue )
{
    float value = ::radSoundVolumeDbToHardwareWin(mBValue / 100.0f);
    rAssert(value >= AL_REVERB_MIN_GAINHF && value <= AL_REVERB_MAX_GAINHF);
    alEffectf(m_Effect, AL_REVERB_GAINHF, value);
    OnParameterUpdated();
}
int radSoundHalEffectEAX2Reverb::GetRoomHF( void )
{
    float value;
    alGetEffectf(m_Effect, AL_REVERB_GAINHF, &value);
    return static_cast<int>(::radSoundVolumeHardwareToDbWin(value) * 100.0f);
}
void  radSoundHalEffectEAX2Reverb::SetRoomRolloffFactor( float value )
{
    rAssert(value >= AL_REVERB_MIN_ROOM_ROLLOFF_FACTOR && value <= AL_REVERB_MAX_ROOM_ROLLOFF_FACTOR);
    alEffectf(m_Effect, AL_REVERB_ROOM_ROLLOFF_FACTOR, value);
    OnParameterUpdated();
}
float radSoundHalEffectEAX2Reverb::GetRoomRolloffFactor( void )
{
    float value;
    alGetEffectf(m_Effect, AL_REVERB_ROOM_ROLLOFF_FACTOR, &value);
    return value;
}
void  radSoundHalEffectEAX2Reverb::SetDecayTime( float value )
{
    rAssert(value >= AL_REVERB_MIN_DECAY_TIME && value <= AL_REVERB_MAX_DECAY_TIME);
    alEffectf(m_Effect, AL_REVERB_DECAY_TIME, value);
    OnParameterUpdated();
}
float radSoundHalEffectEAX2Reverb::GetDecayTime( void )
{
    float value;
    alGetEffectf(m_Effect, AL_REVERB_DECAY_TIME, &value);
    return value;
}
void  radSoundHalEffectEAX2Reverb::SetDecayHFRatio( float value )
{
    rAssert(value >= AL_REVERB_MIN_DECAY_HFRATIO && value <= AL_REVERB_MAX_DECAY_HFRATIO);
    alEffectf(m_Effect, AL_REVERB_DECAY_HFRATIO, value);
    OnParameterUpdated();
}
float radSoundHalEffectEAX2Reverb::GetDecayHFRatio( void )
{
    float value;
    alGetEffectf(m_Effect, AL_REVERB_DECAY_HFRATIO, &value);
    return value;
}
void  radSoundHalEffectEAX2Reverb::SetReflections( int mBValue )
{
    float value = ::radSoundVolumeDbToHardwareWin(mBValue / 100.0f);
    rAssert(value >= AL_REVERB_MIN_REFLECTIONS_GAIN && value <= AL_REVERB_MAX_REFLECTIONS_GAIN);
    alEffectf(m_Effect, AL_REVERB_REFLECTIONS_GAIN, value);
    OnParameterUpdated();
}
int   radSoundHalEffectEAX2Reverb::GetReflections( void )
{
    float value;
    alGetEffectf(m_Effect, AL_REVERB_REFLECTIONS_GAIN, &value);
    return static_cast<int>(::radSoundVolumeHardwareToDbWin(value) * 100.0f);
}
void  radSoundHalEffectEAX2Reverb::SetReflectionsDelay( float value )
{
    rAssert(value >= AL_REVERB_MIN_REFLECTIONS_DELAY && value <= AL_REVERB_MAX_REFLECTIONS_DELAY);
    alEffectf(m_Effect, AL_REVERB_REFLECTIONS_DELAY, value);
    OnParameterUpdated();
}
float radSoundHalEffectEAX2Reverb::GetReflectionsDelay( void )
{
    float value;
    alGetEffectf(m_Effect, AL_REVERB_REFLECTIONS_DELAY, &value);
    return value;
}
void  radSoundHalEffectEAX2Reverb::SetReverb( int mBValue )
{
    float value = ::radSoundVolumeDbToHardwareWin(mBValue / 100.0f);
    rAssert(value >= AL_REVERB_MIN_LATE_REVERB_GAIN && value <= AL_REVERB_MAX_LATE_REVERB_GAIN);
    alEffectf(m_Effect, AL_REVERB_LATE_REVERB_GAIN, value);
    OnParameterUpdated();
}
int   radSoundHalEffectEAX2Reverb::GetReverb( void )
{
    float value;
    alGetEffectf(m_Effect, AL_REVERB_LATE_REVERB_GAIN, &value);
    return static_cast<int>(::radSoundVolumeHardwareToDbWin(value) * 100.0f);
}
void  radSoundHalEffectEAX2Reverb::SetReverbDelay( float value )
{
    rAssert(value >= AL_REVERB_MIN_LATE_REVERB_DELAY && value <= AL_REVERB_MAX_LATE_REVERB_DELAY);
    alEffectf(m_Effect, AL_REVERB_LATE_REVERB_DELAY, value);
    OnParameterUpdated();
}
float radSoundHalEffectEAX2Reverb::GetReverbDelay( void )
{
    float value;
    alGetEffectf(m_Effect, AL_REVERB_LATE_REVERB_DELAY, &value);
    return value;
}
void  radSoundHalEffectEAX2Reverb::SetEnvironmentSize( float value )
{
    rAssert(0);
}
float radSoundHalEffectEAX2Reverb::GetEnvironmentSize( void )
{
    return 0.0f;
}
void  radSoundHalEffectEAX2Reverb::SetEnvironmentDiffusion( float value )
{
    rAssert(value >= AL_REVERB_MIN_DIFFUSION && value <= AL_REVERB_MAX_DIFFUSION);
    alEffectf(m_Effect, AL_REVERB_DIFFUSION, value);
    OnParameterUpdated();
}
float radSoundHalEffectEAX2Reverb::GetEnvironmentDiffusion( void )
{
    float value;
    alGetEffectf(m_Effect, AL_REVERB_DIFFUSION, &value);
    return value;
}
void  radSoundHalEffectEAX2Reverb::SetAirAbsorptionHF( float value )
{
    value = powf(10.f, value / 2000.f);
    rAssert(value >= AL_REVERB_MIN_AIR_ABSORPTION_GAINHF && value <= AL_REVERB_MAX_AIR_ABSORPTION_GAINHF);
    alEffectf(m_Effect, AL_REVERB_AIR_ABSORPTION_GAINHF, value);
    OnParameterUpdated();
}
float radSoundHalEffectEAX2Reverb::GetAirAbsorptionHF( void )
{
    float value;
    alGetEffectf(m_Effect, AL_REVERB_AIR_ABSORPTION_GAINHF, &value);
    return value;
}
void radSoundHalEffectEAX2Reverb::OnParameterUpdated(void)
{
    if (m_AuxSlot != AL_EFFECTSLOT_NULL)
        alAuxiliaryEffectSloti(m_AuxSlot, AL_EFFECTSLOT_EFFECT, m_Effect);
}

//============================================================================
// radSoundHalEffectI3DL2ReverbWin32Create
//============================================================================

IRadSoundHalEffectEAX2Reverb * radSoundHalEffectEAX2ReverbCreate( radMemoryAllocator allocator )
{
    if (alcIsExtensionPresent(((radSoundHalSystem*)radSoundHalSystemGet())->GetOpenALDevice(), "ALC_EXT_EFX"))
        return new ( "radSoundHalEffectEAX2Reverb", allocator ) radSoundHalEffectEAX2Reverb( );
    return nullptr;
}