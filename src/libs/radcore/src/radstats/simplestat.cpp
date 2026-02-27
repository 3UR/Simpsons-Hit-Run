//===========================================================================
// Copyright (C) 2001 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   Statistics collection service.
//
// Description: This file contains the implementation of the basic 
//              statistic collection and reporting class.
//
// Author:      Soleil Lapierre
//
//===========================================================================


//===========================================================================
// Includes
//===========================================================================

#include "pch.hpp"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <radstats.hpp>    // interface specification
#include <radmemorymonitor.hpp>
#include "simplestat.hpp"  // class specification


//===========================================================================
// Global Data, Local Data, Local Classes
//===========================================================================

//===========================================================================
// Static Data Definitions
//===========================================================================

//===========================================================================
// Global Functions
//===========================================================================

//===========================================================================
// radStatSimpleStatisticFactory
//===========================================================================
// Description: Creates a new object implementing IRadSimpleStatistic,
//              addref it and return it. 
//
// Constraints:	The caller is responsible for calling Release on the returned
//              object when finished with it.
//
// Parameters:	pNewStat - place to store pointer to the new object.
//              pBufferSize - Optional buffer size for the stat's history.
//
//===========================================================================

void radStatCreateSimpleStatistic
( 
    IRadSimpleStatistic** pNewStat, 
    unsigned int pBufferSize,
    radMemoryAllocator alloc
)
{
    rAssert( pNewStat != NULL );

    *pNewStat = new( alloc ) radSimpleStatistic( pBufferSize );
    rAssert( *pNewStat != NULL );
    radAddRef( *pNewStat, NULL );
}


//===========================================================================
// Member functions
//===========================================================================

//===========================================================================
// radSimpleStatistic::radSimpleStatistic
//===========================================================================
// Description: Default constructor. Initializes the object with the 
//              appropriate values for the reset state, and with the
//              default buffer size.
//
// Parameters:	None.
//              
// Returns:     None.
//
//===========================================================================

radSimpleStatistic::radSimpleStatistic( void )
    :
    m_ReferenceCount( 0 ),
    m_Samples( NULL ),
    m_SortedSamples( NULL ),
    m_NextIndex( 0 ),
    m_BufferFill( 0 ),
    m_NumSamples( 0 ),
    m_Total( 0 ),
    m_Max( 0 ),
    m_Min( 0 ),
    m_Mean( 0.0 ),
    m_MeanIsCurrent( false ),
    m_SortingIsCurrent( false )
{
    radMemoryMonitorIdentifyAllocation( this, g_nameFTech, "radSimpleStatistic" );
    m_Name[ 0 ] = '\0';
    Reset( 0 );
}


//===========================================================================
// radSimpleStatistic::radSimpleStatistic
//===========================================================================
// Description: This constructor initializes the object to the reset state
//              buf allows the internal buffer size to be specified.
//
// Parameters:	bufferSize - the number of history entries to keep.
//              
// Returns:     None.
//
//===========================================================================

radSimpleStatistic::radSimpleStatistic( unsigned int bufferSize )
    :
    m_ReferenceCount( 0 ),
    m_Samples( NULL ),
    m_SortedSamples( NULL ),
    m_NextIndex( 0 ),
    m_BufferFill( 0 ),
    m_NumSamples( 0 ),
    m_Total( 0 ),
    m_Max( 0 ),
    m_Min( 0 ),
    m_Mean( 0.0 ),
    m_MeanIsCurrent( false ),
    m_SortingIsCurrent( false )
{
    radMemoryMonitorIdentifyAllocation( this, g_nameFTech, "radSimpleStatistic" );
    m_Name[ 0 ] = '\0';
    Reset( bufferSize );
}


//===========================================================================
// radSimpleStatistic::~radSimpleStatistic
//===========================================================================
// Description: Destructor. Frees associated memory when the object is deleted.
//
// Parameters:	None.
//              
// Returns:     None.
//
//===========================================================================

radSimpleStatistic::~radSimpleStatistic( void )
{
    if( m_Samples != NULL )
    {
        radMemoryFree( GetThisAllocator( ), m_Samples );
    }

    if( m_SortedSamples != NULL )
    {
        radMemoryFree( GetThisAllocator( ), m_SortedSamples );
    }
}


