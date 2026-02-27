//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include <radload/radload.hpp>
#include <radload/loader.hpp>
#include <radload/utility/stream.hpp>
#include <radthread.hpp>
#include <radtime.hpp>

radLoadRequest::radLoadRequest()
:
m_tQueued(radTimeGetMilliseconds()),
m_tLoadStart(0),
m_tLoadFinish(0),
m_state( CREATED ),
m_pInventory( NULL )
{
}

radLoadRequest::~radLoadRequest()
{
    if( m_pInventory )
    {
        m_pInventory->Release();
    }
}

void radLoadRequest::Cancel()
{
    m_state = CANCELED;
}

float radLoadRequest::GetPercentDone()
{
    if( m_state == COMPLETE )
    {
        return 1.0f;
    }
    return 0.0f;
}

radLoadInventory* radLoadRequest::GetInventory()
{
    return m_pInventory;
}

radLoadState radLoadRequest::GetState()
{
    return m_state;
}

unsigned int radLoadRequest::GetTotalLoadTime()
{
    if( !m_tLoadFinish && m_tLoadStart )
    {
        return 0;
    }
    return m_tLoadFinish - m_tLoadStart;
}

unsigned int radLoadRequest::GetTotalQueuedTime()
{
    if( !m_tLoadStart )
    {
        return 0;
    }
    return m_tLoadStart - m_tQueued;
}

bool radLoadRequest::IsComplete()
{
    return m_state == COMPLETE;
}

radLoadUpdatableRequest::radLoadUpdatableRequest()
:
m_pStream( NULL )
{
}

radLoadUpdatableRequest::~radLoadUpdatableRequest()
{
    radLoadObject::Release( m_pStream );
}

float radLoadUpdatableRequest::GetPercentDone()
{
    switch( GetState() )
    {
        case COMPLETE:
            return 1.0f;
            break;
        case LOADING:
            if( m_pStream )
            {
                return (static_cast<float>(m_pStream->GetPosition()) / static_cast<float>(m_pStream->GetSize()));
            }
            return 0.0f;
            break;
        default:
            return 0.0f;
            break;
    }
}

void radLoadUpdatableRequest::SetInventory( radLoadInventory* inventory )
{
    if( inventory )
    {
        inventory->AddRef();
    }
    m_pInventory = inventory;
}

void radLoadUpdatableRequest::SetState( radLoadState state )
{
    switch( state )
    {
        case COMPLETE:
            m_tLoadFinish = radTimeGetMilliseconds();
            SetStream( NULL );
            break;
        case LOADING:
            m_tLoadStart = radTimeGetMilliseconds();
            break;
        default:
            break;
    }
    m_state = state;
}

void radLoadUpdatableRequest::SetStream( radLoadStream* stream )
{
    radLoadObject::Assign( m_pStream, stream );
}

radLoadStream* radLoadUpdatableRequest::GetStream()
{
    return m_pStream;
}

