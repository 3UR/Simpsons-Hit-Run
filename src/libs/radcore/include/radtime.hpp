//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        radtime.hpp
//
// Subsystem:	Foundation Technologies - Time Subsystem
//
// Description:	This file contains all definitions and interfaces required to
//              interact with the timer subsystem. The timer provides a means
//              of generating events based on the elapse of time. Also provides
//              services to monitor the elapse of time. 
//
// Revisions:  	Mar 2, 2001 Creation
//
//=============================================================================

#ifndef	RADTIME_HPP
#define RADTIME_HPP

//=============================================================================
// Build Configuration Check
//=============================================================================

#if !defined(RAD_UWP) && !defined(RAD_WIN32)
    #error 'FTech requires definition of RAD_UWP, or RAD_WIN32'
#endif

//=============================================================================
// Include Files
//=============================================================================

#include <radobject.hpp>   
#include <radmemory.hpp>

//=============================================================================
// Forward Class/Struct Declarations
//=============================================================================

struct IRadTimerCallback;
struct IRadTimerList;
struct IRadStopwatch;

//=============================================================================
// Enumerations
//=============================================================================

//
// This enumeration is used to define the time units used by the IRadStopwatch.
//
enum radTimeUnit
{
    radTimeUnit_Nanosecond,         // Nanoseconds
    radTimeUnit_Microsecond,        // Microseconds
    radTimeUnit_Millisecond,        // Milliseconds
    radTimeUnit_Second,             // Seconds
    radTimeUnit_Minute,             // Minutes
    radTimeUnit_Hour,               // Hours
    radTimeUnit_Day                 // Days
};


//=============================================================================
// Functions
//=============================================================================

//
// These two services must frame access to all other time functions. They are
// counted so can be invoked more than one.
//
void radTimeInitialize( void );
void radTimeTerminate( void );

//
// These functions are used to obtain free running counters, returning values
// of different resolution.
//
unsigned int radTimeGetSeconds( void );
unsigned int radTimeGetMilliseconds( void );
unsigned int radTimeGetMicroseconds( void );

//
// define 64 bit integer radTime64 for all platform
//
#if defined (RAD_WIN32) || defined(RAD_UWP)
#include <cstdint>
typedef uint64_t radTime64;
#endif

radTime64 radTimeGetMicroseconds64( void );

//
// The following structure holds the current date.
//
struct radDate
{
    unsigned short  m_Year;     // absolute year A.D., i.e. 2002
    unsigned char   m_Month;    // month since January. [1 .. 12]
    unsigned char   m_Day;      // day of the month [1 .. 31]
    unsigned char   m_Hour;     // hour since midnight [0 .. 23]
    unsigned char   m_Minute;   // minutes after the hour [0 .. 59]
    unsigned char   m_Second;   // seconds after the minute [0 .. 59]
};

/// This enumeration represents the days of the week.
enum radWeekday
{
    Weekday_Monday = 0,
    Weekday_Tuesday = 1,
    Weekday_Wednesday = 2,
    Weekday_Thursday = 3,
    Weekday_Friday = 4,
    Weekday_Saturday = 5,
    Weekday_Sunday = 6
};

//
// This function will return the LOCAL time. If the hardware handles daylight savings time,
// that is the time which will be returned.
//
void radTimeGetDate( radDate* pDate );

/// This function returns the day of the week, given a date. This will not check that the day 
/// belongs in the month. So asking for February 30 will work.
radWeekday radTimeGetWeekday( unsigned short year, unsigned char month, unsigned char day );

//
// Use this member to create a timer list. Each timer list managers a series
// of timers. The game game create multiple timer list if it desires to partion
// things. Caller should provide the upper bound on the number of concurrent timers
// that can exist. 
//
void radTimeCreateList( IRadTimerList** ppIRadTimerList, 
                        unsigned int numberOfTimers = 16, 
                        radMemoryAllocator allocator = RADMEMORY_ALLOC_DEFAULT );

//
// Factory for creating a new stopwatch object.
//
void radTimeCreateStopwatch( IRadStopwatch** pNewWatch, radTimeUnit = radTimeUnit_Second,
                             radMemoryAllocator alloc = RADMEMORY_ALLOC_DEFAULT );

