//===========================================================================
// Copyright (C) 2002 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   profiler
//
// Description: This file contains the implementation of the 
//              profiler.
//
// Authors:     James Tan       July 29th
//
//===========================================================================

//---------------------------------------------------------------------------
// Includes
//===========================================================================
#include "pch.hpp"
#include "profiler.hpp"
#include "profilesample.hpp"
#include <radtime.hpp>
#include <string.h>

#ifdef RADPROFILER
radProfiler * g_pTheProfiler = NULL;

//===========================================================================
// radProfilerInitialize
//===========================================================================
// Description: Initialize the profiler system
//
// Constraints: Not yet
//
// Parameters:  
//
// Return:      none
//
//===========================================================================
void radProfilerInitialize( unsigned int InitReserveProfileSamplesMemory, unsigned int MaxProfileSamples, bool UseWatcher, radMemoryAllocator alloc )
{
    //
    // Assert that this subsystem has not allready been initialized.
    //
    rAssertMsg( g_pTheProfiler == NULL, "radProfiler: Error: radProfiler already initialized");

    //
    // Construct the object.
    //
    radProfiler * pProfiler = NULL;
    
#ifdef DEBUGWATCH
    if ( UseWatcher )
    {
        pProfiler = new( alloc ) radWatcherEnabledProfiler( alloc );
    }
    else
#endif // DEBUGWATCH
    {
        pProfiler = new( alloc ) radProfiler( alloc );
    }
    pProfiler->Initialize( InitReserveProfileSamplesMemory, MaxProfileSamples, alloc );
    g_pTheProfiler = pProfiler;
}

//===========================================================================
// radProfilerTerminate
//===========================================================================
// Description: Terminate the profiler system
//
// Constraints: Not yet
//
// Parameters:  None
//
// Return:      none
//
//===========================================================================
void radProfilerTerminate( )
{
    rAssertMsg( g_pTheProfiler != NULL, "radProfiler: Error: radProfiler not initialized");

    radProfiler * pOldPtr = g_pTheProfiler;
    g_pTheProfiler = NULL;
    pOldPtr->Terminate( );
    radRelease( static_cast< IRadProfiler* >( pOldPtr ), NULL );
}

//===========================================================================
// radProfilerGet
//===========================================================================
// Description: get the profiler pointer (singlton)
//
// Constraints: Not yet
//
// Parameters:  None
//
// Return:      profiler
//
//===========================================================================
IRadProfiler * radProfilerGet( )
{
    return g_pTheProfiler;
}

//===========================================================================
// radProfiler::radProfiler
//===========================================================================
// Description: cstor
//
// Constraints: Not yet
//
// Parameters:  alloc - allocator for memory.
//
// Return:      None
//
//===========================================================================
radProfiler::radProfiler( radMemoryAllocator alloc ) :
    m_alloc( alloc ),
    m_pProfileSampleArrayByHashedName( NULL ),
    m_uProfileSampleArraySize( 0 ),
    m_uProfileSampleMemorySize( 0 ),
    m_uFrameStartTime( 0 ),
    m_uFrameCount( 0 ),
    m_uTotalFrameTime( 0 ),
    m_fAveFrameTime( 0.0f ),
    m_fMaxFrameTime( 0.0f ),
    m_fMinFrameTime( 9999999.0f ),
    m_fFramePerSec( 0 )
{
    rAssertMsg( g_pTheProfiler == NULL, "radProfiler: Error: Cannot create second instance of singleton object [g_pTheProfiler].\n" );
}

//===========================================================================
// radProfiler::radProfiler
//===========================================================================
// Description: dstor
//
// Constraints: Not yet
//
// Parameters:  None
//
// Return:      None
//
//===========================================================================
radProfiler::~radProfiler( )
{
    Terminate( );
}