//===========================================================================
// radSimpleStatistic::AddRef
//===========================================================================
// Description: Increments the reference count of this object instance.
//
// Parameters:	None.
//              
// Returns:     None.
//
//===========================================================================

void radSimpleStatistic::AddRef( void )
{
    m_ReferenceCount++;
}


//===========================================================================
// radSimpleStatistic::Release
//===========================================================================
// Description: Decrements the reference count of this object instance,
//              and deletes the object if the count reaches zero.
//
// Parameters:	None.
//              
// Returns:     None.
//
//===========================================================================

void radSimpleStatistic::Release( void )
{
    m_ReferenceCount--;
    if( m_ReferenceCount < 1 )
    {
        delete this;
    }
}


//===========================================================================
// radSimpleStatistic::SetName
//===========================================================================
// Description: Assign a user-defined name to the accumulator. This ability
//              is provided just as a convenience for the user.
//
// Parameters:	name - the string to name the register with.
//              
// Returns:     None.
//
// Notes:       Maximum length is 255 characters.
//
//===========================================================================

void radSimpleStatistic::SetName( const char* name )
{
    rAssert( name != NULL );
    rAssert( strlen( name ) < 256 );
    strcpy( m_Name, name );
}


//===========================================================================
// radSimpleStatistic::GetName
//===========================================================================
// Description: Retrieve the user-assigned name of the accumulator.
//
// Parameters:	None.
//              
// Returns:     pName - buffer to copy the name string to. Must be big enough.
//
//===========================================================================

void radSimpleStatistic::GetName( char* pName ) const
{
    rAssert( pName != NULL );
    strcpy( pName, m_Name );
}


//===========================================================================
// radSimpleStatistic::Reset
//===========================================================================
// Description: Reset the statistic to its initial, zeroed state, without
//              changing the history buffer size.
//
// Parameters:	None.
//              
// Returns:     None.
//
//===========================================================================

void radSimpleStatistic::Reset( void )
{
    //
    // Reset other members to initial states.
    //
    m_NextIndex = 0;
    m_BufferFill = 0;
    m_NumSamples = 0;
    m_Total = 0;
    m_Max = m_Min = 0;
    m_Mean = 0.0;
    m_MeanIsCurrent = false;
    m_SortingIsCurrent = false;
}


//===========================================================================
// radSimpleStatistic::Reset
//===========================================================================
// Description: Reset the statistic to its initial, zeroed state, and
//              optionally resize its internal history buffer. The history
//              buffer records the samples that have been entered so that
//              the standard deviation and percentiles can be calculated.
//
// Constraints:	You can pass in a buffer size of zero, in which case no
//              history is saved and the standard deviation and percentile
//              queries will return meaningless results.
//
// Parameters:	bufferSize - the number of samples to keep in the history.
//                  More samples means better accuracy but eats memory.
//              
// Returns:     None.
//
//===========================================================================

void radSimpleStatistic::Reset( unsigned int bufferSize )
{
    //
    // If we need to resize the history buffer, delete the old one first.
    //
    if( m_Samples != NULL && bufferSize != m_BufferSize )
    {
        radMemoryFree( GetThisAllocator( ), m_Samples );
        m_Samples = NULL;
    }

    //
    // Resetting invalidates the history, so delete the sorting buffer.
    //
    if( m_SortedSamples != NULL )
    {
        radMemoryFree( GetThisAllocator( ), m_SortedSamples );
        m_SortedSamples = NULL;
    }

    //
    // Allocate new history buffer if needed.
    //
    if( m_Samples == NULL && bufferSize > 0 )
    {
        m_Samples = ( float* )radMemoryAlloc( GetThisAllocator( ), sizeof( float ) * bufferSize );
        rAssert( m_Samples != NULL );
    }
    m_BufferSize = bufferSize;

    //
    // Reset other members to initial states.
    //
    Reset( );
}


//===========================================================================
// radSimpleStatistic::AddSample
//===========================================================================
// Description: Record a new statistical sample. This is where new data
//              gets added to the statistic and modifies its values.
//
// Parameters:	sample - a number representing a measurement of whatever it
//                      is we are keeping statistics on.
//              
// Returns:     None.
//
//===========================================================================

