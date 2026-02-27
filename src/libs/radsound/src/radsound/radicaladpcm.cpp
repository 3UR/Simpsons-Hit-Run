//----------------------------------------------------------------------------- 
// 
//  Copyright (C) 2003 Radical Technology Inc. 
//  All Rights Reserved. 
// 
//----------------------------------------------------------------------------- 

//----------------------------------------------------------------------------- 
// Includes
//----------------------------------------------------------------------------- 

#include "pch.hpp"
#include "radicaladpcm.hpp"

// #define PROFILE_DECODING

static const unsigned int MAX_CHANNELS = 16;

static const int INDEX_ADJUST_TABLE[ 16 ] =
{
   -1, -1, -1, -1,
    2,  4,  6,  8,
   -1, -1, -1, -1,
    2,  4,  6,  8
};

static const int STEP_TABLE[ 89 ] =
{
   7,       8,      9,      10,     11,     12,     13,     14,     16,     17,     19,
   21,      23,     25,     28,     31,     34,     37,      41,    45,     50,     55,
   60,      66,     73,     80,     88,     97,     107,    118,    130,    143,    157,
   173,     190,    209,    230,    253,    279,    307,    337,    371,    408,    449,
   494,     544,    598,    658,    724,    796,    876,    963,    1060,   1166,   1282,
   1411,    1552,   1707,   1878,   2066,   2272,   2499,   2749,   3024,   3327,   3660,
   4026,    4428,   4871,   5358,   5894,   6484,   7132,   7845,   8630,   9493,   10442,
   11487,   12635,  13899,  15289,  16818,  18500,  20350,  22385,  24623,  27086,  29794,
   32767
};

inline int inlRadicalAdpcmDecode(
    int delta,
    int * pIndex,
    int * pPrev )
{
    rAssert( *pIndex >= 0 );
    rAssert( *pIndex <= 88 );

    int step = STEP_TABLE[ *pIndex ];

    int difference = step>>3;

    if ( delta & 1 )
    {
        difference += step>>2;
    }

    if ( delta & 2 )
    {
        difference += step>>1;
    }

    if ( delta & 4 )
    {
        difference += step;
    }

    if ( delta & 8 )
    {
        difference = -difference;
    }

    *pPrev += difference;

    if (*pPrev > 32767)
    {
        *pPrev = 32767;
    }
    else if (*pPrev < -32768)
    {
        *pPrev = -32768;
    }

    *pIndex += INDEX_ADJUST_TABLE[ delta ];

    if (*pIndex < 0 )
    {
        *pIndex= 0;
    }
    else if ( *pIndex > 88 )
    {
        *pIndex= 88;
    }

    return *pPrev;
}

inline int inlRadicalAdpcmEncode(
    int sample,
    int * pIndex,
    int * pPrev )
{
    int diff = sample - *pPrev;
   
    int step = STEP_TABLE[*pIndex];
   
    int delta;

    if ( diff < 0 )
    {
        delta = 8;
        diff = -diff;
    }
    else
    {
        delta = 0;
    }

    if ( diff >= step )
    {
        delta |= 4;
        diff -= step;
    }
    
    step >>= 1;
    
    if ( diff >= step )
    {
        delta |= 2;
        diff -= step;
    }
    
    step >>= 1;
    
    if ( diff >= step )
    {
        delta |= 1;
        diff -= step;
    }

    inlRadicalAdpcmDecode( delta, pIndex, pPrev );
   
    return delta;
}

int RadicalAdpcmEncode(
    int sample,
    int * pIndex,
    int * pPrev )
{
    return inlRadicalAdpcmEncode( sample, pIndex, pPrev );
}

