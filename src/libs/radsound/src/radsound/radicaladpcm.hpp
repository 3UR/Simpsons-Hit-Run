//----------------------------------------------------------------------------- 
// 
//  Copyright (C) 2003 Radical Technology Inc. 
//  All Rights Reserved. 
// 
//----------------------------------------------------------------------------- 

#ifndef AUDIO_SRC_CORE_RADICALADPCM_HPP
#define AUDIO_SRC_CORE_RADICALADPCM_HPP

//----------------------------------------------------------------------------- 
// Includes
//----------------------------------------------------------------------------- 

#include "radsound.hpp"

// 128 samples per frame
// Usually this would be 34 samples of data so:
//
// 120 /34 = 3.75 to 1.

const unsigned int RADICAL_ADPCM_FRAME_SIZE = 20;
const unsigned int RADICAL_ADPCM_SAMPLES_PER_FRAME = 32;

struct RadicalAdpcmState
{
   short index;
   short prev;
};

int RadicalAdpcmDecode( int deltaCode, int * pIndex, int * pPrev );
int RadicalAdpcmEncode( int sample, int * pIndex, int * pPrev );

enum RadicalAdpcmDecodeState
{
    RadicalAdpcmDecodeState_UnInitialized,
    RadicalAdpcmDecodeState_InitializingSource,
    RadicalAdpcmDecodeState_Idle,    
    RadicalAdpcmDecodeState_ReadingSource,
    RadicalAdpcmDecodeState_Finished,
    RadicalAdpcmDecodeState_Error
};
    
struct RadicalAdpcmDecodeStreamInfo
{
    RadicalAdpcmDecodeState state;
    
    IRadSoundHalDataSource* pHalStream;
    IRadSoundHalAudioFormat* pHalAudioFormat;

    struct
    {
        bool done;
        unsigned int framesReading;
        unsigned int framesRead;
        void * pBufferStart;
        void * pAdpcmStart;
        IRadSoundHalDataSourceCallback * pHalStreamCallback;
    } read;
};

struct RadicalAdpcmDecodeStream
    :
    public IRadSoundAdpcmDecodeStream,
    public IRadSoundHalDataSourceCallback,
    public radRefCount
{
    // HalStream
    
    IMPLEMENT_REFCOUNTED( "RadicalAdpcmDecodeStream" );

    RadicalAdpcmDecodeStream( void );
    virtual ~RadicalAdpcmDecodeStream( void );

    virtual IRadSoundHalDataSource::State GetState( void ); 

    virtual IRadSoundHalAudioFormat* GetFormat( void );
    virtual unsigned int GetRemainingFrames( void );
    virtual unsigned int GetAvailableFrames( void ) { return 0xFFFFFFFF; }
    virtual void GetFramesAsync( 
        void* pBytes, 
        radMemorySpace destinationMemorySpace, 
        unsigned int numberOfFrames,
        IRadSoundHalDataSourceCallback* pCallback );
        
    virtual void DoWork( void );
    
    virtual const char * GetName( void ) { return "RadicalAdpcmDecodeStream"; }    
    
    // DecodeStream
    
    virtual void Initialize( IRadSoundHalDataSource* pInStream );
    
    // HalStreamCallback
    
    virtual void OnDataSourceFramesLoaded( unsigned int framesRead );
    
    RadicalAdpcmDecodeStreamInfo info;
};

#endif // AUDIO_SRC_CORE_RADICALADPCM_HPP