void radSimpleStatistic::AddSample( float sample )
{
    //
    // Update minimax data.
    //
    if( m_NumSamples == 0 )
    {
        m_Min = m_Max = sample;
    }
    else
    {
        if( sample < m_Min )
        {
            m_Min = sample;
        }
        if( sample > m_Max )
        {
            m_Max = sample;
        }
    }

    // 
    // Update basic stuff.
    //
    m_Total += sample;
    m_NumSamples++;

    //
    // Invalidate calculated stuff.
    //
    m_MeanIsCurrent = false;
    m_SortingIsCurrent = false;

    //
    // Record sample in history buffer.
    //
    if( m_Samples != NULL )
    {
        m_Samples[ m_NextIndex ] = sample;
        m_NextIndex++;
        if( m_NextIndex >= m_BufferSize )
        {
            m_NextIndex = 0;
        }
        if( m_BufferFill < m_BufferSize )
        {
            m_BufferFill++;
        }
    }
}


void radSimpleStatistic::AddSample( int sample )
{
    float s = ( float ) sample;
    AddSample( s );
}


//===========================================================================
// radSimpleStatistic::GetTotal
//===========================================================================
// Description: Returns the sum of all samples recorded since the last reset.
//
// Parameters:	None.
//              
// Returns:     The total of all samples so far.
//
//===========================================================================

float radSimpleStatistic::GetTotal( void ) const
{
    return( m_Total );
}


//===========================================================================
// radSimpleStatistic::GetNumSamples
//===========================================================================
// Description: Returns the number of samples recorded since the last reset.
//
// Parameters:	None.
//              
// Returns:     The number of samples entered so far.
//
//===========================================================================

unsigned int radSimpleStatistic::GetNumSamples( void ) const
{
    return( m_NumSamples );
}


//===========================================================================
// radSimpleStatistic::GetMax
//===========================================================================
// Description: Returns the largest sample value recorded so far.
//
// Constraints:	Returns zero if no samples have been entered since reset.
//
// Parameters:	None.
//              
// Returns:     The max of all samples entered so far.
//
//===========================================================================

float radSimpleStatistic::GetMax( void ) const
{
    return( m_Max );
}


//===========================================================================
// radSimpleStatistic::GetMin
//===========================================================================
// Description: Returns the smallest sample value recorded so far.
//
// Constraints:	Returns zero if no samples have been entered since reset.
//
// Parameters:	None.
//              
// Returns:     The min of all samples entered so far.
//
//===========================================================================

float radSimpleStatistic::GetMin( void ) const
{
    return( m_Min );
}


//===========================================================================
// radSimpleStatistic::GetMean
//===========================================================================
// Description: Compute and return the average of all samples recorded so
//              far.
//
// Constraints:	Returns zero if no samples have been entered.
//
// Parameters:	None.
//              
// Returns:     The average of all samples entered since last reset.
//
//===========================================================================

float radSimpleStatistic::GetMean( void )
{
    //
    // Recompute the mean if necessary.
    //
    if( !m_MeanIsCurrent && m_NumSamples > 0 )
    {
        m_Mean = m_Total / ( float )m_NumSamples;
        m_MeanIsCurrent = true;
    }
    return( m_Mean );
}


//===========================================================================
// radSimpleStatistic::GetStandardDeviation
//===========================================================================
// Description: Compute and return the standard deviation of the samples
//              saved in the history buffer.
//
// Constraints:	Returns zero if no samples have been entered or if the
//              history has zero length.
//
//              If the history buffer is not big enough to record all the
//              samples that have been entered, the older ones are discarded
//              resulting in a loss of accuracy in this computation.
//
// Parameters:	None.
//              
// Returns:     The standard deviation of the recorded sample data.
//
//===========================================================================

float radSimpleStatistic::GetStandardDeviation( void )
{
    //
    // Trap error condition: If there are no samples, let's
    // define the deviation to be zero.
    //
    if( m_NumSamples == 0 || m_Samples == NULL )
    {
        return( 0.0 );
    }

    //
    // Update the mean if need be.
    //
    if( !m_MeanIsCurrent )
    {
        GetMean( );
    }

    //
    // Sum the differences of the sample data from the mean.
    //
    float totalDeviation = 0.0;
    for( unsigned int i = 0; i < m_BufferFill; i++ )
    {
        float delta = m_Samples[ i ] - m_Mean;
        totalDeviation += ( delta * delta );
    }

    //
    // Calculate the variance from the deviation.
    //
    float variance = totalDeviation / ( float ) m_NumSamples;

    //
    // Now return the standard deviation.
    //
    rAssert( variance >= 0.0 );
    return( ( float )sqrtf( variance ) );
}


