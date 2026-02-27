//===========================================================================
// Copyright (C) 2001 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   Radical performance statistics collection service.
//
// Description: This file implements a "stopwatch" convenience object for
//              recording timings.
//
// Authors:     Soleil Lapierre
//
//===========================================================================

// Recompilation protection flag.
#ifndef __STOPWATCH_H
#define __STOPWATCH_H

//===========================================================================
// Nested Includes
//===========================================================================

#include <radmemory.hpp>
#include <raddebug.hpp>
#include <radtime.hpp>


//===========================================================================
// Class Definitions
//===========================================================================

//===========================================================================
//
// Description: This class implements a simple stopwatch object, which is
//              a convenient container for recording timing data.
//
//===========================================================================
class radStopwatch : public IRadStopwatch,
                     public radObject
{
    public:

    //
    // Constructor.
    //
    radStopwatch( void );

    //
    // IRefCount stuff
    //
    virtual void AddRef( void );
    virtual void Release( void );

    //
    // IRadStopwatch stuff.
    //
    virtual void SetTimeUnits( radTimeUnit timeUnit );
    virtual radTimeUnit GetTimeUnits( void ) const;
    virtual void Start( void );
    virtual void Stop( void );
    virtual unsigned int GetElapsedTime ( void ) const;
    virtual unsigned int GetElapsedTime ( radTimeUnit timeUnit ) const;
    virtual bool IsRunning( void ) const;
    virtual void Reset( void );

    #ifdef RAD_DEBUG
    virtual void Dump( char * pStringBuffer, unsigned int bufferSize );
    #endif

    private:

    //
    // Destructor.
    //
    ~radStopwatch( void );

    //
    // Figure out the conversion factor between two time units.
    //
    static void GetConversionFactor( radTimeUnit fromUnit, radTimeUnit toUnit,
                                     unsigned int* pNumerator, unsigned int* pDenominator );

    //
    // Get the current time in the appropriate units.
    //
    unsigned int GetTime( void ) const;

    //
    // Reference counting member variable.
    //
    int m_ReferenceCount;

    //
    // Member variables for recording the watch state.
    //
    unsigned int  m_TotalTime;
    unsigned int  m_StartTime;
    unsigned int  m_StopTime;
    radTimeUnit   m_TimeUnit;
    bool          m_Running;
};

#endif // file