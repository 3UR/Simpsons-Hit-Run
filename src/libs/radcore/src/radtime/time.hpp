//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        time.hpp
//
// Subsystem:	Foundation Technologies - Time Subsystem
//
// Description:	This file contains all definitions and classes required to
//              implement the the Timer. The timer provides a means
//              of generating events based on timer elapses.
//
// Revisions:  	Mar 12, 2001 Creation
//
//=============================================================================

#ifndef	TIME_HPP
#define TIME_HPP

//=============================================================================
// Include Files
//=============================================================================

#include <radtime.hpp>
#include <radobject.hpp>
#include <radmemory.hpp>

//=============================================================================
// Forward Class Declarations
//=============================================================================

class radTimer;

//=============================================================================
// Defintions
//=============================================================================

//=============================================================================
// Class Declarations
//=============================================================================

//
// This is the timerlist list object.
//
class radTimerList : public IRadTimerList,
                     public radObject
{
    public:
 
    //
    // Construtor and destructor. Nothing too interesting.
    //
    radTimerList( unsigned int numberOfTimers, radMemoryAllocator allocator );
    virtual ~radTimerList( void );

    //  
    // Members associated with IRadTimerList
    //
    virtual void CreateTimer( IRadTimer**           ppTimer,
                              unsigned int          timeout,		
                              IRadTimerCallback*    callback, 
                              void*                 pUserData,
                              bool                  start,                
                              IRadTimer::ResetMode  resetMode );

    virtual void Service( void );
    virtual void Service( unsigned int currentTime );

    //
    // Members used to manage the life of the timer manager.
    //
    void AddRef( void );
    void Release( void );
    
    //
    // Used to report constructed objects.
    //
    #ifdef RAD_DEBUG
    virtual void Dump( char* pStringBuffer, unsigned int bufferSize );
    #endif

    //
    // There two members are provide for allocation and freeing memory used
    // for timer objects.
    //
    void* AllocateTimerMemory( void );
    void  FreeTimerMemory( void* p );

    private:

    friend class radTimer;

    //
    // This is the reference count of the timer manager.
    //
    unsigned int m_ReferenceCount;

    //
    // This is used to indicated if we have been invoked. Used to establish start
    // time.
    //
    bool          m_Started;

    //
    // Maintains the last time our handler was invoked.
    //  
    unsigned int  m_PreviousTime;

    //
    // This holds the interface to the pool we used for timer allocations.
    //
    IRadMemoryPool* m_TimerPool;

    unsigned int    m_NumberOfTimers;

    //
    // This array holds a weak reference to every started timer.
    //
    radTimer**      m_ActiveTimers;
};

//
// This is the timer object implementation. 
//
class radTimer : public IRadTimer
{
    public:

    //
    // Members as a result of IRadTimer
    //
    virtual void RegisterCallback( IRadTimerCallback* handler, void* pUserData );
    virtual void UnregisterCallback( IRadTimerCallback* handler );
    virtual unsigned int GetTimeout( void );
    virtual void SetTimeout( unsigned int timeout );
    virtual ResetMode GetResetMode( void );
    virtual void SetResetMode( ResetMode resetMode );
    virtual void Start( void );
    virtual void Stop( void );
    virtual bool HasTimerExpired( void );

    //
    // Members used to manage the lifetime of the timer object.
    // 
    void AddRef( void );
    void Release( void );
    
    //
    // Create using the time manager object.
    //
    radTimer( radTimerList* pTimerList, unsigned int timeout, IRadTimerCallback* handler,
              void* pUserData, bool start, IRadTimer::ResetMode resetMode);

    virtual ~radTimer( void );    // Virtual only to prevent warnings

    //
    // Need operator new to allocate timers from the pool. The manager is
    // needed so we know what pool to allcate memory for timer object
    //
    void* operator new(size_t size, radTimerList* pTimerList ); 
    
    private:
    
    friend class radTimerList;
 
    //
    // Manages the lifetime of the timer object.
    // 
    unsigned int        m_ReferenceCount;

    //
    // These maintain state information about the timer.
    //
    IRadTimer::ResetMode  m_ResetMode;        
    unsigned int          m_TimePeriod;
    bool                  m_IsStarted;
    
    //
    // This maintains the time accumulated for the specific timer. 
    //
    unsigned int        m_TimeAccumulation;

    //
    // This timers callback handler.
    //
    IRadTimerCallback*  m_Handler;

    void*               m_UserData;

    //
    // Pointer to parent timer list
    //
    radTimerList*       m_pRadTimerList;
    
};

#endif