//===========================================================================
// radProfiler::Initialize
//===========================================================================
// Description: Initialize the profiler
//
// Constraints: Not yet
//
// Parameters:  InitReserveProfileSamplesMemory - amount of memory should 
//              reserve for the system's profile sample (in sample count).
//              MaxProfileSamples - profile sample count
//              alloc - allocator for memory needed for profiler system
//
// Return:      None
//
//===========================================================================
void radProfiler::Initialize( unsigned int InitReserveProfileSamplesMemory, unsigned int MaxProfileSamples, radMemoryAllocator alloc )
{
    rAssertMsg( m_pProfileSamplesPool == NULL, "radProfiler: Error: Cannot re-initialize the system." );
    radMemoryCreatePool( & m_pProfileSamplesPool, sizeof( radProfileSample ), InitReserveProfileSamplesMemory, MaxProfileSamples, false, Moderate, alloc, "radProfiler::m_pProfileSamplesPool" );
    rAssert( m_pProfileSamplesPool != NULL );

    radProfileSample::SetProfileSampleMemoryAllocator( m_pProfileSamplesPool );

    m_pRootProfileSample = radProfileSampleCreate( m_alloc );
    m_pRootProfileSample->Initialize( "radProfiler", NULL, NULL );
    m_pLastUnClosedProfile = m_pRootProfileSample;

    OnInitializeFrameCounter( & m_fAveFrameTime, & m_fMaxFrameTime, & m_fMinFrameTime, & m_fFramePerSec );
}

//===========================================================================
// radProfiler::Terminate
//===========================================================================
// Description: Terminate the profiler
//
// Constraints: Not yet
//
// Parameters:  
//
// Return:      None
//
//===========================================================================
void radProfiler::Terminate( )
{
    m_pLastUnClosedProfile = NULL;
    m_pLastClosedProfile = NULL;

    m_pRootProfileSample = NULL;
    radMemoryFree( m_pProfileSampleArrayByHashedName );
    m_pProfileSampleArrayByHashedName = NULL;
    m_uProfileSampleArraySize = 0;
    m_uProfileSampleMemorySize = 0;

    m_pProfileSamplesPool = NULL;
}

//===========================================================================
// radProfiler::BeginProfile
//===========================================================================
// Description: begine the profile sample node
//
// Constraints: Not yet
//
// Parameters:  pProfileName - name of the sample node
//
// Return:      None
//
//===========================================================================
void radProfiler::BeginProfile( const char * pProfileName )
{
    radRef< radProfileSample > pSample = NULL;

    pSample = FindProfileSample( pProfileName );

    //
    // if profile point is not yet created, create one
    //
    if ( pSample == NULL )
    {
        pSample = InsertChildProfileSample( pProfileName, GetLastUnClosedProfile( ) );

        rAssertMsg( pSample != NULL, "radProfiler: Error: Cannot begin profile sample specified (cannot create).\n" );

        if ( pSample != NULL )
        {
            OnInsertNewProfileSample( pSample );
        }
    }

    //
    // Error checking, see if the profile point is already started
    // 1) just started profile point cannot be last unclosed profile point
    //    and higher parent of last unclosed profile points
    //
#ifdef RAD_DEBUG
    radProfileSample * pLastUnClosedProfile = GetLastUnClosedProfile( );

    while( pLastUnClosedProfile != NULL )
    {
        if ( strcmp( pProfileName, pLastUnClosedProfile->GetName( ) ) == 0 )
        {
            rDebugPrintf( "radProfiler: Error: Cannot BeginProfile( \"%s\"); profile point already began.\n", pProfileName );
            rAssert( false );
            return;
        }
        pLastUnClosedProfile = static_cast< radProfileSample * >( pLastUnClosedProfile->GetParentNode( ) );
    }
#endif // RAD_DEBUG

    //
    // begine the measurment of the sample
    //
    pSample->BegineMeasure( );
    m_pLastUnClosedProfile = pSample;
}

