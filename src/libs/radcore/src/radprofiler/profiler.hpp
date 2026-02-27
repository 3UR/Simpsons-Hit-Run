//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        profiler.hpp
//
// Subsystem:   Foundation Technologies - radProfiler
//
// Description: This file contain all interfaces used for rad profiler.
//              It is based on Steve Rabin's Real-Time In-Game Profiling
//              in "Game Programming Gems" - Mark Deloura
//
// Notes:       
//
// Revisions:   July 10, 2002        Creation
//
//=============================================================================

#ifndef PROFILER_HPP
#define PROFILER_HPP

//---------------------------------------------------------------------------
// Includes
//===========================================================================
#include <radprofiler.hpp>
#include <radkey.hpp>
#include <radtime.hpp>

//
// if RADPROFILER is not defined, don't even compile this code
//
#ifdef RADPROFILER

class radProfileSample;

class radProfiler :
    public radRefCount,
    public IRadProfiler
{
public:
    IMPLEMENT_REFCOUNTED( "radProfiler" )

    radProfiler( radMemoryAllocator alloc );

    virtual ~radProfiler( );

    virtual void Initialize( unsigned int InitReserveProfileSamplesMemory, unsigned int MaxProfileSamples, radMemoryAllocator alloc );

    virtual void Terminate( );

    virtual void BeginProfile( const char * pProfileName );

    virtual void EndProfile( const char * pProfileName );

    virtual void BeginFrame( );

    virtual void EndFrame( );

    virtual IRadProfileSample * GetRootProfileSample( );

    virtual IRadProfileSample * GetProfileSample( const char * pName );

#ifdef RAD_DEBUG

    virtual void DebugDump( );

#endif // RAD_DEBUG

protected:
    //
    // Event handler
    //
    virtual void OnInsertNewProfileSample( radProfileSample * pProfile );

    virtual void OnInitializeFrameCounter( float * fAveFrameTime, float * fMaxFrameTime, float * fMinFrameTime, float * fFramePerSec );

protected:

    radProfileSample * GetLastUnClosedProfile( );

    radProfileSample * FindProfileSample( const char * pProfileName );

    radProfileSample * InsertChildProfileSample( const char * pProfileName, radProfileSample * pParentProfileSample );

    void EndFrameForAllProfileNode( radTime64 uTotalFrameTimeInMicroSec, radTime64 uAveFrameTimeInMicroSec, radTime64 uThisFrameTimeInMicroSec, unsigned int uFrameCount );

private:

    void SortSampleProfileArrayByHashedName( );

    //
    // return the profile sample from sorted list by index,
    // see m_pProfileSampleArrayByHashedName and m_uProfileSampleArraySize.
    // if return -1 then no entry is found.
    //
    int BinarySearchSampleProfileArrayByHashedName( radKey HashedName );

    void AllocateMemoryForProfileSampleArrayAndCopyOldData( unsigned int uSize );

private:

    //
    // allocator
    //
    radMemoryAllocator          m_alloc;

    radRef< IRadMemoryPool >       m_pProfileSamplesPool;

    //
    // hierarchy tree for all profile samples
    //
    radRef< radProfileSample >     m_pRootProfileSample;

    //
    // last unclosed profile sample
    //
    radRef< radProfileSample >     m_pLastUnClosedProfile;

    //
    // last closed profile sample
    //
    radRef< radProfileSample >     m_pLastClosedProfile;

    //
    // sorted profile sample list by name
    //
    struct ProfileSamplePair
    {
        radKey                  HashedName;
        radProfileSample *      pSample;
    };

    ProfileSamplePair *         m_pProfileSampleArrayByHashedName;
    unsigned int                m_uProfileSampleArraySize;
    unsigned int                m_uProfileSampleMemorySize;

    //
    // Get Current time, and record it
    //
    radTime64                   m_uFrameStartTime;
    unsigned int                m_uFrameCount;
    radTime64                   m_uTotalFrameTime;

    float                       m_fAveFrameTime;
    float                       m_fMaxFrameTime;
    float                       m_fMinFrameTime;
    float                       m_fFramePerSec;
};

class radWatcherEnabledProfiler :
    public radProfiler
{
public:
    radWatcherEnabledProfiler( radMemoryAllocator alloc ):
        radProfiler( alloc )
    { }

    virtual ~radWatcherEnabledProfiler( ) { };

protected:

    //
    // Event handler
    //
    virtual void OnInsertNewProfileSample( radProfileSample * pProfile );

    virtual void OnInitializeFrameCounter( float * fAveFrameTime, float * fMaxFrameTime, float * fMinFrameTime, float * fFramePerSec );

    void BuildNameSpace( radProfileSample * pProfile, char * szNameSpace );

private:

};


extern radProfiler * g_pTheProfiler;

#endif // RADPROFILER

#endif // PROFILER_HPP
