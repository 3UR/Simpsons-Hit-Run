//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//============================================================================
// File: radsound_xbox.hpp
//
// Description: 
//============================================================================

#ifndef RADSOUND_XBOX_HPP
#define RADSOUND_XBOX_HPP

//============================================================================
// Include Files
//============================================================================

#include <radsound_hal.hpp>
#include <radsoundfile.hpp>

//============================================================================
// Forward Declarations
//============================================================================

struct IRadSoundWmaFileDataSource;
struct IRadSoundHalEffectEchoXBox;
struct IRadSoundHalEffectI3DL2ReverbXBox;
struct IRadSoundHalEffectDistortionXBox;

//============================================================================
// Factories and static functions
//============================================================================

IRadSoundWmaFileDataSource *        radSoundWmaFileDataSourceCreate( radMemoryAllocator allocator );
IRadSoundHalEffectEchoXBox *        radSoundHalEffectEchoXBoxCreate( radMemoryAllocator allocator );
IRadSoundHalEffectI3DL2ReverbXBox * radSoundHalEffectI3DL2ReverbXBoxCreate( radMemoryAllocator allocator );
IRadSoundHalEffectDistortionXBox *  radSoundHalEffectDistortionXBoxCreate( radMemoryAllocator allocator );

//============================================================================
// Interface: IRadSoundWmaFileDataSource
//
// Description: An UNBUFFERED data source that reads from a .WMA file.  
//      Chain in a buffered data source after the file data source to do 
//      custom buffering to compensate for file and decoding latencies
//============================================================================

struct IRadSoundWmaFileDataSource : public IRadSoundHalDataSource
{
    // NOTE: When using XSoundtrack interfaces... Get file handle
    // (HANDLE) using XOpenSoundTrackSong( ), the create IRadFile
    // using ::radFileOpenFromHandle( ).  
    //
    // ** When calling XOpenSoundTrackSong( ) you MUST set the 
    // second parameter (BOOL fAsyncMode) to false.  This is a
    // requirement of the xbox decoder that is contained within
    // IRadSoundWmaFileDataSource
    // 
    // YieldRate: Specifies how often the the xbox wma decoder 
    // should should yield execution while decoding.  0 means never 
    // yield. 1 means always yield.  2 or more means yield once
    // every n iterations (n = yieldRate)

    virtual void InitializeFromFile( IRadFile * pIRadFile, unsigned int yieldRate = 3 ) = 0;

    virtual IRadFile * GetFile( void ) = 0;
};


//============================================================================
// Interface: IRadSoundHalEffectEchoXBox
//
// Description:  Exposes the modifiable parameters of the echo effect
//               (mono and stereo)
//
//============================================================================

struct IRadSoundHalEffectEchoXBox : public IRadSoundHalEffect
{
    //
    // Gain applied to delay line input when mixing with dry signal
    //
    virtual void SetGain( float gain ) = 0;
    virtual float GetGain( void ) = 0;
};

//============================================================================
// Interface: IRadSoundHalI3DL2EffectReverbXBox
//
// Description:  This effect is the only effect that will work with the 
//               Xbox's listener environmental fx system.  Microsofts
//               various presets are available in DSound.h
//               (e.g. DSI3DL2_ENVIRONMENT_PRESET_CAVE)
//
//============================================================================

struct IRadSoundHalEffectI3DL2ReverbXBox : public IRadSoundHalEffect
{
    // Intensity level and low-pass filter for room effect (wet signal)
    // (-10000mB...1000mB)
    virtual void  SetRoom( int mBvalue ) = 0;
    virtual int   GetRoom( void ) = 0;

    // Attenuation at high frequencies relative to intensity at low frequencies
    // (-10000mB...0mB)
    virtual void  SetRoomHF( int mBvalue ) = 0;
    virtual int   GetRoomHF( void ) = 0;

    // Rolloff effect intensity vs. distance (0.0...10.0)
    virtual void  SetRoomRolloffFactor( float value ) = 0;
    virtual float GetRoomRolloffFactor( void ) = 0;

    // Decay time at low frequencies (0.1...20.0 seconds)
    virtual void  SetDecayTime( float value ) = 0;
    virtual float GetDecayTime( void ) = 0;

    // Ratio of high-frequency decay time to low frequency decay time
    // (0.1...2.0)
    virtual void  SetDecayHFRatio( float value ) = 0;
    virtual float GetDecayHFRatio( void ) = 0;

    // Intensity level of early reflections relative to 'Room' value
    // (-10000mB...1000mB)
    virtual void  SetReflections( int mBvalue ) = 0;
    virtual int   GetReflections( void ) = 0;

    // Delay time of first reflection relative to direct path
    // (0.0...0.3seconds
    virtual void  SetReflectionsDelay( float value ) = 0;
    virtual float GetReflectionsDelay( void ) = 0;

    // Intensity of late reverberations relative to 'Room' value
    // (-10000mB...2000mB)
    virtual void  SetReverb( int mBvalue ) = 0;
    virtual int   GetReverb( void ) = 0;

    // Time limit between the early reflections and the late reverberation, 
    // relative to the time of the first reflection (0.0...0.4seconds)
    virtual void  SetReverbDelay( float value ) = 0;
    virtual float GetReverbDelay( void ) = 0;

    // Echo density in late reverberation decay (0.0%...100.0%)
    // Proportional to # echoes per second
    virtual void  SetDiffusion( float value ) = 0;
    virtual float GetDiffusion( void ) = 0;

    // Modal density in late reverberation decay (0.0%...100.0%)
    // Proportional to # resonances per Hertz
    virtual void  SetDensity( float value ) = 0;
    virtual float GetDensity( void ) = 0;

    // Reference high frequency value (20.0Hz...20000.0Hz)
    virtual void  SetHFReference( float value ) = 0;
    virtual float GetHFReference( void ) = 0;

};

//============================================================================
// Interface: IRadSoundHalEffectDistortionXBox
//
// Description:  Exposes the modifiable parameters of the distortion
//
//============================================================================

struct IRadSoundHalEffectDistortionXBox : public IRadSoundHalEffect
{
    // Distortion includes a pre-filter IIR2 effect and a 
    // post-filter IIR2 effect, where the output of the
    // IIR2 effect is expressed by the following equation:
    //
    // y[n] = b0*x[n] + b1*x[n-1] + b2*x[n-2] + a1*y[n-1] + a2*y[n-2] 
    //

    enum IIR2FilterCoefficient { B0, B1, B2, A1, A2 };

    virtual void SetGain( float gain ) = 0;
    virtual float GetGain( void ) = 0;
    virtual void SetPreFilterCoefficient( IIR2FilterCoefficient coef, float value ) = 0;
    virtual float GetPreFilterCoefficient( IIR2FilterCoefficient coef ) = 0;
    virtual void SetPostFilterCoefficient( IIR2FilterCoefficient coef, float value ) = 0;
    virtual float GetPostFilterCoefficient( IIR2FilterCoefficient coef ) = 0;
};

#endif // RADSOUND_XBOX_HPP