//===========================================================================
// radProfiler::EndProfile
//===========================================================================
// Description: end the profile sample node
//
// Constraints: Not yet
//
// Parameters:  pProfileName - name of the sample node
//
// Return:      None
//
//===========================================================================
void radProfiler::EndProfile( const char * pProfileName )
{
    radRef< radProfileSample > pSample = NULL;

    pSample = FindProfileSample( pProfileName );

    rAssertMsg( pSample != NULL, "radProfiler: Error: Cannot end profile sample specified (not found).\n" );

    //
    // profile point to be ended must be last unclosed profile point, we do not allow interleaved profile points
    // yet.
    //
    rAssertMsg( m_pLastUnClosedProfile == pSample, "radProfiler: Error: Profiler cannot end current profile point specified (disallowing interleaved profile points).\n" );

    pSample->EndMeasure( );
    m_pLastUnClosedProfile = static_cast< radProfileSample * >( pSample->GetParentNode( ) );
    m_pLastClosedProfile = pSample;
}

//===========================================================================
// radProfiler::BeginFrame
//===========================================================================
// Description: begine sample of a frame
//
// Constraints: Not yet
//
// Parameters:  None
//
// Return:      None
//
//===========================================================================
void radProfiler::BeginFrame( )
{
    //
    // Get Current time, and record it
    //
    m_uFrameStartTime = radTimeGetMicroseconds64( );
    m_uFrameCount ++;
}


//===========================================================================
// radProfiler::BeginFrame
//===========================================================================
// Description: end sample of a frame
//
// Constraints: Not yet
//
// Parameters:  None
//
// Return:      None
//
//===========================================================================
void radProfiler::EndFrame( )
{
    unsigned int uDeltaTime = (unsigned int)( radTimeGetMicroseconds64( ) - m_uFrameStartTime );

    m_uTotalFrameTime += uDeltaTime;
    m_fAveFrameTime = (float)( m_uTotalFrameTime / m_uFrameCount ) / 1000.0f;

    float fDeltaTime = uDeltaTime / 1000.0f;

    if ( m_fMaxFrameTime < fDeltaTime )
    {
        m_fMaxFrameTime = fDeltaTime;
    }

    if ( m_fMinFrameTime > fDeltaTime )
    {
        m_fMinFrameTime = fDeltaTime;
    }

    m_fFramePerSec = (float)m_uFrameCount / (float)( m_uTotalFrameTime / ( 1000 * 1000 ) );
    EndFrameForAllProfileNode( m_uTotalFrameTime, ( m_uTotalFrameTime / m_uFrameCount ), uDeltaTime, m_uFrameCount );
}

//===========================================================================
// radProfiler::GetRootProfileSample
//===========================================================================
// Description: return the root profile sample
//
// Constraints: Not yet
//
// Parameters:
//
// Return:     IRadProfileSample - root sample, contain all child nodes.
//
//===========================================================================
IRadProfileSample * radProfiler::GetRootProfileSample( )
{
    return m_pRootProfileSample;
}

//===========================================================================
// radProfiler::GetProfileSample
//===========================================================================
// Description: return the profile sample by the name
//
// Constraints: Not yet
//
// Parameters:
//
// Return:     IRadProfileSample - sample node.
//
//===========================================================================
IRadProfileSample * radProfiler::GetProfileSample( const char * pName )
{
    return FindProfileSample( pName );
}

//===========================================================================
// radProfiler::OnInsertNewProfileSample
//===========================================================================
// Description: Callback when inserted new profile sample into the tree
//
// Constraints: Not yet
//
// Parameters:  pProfile - profile sample just inserted
//
// Return:      None
//
//===========================================================================
void radProfiler::OnInsertNewProfileSample( radProfileSample * pProfile )
{
    (void)pProfile;
}

void radProfiler::OnInitializeFrameCounter( float * fAveFrameTime, float * fMaxFrameTime, float * fMinFrameTime, float * fFramePerSec )
{
    (void)fAveFrameTime;
    (void)fMaxFrameTime;
    (void)fMinFrameTime;
    (void)fFramePerSec;
}