//===========================================================================
// radSimpleStatistic::GetPercentile
//===========================================================================
// Description: Performs a percentile query on the sample data saved in 
//              the history buffer.
//
//              What is a percentile query? You provide a percentage (an
//              integer between 1 and 99) and get back the sample value
//              that is above or equal to that percentage of the samples.
//              
//              For example, if you ask for the 90th percentile of a set
//              of disk seek times and get back 10mS as an answer, then
//              you know that 90% of disk seek times are 10mS or less.
//
// Constraints:	Returns the mean if no samples have been entered or if the
//              history has zero length.
//
//              If the history buffer is not big enough to record all the
//              samples that have been entered, the older ones are discarded
//              resulting in a loss of accuracy in this computation.
//
// Parameters:	percent - a number between 0 and 100 inclusive representing
//                      the percentile you are interested in. Other values
//                      are clamped to this range.
//
// Returns:     The value which is above or equal to the percentage of the
//              recorded samples specified by the parameter.
//
// Notes:       The 0th percentile is the min, the 100th percentile is 
//              the max, and the 50th percentile is what is called the median.
//
//===========================================================================

float radSimpleStatistic::GetPercentile( unsigned int percent )
{
    //
    // On error condition, just return the mean.
    //
    if( m_Samples == NULL || m_NumSamples == 0 )
    {
        return( GetMean( ) );
    }

    //
    // Handle trivial cases.
    //
    if( percent == 0 )
    {
        return( GetMin( ) );
    }
    else if( percent >= 100 )
    {
        return( GetMax( ) );
    }

    //
    // If the data is not already sorted, sort it.
    //
    if( !m_SortingIsCurrent || m_SortedSamples == NULL )
    {
        //
        // First allocate a buffer if needed.
        //
        if( m_SortedSamples == NULL )
        {
            m_SortedSamples = ( float* )radMemoryAlloc( GetThisAllocator( ), sizeof( float ) * m_BufferSize );
            rAssert( m_SortedSamples != NULL );
        }

        //
        // Copy data from one buffer to the other.
        //
        memcpy( m_SortedSamples, m_Samples, m_BufferSize * sizeof( float ) );

        //
        // Sort the second buffer.
        //
        qsort( m_SortedSamples, m_BufferFill, sizeof( float ), FloatCompare );

        //
        // Set sorted flag to eliminate redundant work.
        //
        m_SortingIsCurrent = true;
    }

    //
    // Calculate the index of the percentile and round to the nearest integer index.
    //
    float realIndex = ( ( float ) ( m_BufferSize - 1 ) * ( float ) percent ) / ( float )100.0;
    int index = ( int )( realIndex + 0.5 );
    rAssert( index < ( int ) m_BufferSize );
    rAssert( index >= 0.0 );
    
    //
    // Return the sample at that index in the sorted array.
    //
    return( m_SortedSamples[ index ] );
}



//=============================================================================
// Function:    radSimpleStatistic::Dump
//=============================================================================
// Description: In a debug build, report the name of this class.
//
// Parameters:  pStringBuffer - where to copy name,
//              bufferSize    - size of buffer
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

#ifdef RAD_DEBUG

void radSimpleStatistic::Dump( char * pStringBuffer, unsigned int bufferSize )
{
    sprintf( pStringBuffer, "Object: [radSimpleStatistic] At Memory Location:[%p]\n", this );
}

#endif

//===========================================================================
// FloatCompare
//===========================================================================
// Description: A private sorting test for use with qsort. For some reason 
//              this wouldn't work as a class member.
//
// Parameters:	Two pointers to floats in an array of floats.
//
// Returns:     A comparison result that can be used to sort the array.
//
//===========================================================================

int FloatCompare( const void* arg1, const void* arg2 )
{
    float f1 = *( float* ) arg1;
    float f2 = *( float* ) arg2;

    if( f1 > f2 )
    {
        return( 1 );
    }
    else if ( f1 < f2 )
    {
        return( -1 );
    }

    return( 0 );
}

