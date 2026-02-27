//===========================================================================
// Copyright (C) 2001 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   Radical performance statistics collection service.
//
// Description: This file contains classes and definitions relevant to the
//              simple statistic interface.
//
// Authors:     Soleil Lapierre
//
//===========================================================================

// Recompilation protection flag.
#ifndef __SIMPLESTAT_H
#define __SIMPLESTAT_H

//===========================================================================
// Nested Includes
//===========================================================================

#include <stdio.h>
#include <radmemory.hpp>
#include <raddebug.hpp>
#include <radstats.hpp>

#define MODULE_NAME_MAX_LENGTH 256


//
// Compare function for use with qsort.
// It won't work if it is a member of the class.
//
int FloatCompare( const void* arg1, const void* arg2 );


//===========================================================================
// Private Class Definitions
//===========================================================================

//===========================================================================
//
// Description: This private class implements a simple numeric statistics
//              accumulator. It can answer queries about the number of
//              samples recorded, the sum, max, min and mean (average) of
//              the samples. It can also perform standard deviation and
//              percentile calculations, but if the number of samples
//              exceeds its internal storage capacity, the accuracy is
//              reduced.
//
//===========================================================================

class radSimpleStatistic : public IRadSimpleStatistic,
                           public radObject
{
    public:

    //
    // Constructor. 
    //
    radSimpleStatistic( void );
    radSimpleStatistic( unsigned int bufferSize );

    //
    // Destructor.
    // The destructor is public because the stats mananger needs to
    // create and delete arrays of these objects.
    //
    ~radSimpleStatistic( void );

    //
    // IRefCount stuff.
    //
    virtual void AddRef( void );
    virtual void Release( void );

    //
    // Reset and resize method.
    //
    virtual void Reset( void );
    virtual void Reset( unsigned int bufferSize );

    //
    // IRadSimpleStatistic stuff.
    //
    virtual void SetName( const char* name );
    virtual void GetName( char* pName ) const;
    virtual void AddSample( float sample );
    virtual void AddSample( int sample );
    virtual float GetTotal( void ) const;
    virtual unsigned int GetNumSamples( void ) const;
    virtual float GetMax( void ) const;
    virtual float GetMin( void ) const;
    virtual float GetMean( void );
    virtual float GetStandardDeviation( void );
    virtual float GetPercentile( unsigned int percent );

    #ifdef RAD_DEBUG
    virtual void Dump( char * pStringBuffer, unsigned int bufferSize );
    #endif

    private:

    //
    // Reference counting member.
    //
    int m_ReferenceCount;

    //
    // Register name.
    //
    char m_Name[ 256 ];

    // 
    // Source data.
    // These items are updated each time a new sample is added.
    //
    float* m_Samples;          // Sample history buffer.
    float* m_SortedSamples;    // Sorted sample data.
    unsigned int m_BufferSize; // Size of buffer.
    unsigned int m_NextIndex;  // Next available slot in buffer.
    unsigned int m_BufferFill; // Number of slots used in the buffer.

    unsigned int m_NumSamples; // Number of samples entered.
    float m_Total;             // Sum of all samples.
    float m_Max, m_Min;        // Max and min values encountered.

    //
    // These items are updated only when a query is made, to save
    // time during statistics gathering. They are used for internal
    // calculation rather than for answering queries.
    //
    float m_Mean;              // Average so far.
    bool m_MeanIsCurrent;      // Mean has been updated?
    bool m_SortingIsCurrent;   // Sorted data has been updated?
};


#endif   // file