#ifdef RAD_DEBUG

//===========================================================================
// radProfiler::DebugDump
//===========================================================================
// Description: dump stats
//
// Constraints: Not yet
//
// Parameters:  None
//
// Return:      None
//
//===========================================================================
void radProfiler::DebugDump( )
{
    m_pRootProfileSample->DebugDump( );
}

#endif // RAD_DEBUG


#ifdef DEBUGWATCH
#include <raddebugwatch.hpp>

//===========================================================================
// radWatcherEnabledProfiler::OnInsertNewProfileSample
//===========================================================================
// Description: Callback when a new profile sample has been inserted.
//
// Constraints: Not yet
//
// Parameters:  pProfile - pointer to the profile sample
//
// Return:      None
//
//===========================================================================
void radWatcherEnabledProfiler::OnInsertNewProfileSample( radProfileSample * pProfile )
{
    //
    // build a path from profile sample's parent tree
    //
    // \\radProfiler\\...
    //

    char szNameSpace[ 256 ] = "";

    BuildNameSpace( pProfile, szNameSpace );

    radDbgWatchAddFloat( & pProfile->m_fAveTimeInMSec, "AveTime(msec)", szNameSpace );
    radDbgWatchAddFloat( & pProfile->m_fMinTimeInMSec, "MinTime(msec)", szNameSpace );
    radDbgWatchAddFloat( & pProfile->m_fMaxTimeInMSec, "MaxTime(msec)", szNameSpace );
    radDbgWatchAddFloat( & pProfile->m_fAveTimePerFrameInPercent, "AveTime(% of Frame)", szNameSpace );
    radDbgWatchAddFloat( & pProfile->m_fMinTimePerFrameInPercent, "MinTime(% of Frame)", szNameSpace );
    radDbgWatchAddFloat( & pProfile->m_fMaxTimePerFrameInPercent, "MaxTime(% of Frame)", szNameSpace );
    radDbgWatchAddUnsignedInt( & pProfile->m_uSampleCountAllFrame, "TotalExecutionCount", szNameSpace );
}

void radWatcherEnabledProfiler::OnInitializeFrameCounter( float * fAveFrameTime, float * fMaxFrameTime, float * fMinFrameTime, float * fFramePerSec )
{
    radProfileSample * pSample = static_cast< radProfileSample * >( GetRootProfileSample( ) );
    char szNameSpace[ 256 ];
    sprintf( szNameSpace, "\\%s", pSample->GetName( ) );
    radDbgWatchAddFloat( fAveFrameTime, "AveFrameTime(msec)", szNameSpace );
    radDbgWatchAddFloat( fMaxFrameTime, "MaxFrameTime(msec)", szNameSpace );
    radDbgWatchAddFloat( fMinFrameTime, "MinFrameTime(msec)", szNameSpace );
    radDbgWatchAddFloat( fFramePerSec, "FramePerSec", szNameSpace );

}


//===========================================================================
// radWatcherEnabledProfiler::BuildNameSpace
//===========================================================================
// Description: recursivly build a namespace from parent tree
//
// Constraints: Not yet
//
// Parameters:  pProfile - pointer to the profile sample
//              szNameSpace - namespace string
//
// Return:      None
//
//===========================================================================
void radWatcherEnabledProfiler::BuildNameSpace( radProfileSample * pProfile, char * szNameSpace )
{
    if( pProfile != static_cast< radProfileSample * >( GetRootProfileSample( ) ) )
    {
        BuildNameSpace( static_cast< radProfileSample * >( pProfile->GetParentNode( ) ), szNameSpace );
    }

    strcat( szNameSpace, "\\" );
    strcat( szNameSpace, pProfile->GetName( ) );
}

#endif // DEBUGWATCH

#endif // RAD_PROFILE
