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
#include "profilesample.hpp"
#include <radtime.hpp>
#include <string.h>

#ifdef RADPROFILER

IRadMemoryAllocator * radProfileSample::s_pProfileSampleAllocator = NULL;

radRef< radProfileSample > radProfileSampleCreate( radMemoryAllocator alloc )
{
    return new( alloc ) radProfileSample( alloc );
}


void radProfileSample::SetProfileSampleMemoryAllocator( IRadMemoryAllocator * pAllocator )
{
    if( s_pProfileSampleAllocator == NULL )
    {
        s_pProfileSampleAllocator = pAllocator;
    }
    else
    {
        rAssertMsg( false, "radProfiler: Error: Cannot change memory allocator for profile samples.\n" );
    }
}

radProfileSample::radProfileSample( radMemoryAllocator alloc ) :
    radRefCount( 0 ),
    m_alloc( alloc ),
    m_uStartTime( 0 ),
    m_uTotalTime( 0 ),
    m_uTotalTimeAllFrame( 0 ),
    m_uTotalTimeThisFrame( 0 ),
    m_uSampleCount( 0 ),
    m_uSampleCountAllFrame( 0 ),
    m_uSampleCountThisFrame( 0 ),
    m_fAveTimeInMSec( 0.0f ),
    m_fMinTimeInMSec( 999999.0f ),
    m_fMaxTimeInMSec( 0.0f ),
    m_fAveTimePerFrameInPercent( 0.0f ),
    m_fMinTimePerFrameInPercent( 999999.0f ),
    m_fMaxTimePerFrameInPercent( 0.0f ),
    m_uExecutionCountPerFrame( 0 ),
    m_pParentProfileSample( NULL ),
    m_pFirstChildProfileSample( NULL ),
    m_pNextSiblingProfileSample( NULL ),
    m_pPrevSiblingProfileSample( NULL )
{
    m_szName[0] = '\0';
}

radProfileSample::~radProfileSample( )
{

}

void radProfileSample::Initialize( const char * szName, radProfileSample * pParent, radProfileSample * pPrevSibling )
{
    rAssert( szName != NULL && strlen( szName ) > 0 );

    strcpy( m_szName, szName );
    m_uStartTime                = 0;
    m_uTotalTime                = 0;
    m_uTotalTimeAllFrame        = 0;
    m_uTotalTimeThisFrame       = 0;
    m_uSampleCount              = 0;
    m_uSampleCountAllFrame      = 0;
    m_uSampleCountThisFrame     = 0;
    m_fAveTimeInMSec            = 0.0f;
    m_fMinTimeInMSec            = 999999.0f;
    m_fMaxTimeInMSec            = 0.0f;
    m_fAveTimePerFrameInPercent = 0.0f;
    m_fMinTimePerFrameInPercent = 999999.0f;
    m_fMaxTimePerFrameInPercent = 0.0f;
    m_uExecutionCountPerFrame   = 0;
    m_pParentProfileSample      = NULL;
    m_pFirstChildProfileSample  = NULL;
    m_pNextSiblingProfileSample = NULL;
    m_pPrevSiblingProfileSample = NULL;

    if ( pParent != NULL )
    {
        pParent->AddChildNode( this, pPrevSibling );
        this->m_pParentProfileSample = pParent;
    }
    else
    {
        //
        // if sample don't have parent, then there's no sibling as well.
        //
        rAssert( pParent == NULL && pPrevSibling == NULL );
    }
}

