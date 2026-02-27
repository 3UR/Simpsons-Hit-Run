//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        radstats.hpp
//
// Subsystem:   Foundation Technologies Profiling Services
//
// Description: This file contains all definitions and functions prototypes
//              required to use statistics collection services.
//
// Revisions:   V1.00 November 8, 2001           Creation
//
//=============================================================================

//
// How the statistics collection system works.
//
// This facility provides the ability to allocate a set of statistical
// accumulators, update them, and query them without knowing their
// interface definition. Hiding the interface behind global functions
// like this makes it easy to nullify the code in release builds. Profiling
// eats CPU time and is not needed in a release.
//
// Each module that wants to profile itself must identify itself with
// a name string - ie, "radfile", "movieplayer" etc. This string is
// used to keep separate sets of accumulators for each module.
//
// The module must initially specify how many accumulators it wants to
// use, and thereafter must address the accumulators with an integer
// index between 0 and N-1, where N is the number of accumulators requested.
//
// Each accumulator collects numeric samples. These can be routine service
// times, memory allocation sizes, etc. For now the stats system only deals
// with signed floating point data, but the user can decide the meaning and units
// of the data. 
//
// An accumulator can be queried for the following information about the
// data it has been given:
// - Number of samples.
// - Sum (total) of the samples.
// - Mean (average) of the samples.
// - Maximum and minimum sample values.
// - Standard deviation of the samples.
// - Percentile queries ("X% of the samples are at or below what value?", where X is between 1 and 99)
//
// The standard deviation and percentile queries require storing and sorting
// all the individual samples, so the caller should also provide an array
// size for each accumulator to use for this purpose. The default is 1000,
// which may be wastefully large for some applications or too small for others,
// If more samples are recorded than the array allows, older ones are
// pushed off and the std.dev and percentile queries become less accurate.
// Because they are time-consuming, the standard deviation and percentile
// computations are only performed upon request.
// 
// The reset call can be used to clear an accumulator and resize its
// sample storage table.
//
// Typical usage scenario:
// ... Create and name some accumulators.
// radStatInitialize( "mymodulename", 2 );
// radStatSetAccumulatorName( "mymodulename", 0, "Time" );
// radStatSetAccumulatorName( "mymodulename", 1, "Memory" );
// ... measure some processing time
// radStatAddSample( "mymodulename", "Time", timeSpent );
// ... allocate some memory
// radStatAddSample( "mymodulename", "Memory", numBytes );
// ... free some memory
// radStatAddSample( "mymodulename", "Memory", -numBytes );
// ... at end of run, get info
// radStatQuery( "mymodulename", "Time", NULL, &totalTime, NULL, NULL, NULL, NULL );
// printf( "Total time is %f\n", totalTime );
// radStatQuery( "mymodulename", "Memory", NULL, NULL, &avgMem, &maxMem, NULL, NULL );
// printf( "Memory usage peak = %d bytes, average = %d bytes\n", (int)maxMem, (int)avgMem );
// ... free accumulators when program exits
// radStatTerminate( "mymodulename" );
//

#ifndef  RADSTATS_HPP
#define  RADSTATS_HPP

//=============================================================================
// Build Configuration Check
//=============================================================================

#if !defined(RAD_DEBUG) && !defined(RAD_TUNE) && !defined(RAD_RELEASE) 
    #error 'FTech requires definition of RAD_DEBUG, RAD_TUNE, or RAD_RELEASE'
#endif

//=============================================================================
// Include Files
//=============================================================================

#include <radmemory.hpp>
#include <radobject.hpp>

//
// Setting this default to zero will save memory by default, but will
// disable the standard deviation and percentile queries - to re-enable
// then you will have to explicitly reset the stats accumulators that you
// will want to run those queries on, in order to resize their buffers.
//
#define RADSTAT_DEFAULT_NUM_SAMPLES 0

//
// DON'T USE THESE FUNCTIONS. USE THE MACROS BELOW INSTEAD.
// This is so that the macros can be redefined to compile out the
// time-wasting profiling code in a release build.
//
// Documentation for these functions can be found where they are
// implemented.
//

#ifdef PROFILE
void radStatInitialize_Implementation( const char* moduleName, unsigned int numEntries, 
                                       unsigned int numSamplesPerEntry, radMemoryAllocator alloc );
void radStatTerminate_Implementation( const char* moduleName );
void radStatSetAccumulatorName_Implementation( const char* moduleName, unsigned int index,
                                               const char* accumName );
void radStatGetAccumulatorName_Implementation( const char* moduleName, unsigned int index,
                                               char* accumName );
void radStatReset_Implementation( const char* moduleName, unsigned int index,
                                  unsigned int numSamplesToStore );
void radStatReset_Implementation( const char* moduleName, const char* accumName,
                                  unsigned int numSamplesToStore );
