//===========================================================================
// Copyright (C) 2001 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   Radical time services.
//
// Description: This file implements a "stopwatch" convenience object for
//              recording timings.
//
// Authors:     Soleil Lapierre
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
#include <radtime.hpp>     // interface specification
#include <radmemorymonitor.hpp>
#include "stopwatch.hpp"   // class specification



//===========================================================================
// Global Functions
//===========================================================================

//===========================================================================
// radTimeCreateStopwatch
//===========================================================================
// Description: Create and addref a new object that implements the 
//              IRadStopwatch interface.
//
// Constraints:	The caller is responsible for calling Release on the returned
//              object when finished with it.
//
// Parameters:	pNewWatch - place to store pointer to new object.
//              timeUnit - time unit for the new watch to use.
//
//===========================================================================

void radTimeCreateStopwatch
( 
    IRadStopwatch** pNewWatch, 
    radTimeUnit timeUnit, 
    radMemoryAllocator alloc
)
{
    rAssert( pNewWatch != NULL );

    *pNewWatch = new( alloc ) radStopwatch;
    rAssert( *pNewWatch != NULL );

    ( *pNewWatch )->SetTimeUnits( timeUnit );
}

//=============================================================================
// Function:    radTimeGetUnitName
//=============================================================================
// Description: Convert a time unit enum to a string suitable for printing 
//              after a number.
//
// Parameters:  timeUnit - the time unit to get the name of.
//              
// Returns:     pName - buffer to write the name string into.
//------------------------------------------------------------------------------

void radTimeGetUnitName( radTimeUnit timeUnit, char* pName )
{
    rAssert( pName != NULL );

    switch( timeUnit )
    {
        case radTimeUnit_Nanosecond:
        {
            strcpy( pName, "nS" );
            break;
        }
        case radTimeUnit_Microsecond:
        {
            strcpy( pName, "uS" );
            break;
        }
        case radTimeUnit_Millisecond:
        {
            strcpy( pName, "mS" );
            break;
        }
        case radTimeUnit_Second:
        {
            strcpy( pName, "s" );
            break;
        }
        case radTimeUnit_Minute:
        {
            strcpy( pName, "m" );
            break;
        }
        case radTimeUnit_Hour:
        {
            strcpy( pName, "h" );
            break;
        }
        case radTimeUnit_Day:
        {
            strcpy( pName, "d" );
            break;
        }
        default:
        {
            strcpy( pName, "" );
            break;
        }
    }
}


//=============================================================================
// Function:    radStopwatch::radStopwatch
//=============================================================================
// Description: Constructor for the stopwatch implementation class.
//              Initializes the watch to the reset state and sets the 
//              reference count to 0 (the factory should set it to 1).
//
// Parameters:  None.
//              
// Returns:     None.
//------------------------------------------------------------------------------

radStopwatch::radStopwatch( void ) 
    :
    m_ReferenceCount( 0 ),
    m_TotalTime( 0 ),
    m_StartTime( 0 ),
    m_StopTime( 0 ),
    m_TimeUnit( radTimeUnit_Second ),
    m_Running( false )
{
    radMemoryMonitorIdentifyAllocation( this, g_nameFTech, "radStopwatch" );
}


//=============================================================================
// Function:    radStopwatch::~radStopwatch
//=============================================================================
// Description: Destructor for the stopwatch implementation class.
//              It would free internal allocated objects if there were any.
//
// Parameters:  None.
//              
// Returns:     None.
//------------------------------------------------------------------------------

radStopwatch::~radStopwatch( void ) 
{
}


//=============================================================================
// Function:    radStopwatch::AddRef
//=============================================================================
// Description: Increment reference count of this object.
//
// Parameters:  None.
//              
// Returns:     None.
//------------------------------------------------------------------------------

void radStopwatch::AddRef( void )
{
    m_ReferenceCount++;
}


//=============================================================================
// Function:    radStopwatch::Release
//=============================================================================
// Description: Decrement reference count of this object, and destroy it if
//              no references remain.
//
// Parameters:  None.
//              
// Returns:     None.
//------------------------------------------------------------------------------

void radStopwatch::Release( void )
{
    m_ReferenceCount--;
    if( m_ReferenceCount < 1 )
    {
        delete this;
    }
}


//=============================================================================
// Function:    radStopwatch::Start
//=============================================================================
// Description: Start measuring passing time.
//              This function records the current time as the stopwatch's
//              starting time.
//
// Parameters:  None.
//              
// Returns:     None.
//
// Notes:       If this function is called while the watch is running, it first
//              calls Stop() to record the elapsed time so far, then restarts.
//
//------------------------------------------------------------------------------