void radProfileSample::AddChildNode( radProfileSample * pChildNode, radProfileSample * pPrevSibling )
{
    //
    // search if pChildNode is already in
    //
    radProfileSample * pIterator = m_pFirstChildProfileSample;

    if ( m_pFirstChildProfileSample == NULL )
    {
        //
        // if current node has no child node yet, add pChildNode as the first,
        //

        // make sure parameter pPrevSibling is NULL.
        rAssert( pPrevSibling == NULL );
        m_pFirstChildProfileSample = pChildNode;
        m_pFirstChildProfileSample->m_pPrevSiblingProfileSample = NULL;
        m_pFirstChildProfileSample->m_pNextSiblingProfileSample = NULL;
    }
    else
    {
        if ( pPrevSibling == NULL )
        {
            pChildNode->m_pNextSiblingProfileSample = m_pFirstChildProfileSample;
            m_pFirstChildProfileSample = pChildNode;
        }
        else
        {
            // make sure pPrevSibling specified is infact a child node.
            rAssert( IsChildNode( pPrevSibling ) );

            //
            // insert pChildNode in between of the pPrevSibling and pPrevSibling->m_pNextSiblingProfileSample.
            //
            radProfileSample * pNextSibling = pPrevSibling->m_pNextSiblingProfileSample;

            pPrevSibling->m_pNextSiblingProfileSample = pChildNode;
            pChildNode->m_pNextSiblingProfileSample = pNextSibling;
            pChildNode->m_pParentProfileSample = this;
        }
    }
}

bool radProfileSample::IsChildNode( radProfileSample * pChildNode )
{
    radProfileSample * pIterator = m_pFirstChildProfileSample;

    //
    // go through entire list and check every node
    //
    while( pIterator != NULL )
    {
        if ( pIterator == pChildNode )
        {
            return true;
        }
        pIterator = pIterator->m_pNextSiblingProfileSample;
    }

    return false;
}

const char * radProfileSample::GetName( )
{
    return m_szName;
}

IRadProfileSample * radProfileSample::GetParentNode( )
{
    return m_pParentProfileSample;
}

IRadProfileSample * radProfileSample::GetFirstChildNode( )
{
    return m_pFirstChildProfileSample;
}

IRadProfileSample * radProfileSample::GetNextSilbingNode( )
{
    return m_pNextSiblingProfileSample;
}

IRadProfileSample * radProfileSample::GetPrevSiblingNode( )
{
    return m_pPrevSiblingProfileSample;
}

void radProfileSample::GetSampleResult( float * pAveTimeInMSec, float * pMinTimeInMSec, float * pMaxTimeInMSec, unsigned int * pExecutionCount )
{
    if ( pAveTimeInMSec != NULL )
    {
        *pAveTimeInMSec = 0.0f;
    }

    if ( pMinTimeInMSec != NULL )
    {
        *pMinTimeInMSec = 0.0f;
    }

    if ( pMaxTimeInMSec != NULL )
    {
        *pMaxTimeInMSec = 0.0f;
    }

    if ( pExecutionCount != NULL )
    {
        *pExecutionCount = 0;
    }
}

void radProfileSample::GetSampleResultPerFrame( float * pAveTimePerFrameInPercent, float * pMinTimePerFrameInPercent, float * pMaxTimePerFrameInPercent, unsigned int * pExecutionCountPerFrame )
{
    if ( pAveTimePerFrameInPercent != NULL )
    {
        *pAveTimePerFrameInPercent = 0.0f;
    }

    if ( pMinTimePerFrameInPercent != NULL )
    {
        *pMinTimePerFrameInPercent = 0.0f;
    }

    if ( pMaxTimePerFrameInPercent != NULL )
    {
        *pMaxTimePerFrameInPercent = 0.0f;
    }

    if ( pExecutionCountPerFrame != NULL )
    {
        *pExecutionCountPerFrame = 0;
    }
}

//
// measurements start
//
void radProfileSample::BegineMeasure( )
{
    rAssert( m_uStartTime == 0 );
    //
    // Get Current time, and record it
    //
    m_uStartTime = radTimeGetMicroseconds64( );

    m_uSampleCount ++;
    m_uSampleCountThisFrame ++;
    m_uSampleCountAllFrame ++;
}