void DecodeFrames( void* pBufferStart, void * pAdpcmStart, unsigned numAdpcmFrames, unsigned int numChannels )
{
    rAssert( numChannels < MAX_CHANNELS );

    #ifdef PROFILE_DECODING
        static unsigned int totalBytes = 0;
        static unsigned int totalMicroseconds = 0;
        static unsigned int runningTimeMs = 0;
        static unsigned int runningLastTimeMs = 0;

        static bool init = false;

        if ( ! init )
        {
            runningLastTimeMs = radTimeGetMilliseconds( );
            init = true;
        }

        unsigned int startTime = radTimeGetMicroseconds( );
        
        unsigned int runningNowMs = radTimeGetMilliseconds( );
        unsigned int runningElapsed = runningNowMs - runningLastTimeMs;
        runningLastTimeMs = runningNowMs;

        runningTimeMs += runningElapsed; 
                   
    #endif

    
    RadicalAdpcmState channelState[ MAX_CHANNELS ];
        
    unsigned int adpcmFrameSize =
        numChannels * RADICAL_ADPCM_FRAME_SIZE;
    
    unsigned int totalAdpcmBytes = adpcmFrameSize * numAdpcmFrames;
        
    unsigned int pcmBlockSize =
        numChannels * RADICAL_ADPCM_SAMPLES_PER_FRAME * 2;
        
    unsigned int totalPcmBytes = pcmBlockSize * numAdpcmFrames;     
    
    short* pTarget = (short*) pBufferStart;
    char* pSource = (char*) pAdpcmStart;    
    char* pSourceEnd = pSource + totalAdpcmBytes;        

    while( pSource < pSourceEnd )
    {
        char* pSourceFrameEnd = pSource + adpcmFrameSize;

        for( unsigned int c = 0; c < numChannels; c ++ )
        {
            channelState[ c ].index = *((short*)pSource); pSource += 2;
            channelState[ c ].prev = *((short*)pSource); pSource += 2;
        }
        
        while( pSource < pSourceFrameEnd )
        {
            for( unsigned int c = 0; c < numChannels; c ++ )
            {
                char nibble1 = *pSource & 0x0F;
                char nibble2 = ( (*pSource) >> 4 ) & 0x0F;
                pSource++;
                
                int index = (int) channelState[ c ].index;
                int prev  = (int) channelState[ c ].prev;                

                *pTarget = (short) inlRadicalAdpcmDecode( nibble1, & index, & prev );
                *(pTarget + numChannels) = (short) inlRadicalAdpcmDecode( nibble2, & index, & prev );

                channelState[ c ].index = (short) index;
                channelState[ c ].prev  = (short) prev;
                
                pTarget++;
            }
            
            pTarget += numChannels;
        }
    }

    #ifdef PROFILE_DECODING
        
        unsigned int endTime = radTimeGetMicroseconds( );

        unsigned int elapsed = endTime - startTime;

        totalBytes += totalPcmBytes;
        totalMicroseconds += elapsed;
        
        float fTotalMicroseconds = radSoundUIntToFloat( totalMicroseconds );
        float bytesPerSec = ( radSoundUIntToFloat( totalBytes )  ) / ( fTotalMicroseconds / 1000000.0f );

        float cpu = ( fTotalMicroseconds / 1000.0f ) / radSoundUIntToFloat( runningTimeMs );

        rTunePrintf(
            "ADPCM Decode: [%f] bytes/sec, [%f%%] of CPU time, ([%d] bytes in [%d]usec) \n",
            bytesPerSec, cpu, totalPcmBytes, elapsed );

    #endif
}

RadicalAdpcmDecodeStream* RadicalAdpcmDecodeStreamCreate( radMemoryAllocator a )
{
    return new ( a ) RadicalAdpcmDecodeStream;

}

RadicalAdpcmDecodeStream::RadicalAdpcmDecodeStream( void )
    :
    radRefCount( 0 )
{
}

RadicalAdpcmDecodeStream::~RadicalAdpcmDecodeStream( void )
{
    rAssert( info.state != RadicalAdpcmDecodeState_ReadingSource );

    info.pHalStream->Release( );
    
    if ( info.pHalAudioFormat != NULL )
    {
        info.pHalAudioFormat->Release( );
    }
}

    
void RadicalAdpcmDecodeStream::Initialize( IRadSoundHalDataSource* pHalStream )
{
    rAssert( NULL != pHalStream );
    
    info.pHalStream = pHalStream;
    info.pHalStream->AddRef( );
    info.state = RadicalAdpcmDecodeState_InitializingSource;
    info.pHalAudioFormat = 0;
    DoWork( );
}

        
void RadicalAdpcmDecodeStream::DoWork( void )
{
    //info.pHalStream->DoWork( );
    
    switch( info.state )
    {
        case RadicalAdpcmDecodeState_UnInitialized:
        {
            rAssert( false );
            break;
        }
        case RadicalAdpcmDecodeState_InitializingSource:
        {
            switch( info.pHalStream->GetState( ) )
            {
                case IRadSoundHalDataSource::Initializing:
                {
                    break;
                }
                case IRadSoundHalDataSource::Initialized:
                {
                    IRadSoundHalAudioFormat* pInFormat = info.pHalStream->GetFormat( );
                    
                    rAssert( (IRadSoundHalAudioFormat::RadicalAdpcm) == pInFormat->GetEncoding( ) );
                    rAssert( 16 == pInFormat->GetBitResolution( ) );
                    
                    info.pHalAudioFormat = radSoundHalAudioFormatCreate( RADMEMORY_ALLOC_TEMP );
                    info.pHalAudioFormat->AddRef( );
                    info.pHalAudioFormat->Initialize(
                        IRadSoundHalAudioFormat::PCM,
                        NULL,
                        pInFormat->GetSampleRate( ),
                        pInFormat->GetNumberOfChannels( ),
                        pInFormat->GetBitResolution( ) );
                        
                    info.state = RadicalAdpcmDecodeState_Idle;
                    break;
                }
                case IRadSoundHalDataSource::Error:
                {
                    info.state = RadicalAdpcmDecodeState_Error;
                    break;
                }
                default:
                {
                    rAssert( false );
                }
            }
        }
        case RadicalAdpcmDecodeState_Idle:
        {
            break;
        }
        case RadicalAdpcmDecodeState_ReadingSource:
        {
            if ( true == info.read.done )
            {
                DecodeFrames(
                    info.read.pBufferStart,
                    info.read.pAdpcmStart,
                    info.read.framesRead,
                    info.pHalAudioFormat->GetNumberOfChannels( ) );
                
                if ( info.read.framesRead < info.read.framesReading )
                {
                    info.state = RadicalAdpcmDecodeState_Finished;                
                }
                else
                {
                    info.state = RadicalAdpcmDecodeState_Idle;
                }
                
                IRadSoundHalDataSourceCallback* pCallback = info.read.pHalStreamCallback;
                
                unsigned int pcmFrames = info.read.framesRead * RADICAL_ADPCM_SAMPLES_PER_FRAME;
                
                pCallback->OnDataSourceFramesLoaded( pcmFrames );
                
                pCallback->Release( );
            }
            break;
        }
        case RadicalAdpcmDecodeState_Finished:
        {
            break;
        }
        case RadicalAdpcmDecodeState_Error:
        {
            break;
        }
    }
}