void radStopwatch::Start( void ) 
{ 
    if( m_Running )
    {
        Stop( );
    }
    m_StartTime = GetTime( );
    m_Running = true; 
}


//=============================================================================
// Function:    radStopwatch::Stop
//=============================================================================
// Description: Stop measuring passing time and add the time elapsed since
//              Start() was called to the internal total.
//
//              This function records the current time as the stopwatch's
//              stop time and adds the difference between its start and stop
//              times to the running total elapsed time.
//
// Parameters:  None.
//              
// Returns:     None.
//
// Notes:       Has no effect if the watch is not running.
//
//------------------------------------------------------------------------------

void radStopwatch::Stop( void ) 
{ 
    if( m_Running )
    {
        m_StopTime = GetTime( );
        m_Running = false;
    }
    else
    {
        m_StopTime = m_StartTime;
    }
    m_TotalTime += m_StopTime - m_StartTime;
}


//=============================================================================
// Function:    radStopwatch::GetElapsedTime
//=============================================================================
// Description: Return the total elapsed time in the watch's native units.
//
// Parameters:  None.
//              
// Returns:     Returns total recorded time. This is the total
//              time that passed between each call to Start() and the following
//              call to Stop(). There can be many such pairs.
//
// Notes:       This call can be made while the watch is running, in which case
//              it does not stop the watch or update the total elapsed time,
//              but returns what the elapsed time so far is.
//
//              GetTimeUnits() can be called to find out what units the return
//              value is in.
//
//------------------------------------------------------------------------------

unsigned int radStopwatch::GetElapsedTime ( void ) const
{ 
    if( m_Running ) 
    {
        return( GetTime( ) - m_StartTime + m_TotalTime );
    }
    else
    {
        return( m_TotalTime );
    }
}


//=============================================================================
// Function:    radStopwatch::GetElapsedTime
//=============================================================================
// Description: Return the total elapsed time in user-specified units.
//
// Parameters:  timeUnit - unit to convert the return value to.
//              
// Returns:     Returns total recorded time in any unit. This is the total
//              time that passed between each call to Start() and the following
//              call to Stop(). There can be many such pairs.
//
// Notes:       This call can be made while the watch is running, in which case
//              it does not stop the watch or update the total elapsed time,
//              but returns what the elapsed time so far is.
//
//              It is important to pay attention to overflow/underflow 
//              issues when converting time units. For example, converting 
//              a day to nanoseconds will overflow an integer many times over
//              while converting thousands of microseconds to seconds will
//              produce a result of zero.
//
//------------------------------------------------------------------------------

unsigned int radStopwatch::GetElapsedTime ( radTimeUnit timeUnit ) const
{ 
    unsigned int ret = 0;
    if( m_Running ) 
    {
        ret = GetTime( ) - m_StartTime + m_TotalTime;
    }
    else
    {
        ret = m_TotalTime;
    }

    unsigned int num, denom;
    GetConversionFactor( m_TimeUnit, timeUnit, &num, &denom );

    rWarningMsg( ret * num >= ret, "WARNING: radStopwath overflow GetElapsedTime\n" );

    ret = ( ret * num ) / denom;
    return( ret );
}


//=============================================================================
// Function:    radStopwatch::IsRunning
//=============================================================================
// Description: Query the stopwatch to see if it is currently recording time.
//
// Parameters:  None.
//              
// Returns:     True if Stop() or Reset() have not been called since the last
//              call to Start().
//
//------------------------------------------------------------------------------

bool radStopwatch::IsRunning( void ) const
{
    return( m_Running );
}


//=============================================================================
// Function:    radStopwatch::Reset
//=============================================================================
// Description: Reset the stopwatch to the zeroed, stopped state.
//              This stops the watch and sets its total accumulated time to zero.
//
// Parameters:  None.
//              
// Returns:     None.
//------------------------------------------------------------------------------

void radStopwatch::Reset( void )
{
    m_TotalTime = 0;
    m_Running = false;
}


//=============================================================================
// Function:    radStopwatch::SetTimeUnits
//=============================================================================
// Description: Change the time units used by the stopwatch. 
//
// Parameters:  timeUnit - the new base measurement unit to use.
//              
// Returns:     None.
//
// Notes:       Cannot be called when the watch is running.
//------------------------------------------------------------------------------