void radProfileSample::EndMeasure( radTime64 * puTotalTimeInMicroSec, radTime64 * puAveTimeInMicroSec, radTime64 * puThisTimeInMicroSec, unsigned int * puExecutionCount )
{
    radTime64 uDeltaTime = radTimeGetMicroseconds64( ) - m_uStartTime;
    m_uTotalTime += uDeltaTime;
    m_uTotalTimeAllFrame += uDeltaTime;
    m_uTotalTimeThisFrame += uDeltaTime;
    m_uStartTime = 0;

    float fCurrentTimeInMSec = (float)uDeltaTime / 1000.0f;

    if ( m_fMinTimeInMSec > fCurrentTimeInMSec )
    {
        m_fMinTimeInMSec = fCurrentTimeInMSec;
    }

    if ( m_fMaxTimeInMSec < fCurrentTimeInMSec )
    {
        m_fMaxTimeInMSec = fCurrentTimeInMSec;
    }

    m_fAveTimeInMSec = (float)( m_uTotalTime / m_uSampleCount ) / 1000.0f;

    //
    // return parameter for some simple stats
    //
    if ( puTotalTimeInMicroSec != NULL )
    {
        *puTotalTimeInMicroSec = (unsigned int)m_uTotalTimeAllFrame;
    }

    if ( puAveTimeInMicroSec != NULL )
    {
        *puAveTimeInMicroSec = (unsigned int)( m_uTotalTimeAllFrame / m_uSampleCount );
    }

    if ( puThisTimeInMicroSec != NULL )
    {
        *puThisTimeInMicroSec = (unsigned int)uDeltaTime;
    }

    if ( puExecutionCount != NULL )
    {
        *puExecutionCount = (unsigned int)m_uSampleCount;
    }
}

void radProfileSample::StartFrame( )
{
    m_uSampleCountThisFrame = 0;
    m_uTotalTimeThisFrame = 0;
}

void radProfileSample::EndFrame( radTime64 uTotalFrameTimeInMicroSec, radTime64 uAveFrameTimeInMicroSec, radTime64 uThisFrameTimeInMicroSec, unsigned int uFrameCount )
{
    float fTimeThisFrameInPercent = (float)( m_uTotalTimeThisFrame / m_uSampleCountThisFrame ) / (float)uThisFrameTimeInMicroSec;

    m_fAveTimePerFrameInPercent = (float)( m_uTotalTimeAllFrame / m_uSampleCountAllFrame ) / (float)( uAveFrameTimeInMicroSec );

    if ( fTimeThisFrameInPercent < m_fMinTimePerFrameInPercent )
    {
        m_fMinTimePerFrameInPercent = fTimeThisFrameInPercent;
    }

    if ( fTimeThisFrameInPercent > m_fMaxTimePerFrameInPercent )
    {
        m_fMaxTimePerFrameInPercent = fTimeThisFrameInPercent;
    }

    m_uExecutionCountPerFrame = m_uSampleCountAllFrame / uFrameCount;
}

#ifdef RAD_DEBUG

void radProfileSample::DebugDump( )
{
    DebugDumpNode( this, 0 );
}

void radProfileSample::DebugDumpNode( radProfileSample * pNode, unsigned int uLevel )
{
    if ( pNode == NULL )
    {
        return;
    }

    for ( unsigned int i = 0; i < uLevel; i++ )
    {
        rDebugString( "\t" );
    }
    
    rDebugPrintf( "[%s] Ave[%.6f][%.2f%%], Min[%.6f][%.2f%%], Max[%.6f][%.2f%%], Count[%d]", 
                  pNode->GetName( ),
                  pNode->m_fAveTimeInMSec,
                  pNode->m_fAveTimePerFrameInPercent * 100.0f,
                  pNode->m_fMinTimeInMSec,
                  pNode->m_fMinTimePerFrameInPercent * 100.0f,
                  pNode->m_fMaxTimeInMSec,
                  pNode->m_fMaxTimePerFrameInPercent * 100.0f,
                  pNode->m_uSampleCount );

    rDebugString( "\n" );

    //
    // Dump all children
    //
    radProfileSample * pIterator = pNode->m_pFirstChildProfileSample;
    while ( pIterator )
    {
        DebugDumpNode( pIterator, uLevel + 1 );
        pIterator = pIterator->m_pNextSiblingProfileSample;
    }
}

#endif // RAD_DEBUG

#endif // RADPROFILER