IRadSoundHalDataSource::State RadicalAdpcmDecodeStream::GetState( void )
{
    DoWork( );
    
    switch( info.state )
    {
        case RadicalAdpcmDecodeState_InitializingSource:
        {
            return IRadSoundHalDataSource::Initializing;
        }
        case RadicalAdpcmDecodeState_Idle:
        case RadicalAdpcmDecodeState_ReadingSource:
        case RadicalAdpcmDecodeState_Finished:
        {
            return IRadSoundHalDataSource::Initialized;
        }
        case RadicalAdpcmDecodeState_Error:
        {
            return IRadSoundHalDataSource::Error;
        }
        case RadicalAdpcmDecodeState_UnInitialized:
        default:
        {
            rAssert( false );
            return (IRadSoundHalDataSource::State) -1;
        }        
    }
}

IRadSoundHalAudioFormat * RadicalAdpcmDecodeStream::GetFormat( void )
{
    rAssert( IRadSoundHalDataSource::Initialized == GetState( ) );
    
    return info.pHalAudioFormat;
}

unsigned int RadicalAdpcmDecodeStream::GetRemainingFrames( void )
{
    rAssert( IRadSoundHalDataSource::Initialized == GetState( ) );
    
    unsigned int sourceFrames = info.pHalStream->GetRemainingFrames( );
    
    if ( 0xFFFFFFFF == sourceFrames )
    {
        return sourceFrames;
    }

    return sourceFrames * RADICAL_ADPCM_SAMPLES_PER_FRAME;

}

void RadicalAdpcmDecodeStream::GetFramesAsync(
    void* pBytes,
    radMemorySpace destinationMemorySpace,
    unsigned int numberOfFrames,
    IRadSoundHalDataSourceCallback * pCallback )
{
    rAssert( pBytes != NULL );
    rAssert( radMemorySpace_Local == destinationMemorySpace );

    switch( info.state )
    {
        case RadicalAdpcmDecodeState_Idle:
        {
            rAssert( ( numberOfFrames % RADICAL_ADPCM_SAMPLES_PER_FRAME ) == 0 );
            
            unsigned int pcmBytes = 
                numberOfFrames * info.pHalAudioFormat->GetNumberOfChannels( )* 2;
                
            unsigned int adpcmFrames
                = numberOfFrames / RADICAL_ADPCM_SAMPLES_PER_FRAME;
            
            unsigned int adpcmBytes =
                adpcmFrames *
                RADICAL_ADPCM_FRAME_SIZE *
                info.pHalAudioFormat->GetNumberOfChannels( );
         
            info.read.done = false;
            info.read.framesReading = adpcmFrames;
            info.read.pBufferStart = pBytes;
            info.read.pAdpcmStart = (((char*)pBytes) + pcmBytes - adpcmBytes);
            info.read.pHalStreamCallback = pCallback;
            info.read.pHalStreamCallback->AddRef( );
            info.state = RadicalAdpcmDecodeState_ReadingSource;
                            
            info.pHalStream->GetFramesAsync(
                info.read.pAdpcmStart,
                destinationMemorySpace,
                adpcmFrames,
                this );

            break;
        }
        case RadicalAdpcmDecodeState_Finished:
        {
            pCallback->OnDataSourceFramesLoaded( 0 );
            break;
        }
        default:
        {
            rAssert( false );
        }
    }
}

void RadicalAdpcmDecodeStream::OnDataSourceFramesLoaded( unsigned int framesRead )
{
    rAssert( false == info.read.done );
    rAssert( RadicalAdpcmDecodeState_ReadingSource == info.state );
                                       
    info.read.done = true;
    info.read.framesRead = framesRead;
    
    DoWork( );
}

IRadSoundAdpcmDecodeStream  * radSoundAdpcmDecodeStreamCreate( radMemoryAllocator alloc )
{
    return new ( alloc ) RadicalAdpcmDecodeStream( );
}
    