void radStatAddSample_Implementation( const char* moduleName, unsigned int index, float sample );
void radStatAddSample_Implementation( const char* moduleName, const char* accumName, float sample );
void radStatAddSample_Implementation( const char* moduleName, unsigned int index, int sample );
void radStatAddSample_Implementation( const char* moduleName, const char* accumName, int sample );
void radStatQuery_Implementation( const char* moduleName, unsigned int index,
                                  unsigned int* pNumSamples, float* pTotal, 
                                  float* pMean, float* pMax, float* pMin,
                                  float* pStandardDeviation );
void radStatQuery_Implementation( const char* moduleName, const char* accumName,
                                  unsigned int* pNumSamples, float* pTotal, 
                                  float* pMean, float* pMax, float* pMin,
                                  float* pStandardDeviation );
void radStatQueryPercentile_Implementation( const char* moduleName, unsigned int index,
                                            unsigned int percentile, float* pValueBelow );
void radStatQueryPercentile_Implementation( const char* moduleName, const char* accumName,
                                            unsigned int percentile, float* pValueBelow );
void radStatEnumerateAccumulators_Implementation( const char* modulePattern, const char* accumPattern,
                       void (*enumerationCallback)( const char* moduleName, const char* accumName ) );

#endif // PROFILE

//
// USE THESE MACROS to use the statistics manager for profiling.
//

#ifdef PROFILE
    #define radStatInitialize( x, y, z, a ) ( radStatInitialize_Implementation( x, y, z, a ) )
    #define radStatTerminate( x ) ( radStatTerminate_Implementation( x ) )
    #define radStatReset( x, y, z ) ( radStatReset_Implementation( x, y, z ) )
    #define radStatAddSample( x, y, z ) ( radStatAddSample_Implementation ( x, y, z ) )
    #define radStatQuery( a, b, c, d, e, f, g, h ) ( radStatQuery_Implementation( a, b, c, d, e, f, g, h ) )
    #define radStatQueryPercentile( a, b, c, d ) ( radStatQueryPercentile_Implementation( a, b, c, d ) )
    #define radStatSetAccumulatorName( x, y, z ) ( radStatSetAccumulatorName_Implementation( x, y, z ) )
    #define radStatGetAccumulatorName( x, y, z ) ( radStatGetAccumulatorName_Implementation( x, y, z ) )
    #define radStatEnumerateAccumulators( x, y, z ) ( radStatEnumerateAccumulators_Implementation( x, y, z ) )
#else
    #define radStatInitialize( x, y, z, a ) ((void)0)
    #define radStatTerminate( x ) ((void)0)
    #define radStatReset( x, y, z ) ((void)0)
    #define radStatAddSample( x, y, z ) ((void)0)
    #define radStatQuery( a, b, c, d, e, f, g, h ) ((void)0)
    #define radStatQueryPercentile( a, b, c, d ) ((void)0)
    #define radStatSetAccumulatorName( x, y, z ) ((void)0)
    #define radStatGetAccumulatorName( x, y, z ) ((void)0)
    #define radStatEnumerateAccumulators( x, y, z ) ((void)0)
#endif // PROFILE


//
// The interface definitions below are not required in order to use the
// statistics collection service, but they are made public in case they
// are useful for other purposes.
//

//
// Interface for a simple statistical analysis tool. It records
// a sequence of numbers and can then answer queries about the
// data it has recorded. 
//
struct IRadSimpleStatistic : public IRefCount
{
    //
    // Clear the accumulated statistics and optionally
    // resize the buffer used for recording them.
    //
    virtual void Reset( void ) = 0;
    virtual void Reset( unsigned int bufferSize ) = 0;

    //
    // Convenience methods to assign a name to the accumulator.
    //
    virtual void SetName( const char* name ) = 0;
    virtual void GetName( char* pName ) const = 0;

    //
    // Accessor methods.
    //

    //
    // Record a new measurement.
    //
    virtual void AddSample( float sample ) = 0;
    virtual void AddSample( int sample ) = 0;

    //
    // Get the sum of all samples recorded.
    //
    virtual float GetTotal( void ) const = 0;

    //
    // Get the number of samples recorded.
    //
    virtual unsigned int GetNumSamples( void ) const = 0;

    //
    // Get the max and min sample values recorded.
    //
    virtual float GetMax( void ) const = 0;
    virtual float GetMin( void ) const = 0;

    //
    // Get the mean (average) of the samples.
    //
    virtual float GetMean( void ) = 0;

    //
    // Compute the standard deviation of the samples
    // saved in the internal buffer.
    //
    virtual float GetStandardDeviation( void ) = 0;

    //
    // Answer a percentile query about the samples
    // saved in the internal buffer.
    //
    virtual float GetPercentile( unsigned int percent ) = 0;
};

//
// Factory for creating a new statistic object.
//
void radStatCreateSimpleStatistic( IRadSimpleStatistic** pNewStat, 
                                   unsigned int pBufferSize = RADSTAT_DEFAULT_NUM_SAMPLES,
                                   radMemoryAllocator alloc = RADMEMORY_ALLOC_DEFAULT );


#endif // RADSTATS

