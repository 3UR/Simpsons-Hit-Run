//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        profilesample.hpp
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

#ifndef PROFILERSAMPLE_HPP
#define PROFILERSAMPLE_HPP

//---------------------------------------------------------------------------
// Includes
//===========================================================================
#include <radprofiler.hpp>
#include <radobject.hpp>
#include <radtime.hpp>

class radProfileSample;
class radWatcherEnabledProfiler;

class radProfileSample :
    public IRadProfileSample,
    public radRefCount
{
public:

    friend class radWatcherEnabledProfiler;

    IMPLEMENT_REFCOUNTED( "radProfileSample" )

    radProfileSample( radMemoryAllocator alloc );

    virtual ~radProfileSample( );

    static void SetProfileSampleMemoryAllocator( IRadMemoryAllocator * pAllocator );

    inline void* operator new( size_t size, void* p );
    inline void* operator new( size_t size, radMemoryAllocator allocator );
    inline void* operator new[]( size_t size, void* p );	
    inline void* operator new[]( size_t size, radMemoryAllocator allocator );
    inline void operator delete( void * pMemory );
    inline void operator delete[]( void * pMemory );

    void Initialize( const char * szName, radProfileSample * pParent, radProfileSample * pPrevSibling );

    virtual void AddChildNode( radProfileSample * pChildNode, radProfileSample * pPrevSibling );

    virtual bool IsChildNode( radProfileSample * pChildNode );

    //
    // IRadProfileSample
    //
    virtual IRadProfileSample * GetParentNode( );

    virtual const char * GetName( );

    virtual IRadProfileSample * GetFirstChildNode( );

    virtual IRadProfileSample * GetNextSilbingNode( );

    virtual IRadProfileSample * GetPrevSiblingNode( );

    virtual void GetSampleResult( float * pAveTimeInMSec, float * pMinTimeInMSec, float * pMaxTimeInMSec, unsigned int * pExecutionCount );

    virtual void GetSampleResultPerFrame( float * pAveTimePerFrameInPercent, float * pMinTimePerFrameInPercent, float * pMaxTimePerFrameInPercent, unsigned int * pExecutionCountPerFrame );

    //
    // measurements start
    //
    virtual void BegineMeasure( );

    virtual void EndMeasure( radTime64 * uTotalTimeInMicroSec = NULL, radTime64 * uAveTimeInMicroSec = NULL, radTime64 * uThisTimeInMicroSec = NULL, unsigned int * uExecutionCount = NULL );

    virtual void StartFrame( );

    virtual void EndFrame( radTime64 uTotalFrameTimeInMicroSec, radTime64 uAveFrameTimeInMicroSec, radTime64 uThisFrameTimeInMicroSec, unsigned int uFrameCount );

#ifdef RAD_DEBUG

    virtual void DebugDump( );

    virtual void DebugDumpNode( radProfileSample * pNode, unsigned int uLevel );

#endif // RAD_DEBUG

protected:

private:

    //
    // allocator
    //
    radMemoryAllocator  m_alloc;


    //
    // allocator where all samples will be stored in.
    //
    static IRadMemoryAllocator * s_pProfileSampleAllocator;


    //
    // raw data
    //
    char                m_szName[ 256 ];

    //
    // all time is in microsec
    //
    radTime64           m_uStartTime;               // The Current open profile start time

    radTime64           m_uTotalTime;               // Total amount of time for this sample

    radTime64           m_uTotalTimeAllFrame;

    radTime64           m_uTotalTimeThisFrame;

    unsigned int        m_uSampleCount;              // Number of time sample been taken

    unsigned int        m_uSampleCountAllFrame;

    unsigned int        m_uSampleCountThisFrame;

    //
    // calculated each time.
    //
    float               m_fAveTimeInMSec;

    float               m_fMinTimeInMSec;

    float               m_fMaxTimeInMSec;

    float               m_fAveTimePerFrameInPercent;

    float               m_fMinTimePerFrameInPercent;

    float               m_fMaxTimePerFrameInPercent;

    unsigned int        m_uExecutionCountPerFrame;

    //
    // tree list
    //
    radProfileSample *  m_pParentProfileSample;

    radRef< radProfileSample > m_pFirstChildProfileSample;

    radRef< radProfileSample >  m_pNextSiblingProfileSample;

    radProfileSample *  m_pPrevSiblingProfileSample;

};

radRef< radProfileSample > radProfileSampleCreate( radMemoryAllocator alloc = RADMEMORY_ALLOC_DEFAULT );

inline void* radProfileSample::operator new( size_t size, void* p )
{
    (void) size;
    return( p );
}    	

inline void* radProfileSample::operator new[ ]( size_t size, void* p )
{
    (void) size;
    return( p );
}    	

inline void* radProfileSample::operator new( size_t size, radMemoryAllocator allocator )
{
    if ( s_pProfileSampleAllocator != NULL )
    {
        //
        // if we define the allocator where memory will come from then
        // use that allocator
        //
        return s_pProfileSampleAllocator->GetMemory( size );
    }
    else
    {
        //
        // otherwise use the default memory allocator, and send warning
        //
        rTuneString( "radProfiler: Warning: Performance warning, specialized memory allocator not found.\n" );
        return radObject::operator new ( size, allocator );
    }
}

inline void* radProfileSample::operator new[ ]( size_t size, radMemoryAllocator allocator )
{
    if ( s_pProfileSampleAllocator != NULL )
    {
        //
        // if we define the allocator where memory will come from then
        // use that allocator
        //
        return s_pProfileSampleAllocator->GetMemory( size );
    }
    else
    {
        //
        // otherwise use the default memory allocator, and send warning
        //
        rTuneString( "radProfiler: Warning: Performance warning, specialized memory allocator not found.\n" );
        return radObject::operator new ( size, allocator );
    }
}

inline void radProfileSample::operator delete( void * pMemory )
{
    if ( s_pProfileSampleAllocator != NULL )
    {
        if( s_pProfileSampleAllocator->CanFreeMemory( pMemory ) )
        {
            s_pProfileSampleAllocator->FreeMemory( pMemory );
            return;
        }
    }
    radObject::operator delete ( pMemory );
}

inline void radProfileSample::operator delete[ ]( void * pMemory )
{
    if ( s_pProfileSampleAllocator != NULL )
    {
        if( s_pProfileSampleAllocator->CanFreeMemory( pMemory ) )
        {
            s_pProfileSampleAllocator->FreeMemory( pMemory );
            return;
        }
    }
    radObject::operator delete ( pMemory );
}

#endif // PROFILERSAMPLE_HPP