//
// Copy the name of a time unit into a string buffer.
// The names are nS, uS, mS, S, m, h and d.
//
void radTimeGetUnitName( radTimeUnit timeUnit, char* pName );



//=============================================================================
// Interfaces
//=============================================================================

//
// This is the timer interface. Objects implementing this interface can be 
// obtained for the timer list object.
//
struct IRadTimer : public IRefCount
{
    //
    // The reset enumeration governs how a timer behaves when it expires.
    // Periodic means it will restart after expiration, while one shot
    // means it will stop after expiration.
    //
    enum ResetMode
    {
        ResetModePeriodic,               // Timer will reset after expiration.
        ResetModeOneShot                 // Timer will only run once.
    };

    //
    // These members allow control of timer. The first is used to register and remove
    // callbacks. Callbacks are not reference counted interrfaces, so it
    // is the client's responsibility to ensure the lifetime of the callback
    // remains valid until the handler is unregistered or the timer is deleted,
    //
    virtual void RegisterCallback( IRadTimerCallback* handler, void* pUserData ) = 0;
    virtual void UnregisterCallback( IRadTimerCallback* handler ) = 0;

    ///  
    // Accessors used to get/set time out value.
    //
    virtual unsigned int GetTimeout( void ) = 0;
    virtual void SetTimeout( unsigned int timeout ) = 0;

    //
    // Accessors used to get/set reset mode.
    //    
    virtual ResetMode GetResetMode( void ) = 0;
    virtual void SetResetMode( ResetMode resetMode ) = 0;

    //
    // Members to start and stop timer.
    //
    virtual void Start( void ) = 0;
    virtual void Stop( void ) = 0;

    //  
    // This member can be used to determine if a timer has expired. Only
    // relavent for one-shot timers.
    //
    virtual bool HasTimerExpired( void ) = 0;
};

//
// Use this interface to allocate timers form. Must also be used to drive the
// execution of timers.
//
struct IRadTimerList : public IRefCount
{
    //
    // Use this member to create an actual timer object. Specify timeout
    // is units of milliseconds. Reset mode governs if it is a one shot timer or 
    // invoked periodically. 
    //
    virtual void CreateTimer( IRadTimer**           ppTimer,
                              unsigned int          timeout,		
                              IRadTimerCallback*    callback, 
                              void*                 pUserData = NULL,
                              bool                  start = true,                
                              IRadTimer::ResetMode  resetMode = IRadTimer::ResetModePeriodic ) = 0;

    //
    // These services are used to drive the processing of timers assoicated with this
    // list. Warning!!!. Timers will not execute unless this function is invoked.
    // Two methods are provide. The first uses system time as the time base. The seconds
    // allows the caller to create his own time base.
    //
    virtual void Service( void ) = 0;
    virtual void Service( unsigned int currentTime ) = 0;
};

//
// Implement this interface to handle the timer notification,
//
struct IRadTimerCallback
{
    virtual void OnTimerDone( unsigned int elapsedTime, void * pUserData ) = 0;      
};


//
// Interface for a simple passive stopwatch class. This can be used to gather
// processing time data. This class does not use a background process to do
// timing - it is merely a convenience to encapsulate code and members that
// you would otherwise have to write yourself to measure time periods.
//

struct IRadStopwatch : public IRefCount
{
    //
    // Reset the stopwatch to zero and stop it.
    //
    virtual void Reset( void ) = 0;

    //
    // Set/get the time units used by the watch.
    // The units cannot be changed while the watch is running.
    //
    virtual void SetTimeUnits( radTimeUnit ) = 0;
    virtual radTimeUnit GetTimeUnits( void ) const = 0;

    //
    // Start the watch.
    //
    virtual void Start( void ) = 0;

    //
    // Stop the watch; add the time since starting to the
    // total time since the last reset.
    //
    virtual void Stop( void ) = 0;

    //
    // Find out if the stopwatch is currently running.
    //
    virtual bool IsRunning( void ) const = 0;

    //
    // Get the total measured time in the watch's native units.
    //
    virtual unsigned int GetElapsedTime ( void ) const = 0;

    //
    // Get the total measured time in specified units.
    // This version allows you to specify a unit to convert to.
    // Be aware of overflow issues.
    //
    virtual unsigned int GetElapsedTime ( radTimeUnit timeUnit ) const = 0;
};



#endif