void radStopwatch::SetTimeUnits( radTimeUnit timeUnit )
{
    rAssert( !m_Running );

    unsigned int num, denom;
    GetConversionFactor( m_TimeUnit, timeUnit, &num, &denom );

    rWarningMsg( m_TotalTime * num >= m_TotalTime, "WARNING: radStopwath overflow SetTimeUnits\n" );

    m_TotalTime = ( m_TotalTime * num ) / denom;
    m_TimeUnit = timeUnit;
}


//=============================================================================
// Function:    radStopwatch::GetTimeUnits
//=============================================================================
// Description: Find the time unit used by this stopwatch,
//
// Parameters:  None.
//              
// Returns:     The time unit size this watch is currently using.
//------------------------------------------------------------------------------

radTimeUnit radStopwatch::GetTimeUnits( void ) const
{
    return( m_TimeUnit );
}


//=============================================================================
// Function:    radStopwatch::GetConversionFactor
//=============================================================================
// Description: Compute the conversion factor between two time units.
//
// Parameters:  fromUnit - the unit you want to convert from.
//              toUnit - the unit you want to convert to.
//              
// Returns:     pNumerator - the numerator of the conversion factor. 
//              pDenominator - the denominator of the conversion factor.
//
// Notes:       To perform the conversion, multiply your time quantity by
//              the numerator value then divide by the denominator.
//------------------------------------------------------------------------------

void radStopwatch::GetConversionFactor
( 
    radTimeUnit fromUnit, 
    radTimeUnit toUnit,                       
    unsigned int* pNumerator, 
    unsigned int* pDenominator
)
{
    rAssert( pNumerator != NULL );
    rAssert( pDenominator != NULL );

    radTimeUnit index[ 7 ] = {
        radTimeUnit_Nanosecond,
        radTimeUnit_Microsecond,
        radTimeUnit_Millisecond,
        radTimeUnit_Second,
        radTimeUnit_Minute,
        radTimeUnit_Hour,
        radTimeUnit_Day
    };

    unsigned int factors[ 7 ] = { 1, 1000, 1000, 1000, 60, 60, 24 };

    unsigned int num = 1;
    unsigned int denom = 1;
    int fromIndex = 0, toIndex = 0;

    //
    // Convert the enums to indeces.
    //
    for( int i = 0; i < 7; i++ )
    {
        if( index[i] == fromUnit )
        {
            fromIndex = i;
        }
        if( index[i] == toUnit )
        {
            toIndex = i;
        }
    }

    //
    // Iteratively bring the two indeces closer together.
    //
    while( fromIndex < toIndex )
    {
        fromIndex++;
        denom *= factors[ fromIndex ];
    }

    while( fromIndex > toIndex )
    {
        num *= factors[ fromIndex ];
        fromIndex--;
    }

    //
    // Return the answers.
    //
    *pNumerator = num;
    *pDenominator = denom;
}


//=============================================================================
// Function:    radStopwatch::GetTime
//=============================================================================
// Description: This private member get the current time in the appropriate 
//              units.
//
// Parameters:  None.
//              
// Returns:     The current time in the watch's native units.
//------------------------------------------------------------------------------

unsigned int radStopwatch::GetTime( void ) const
{
    switch( m_TimeUnit )
    {
        case radTimeUnit_Nanosecond:
        {
            unsigned int nowMicro = radTimeGetMicroseconds( );
            rWarningMsg( nowMicro < 4294968, "WARNING: radStopwatch overflowing nanoseconds\n" );
            return( 1000 * nowMicro );
            break;
        }
        case radTimeUnit_Microsecond:
        {
            return( radTimeGetMicroseconds( ) );
            break;
        }
        case radTimeUnit_Millisecond:
        {
            return( radTimeGetMilliseconds( ) );
            break;
        }
        case radTimeUnit_Second:
        {
            return( radTimeGetSeconds( ) );
            break;
        }
        case radTimeUnit_Minute:
        {
            return( radTimeGetSeconds( ) / 60 );
            break;
        }
        case radTimeUnit_Hour:
        {
            return( radTimeGetSeconds( ) / 3600 );
            break;
        }
        case radTimeUnit_Day:
        {
            return( radTimeGetSeconds( ) / ( 24 * 3600 ) );
            break;
        }
        default:
        {
            return( radTimeGetSeconds( ) );
            break;
        }
    }
}


//=============================================================================
// Function:    radStopwatch::Dump
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

void radStopwatch::Dump( char * pStringBuffer, unsigned int bufferSize )
{
    sprintf( pStringBuffer, "Object: [radStopwatch] At Memory Location:[%p]\n", this );
}

#endif
