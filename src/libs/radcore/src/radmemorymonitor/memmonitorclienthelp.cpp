//===========================================================================
// Copyright (C) 2001 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   Memory monitor client
//
// Description: This file contains the implementation of the 
//              Memory monitor client.
//
// Authors:     James Tan
//
//===========================================================================

//---------------------------------------------------------------------------
// Includes
//===========================================================================
#include "pch.hpp"

#include <radoptions.hpp>
#include "../radprotocols/memorymonitorprotocol.hpp"

#include <string.h>
#include <radstacktrace.hpp>

#include <radtime.hpp>
#include <radplatform.hpp>
#include "memmonitorclient.hpp"

#ifdef RADMEMORYMONITOR

MM_IMPLEMENT_TX_PROTOCOL( DeclarePlatformType, MM_DeclarePlatformData )
MM_IMPLEMENT_TX_PROTOCOL( DeclareSpaceDataType, MM_DeclareSpaceData )

MM_IMPLEMENT_TX_PROTOCOL( DeclareSectionType, MM_DeclareSectionData )
MM_IMPLEMENT_TX_PROTOCOL( RescindSectionType, MM_RescindSectionData )
MM_IMPLEMENT_TX_PROTOCOL( IdentifySectionType, MM_IdenitfySectionData )

MM_IMPLEMENT_TX_PROTOCOL( DeclareAllocationType, MM_DeclareAllocationData )
MM_IMPLEMENT_TX_PROTOCOL( RescindAllocationType, MM_RescindAllocationData )
MM_IMPLEMENT_TX_PROTOCOL( IdentifyAllocationType, MM_IdentifyAllocationData )

MM_IMPLEMENT_TX_PROTOCOL( ReportRefCountType, MM_ReportRefCountData )
MM_IMPLEMENT_TX_PROTOCOL( ReportAddRefType, MM_ReportAddRefData )
MM_IMPLEMENT_TX_PROTOCOL( ReportReleaseType, MM_ReportReleaseData )

MM_IMPLEMENT_TX_PROTOCOL( IssueFlagType, MM_IssueFlagData )

MM_IMPLEMENT_TX_PROTOCOL( MemoryBlockType, MM_SendMemoryBlockData )

MM_IMPLEMENT_TX_PROTOCOL( ReportSuspendStateType, MM_ReportSuspend )
MM_IMPLEMENT_TX_PROTOCOL( ReportStackUsageType, MM_ReportStackUsageData )

MM_IMPLEMENT_TX_PROTOCOL( AcknowledgementType, MM_Acknowledgement )

//=============================================================================
// Function:    radMemoryMonitorClient::InitiateTransmission
//=============================================================================
// Description: Try to send data if we can.
//              
// Parameters:  
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void radMemoryMonitorClient::InitiateTransmission( )
{
    if ( m_bCreatingChannel == true || m_bInTransmitting == true )
    {
        return;
    }

    m_bInTransmitting = true;

    //
    // make sure we only initialize when debug communication is initialized and
    // we have not initialize before
    //
    if ( m_pChannel == NULL )
    {
        m_bCreatingChannel = true;
        //
        // Get communication channel
        //
        radDbgComTargetCreateChannel( MEM_MONITOR_PROTOCOL, 
                                      &m_pChannel, RADMEMORY_ALLOC_DEFAULT );
        m_eInitialized = MM_Initialized;
        if ( m_pChannel != NULL )
        {

            m_pChannel->RegisterStatusCallback( this );
            //
            // Attach to the host.
            //
            m_pChannel->Attach( );

            m_bTxOutstanding = false;
            m_uRxBytesQueued = 0;
        }
        m_bCreatingChannel = false;
    }

    //
    // Check if attached, no transmit outstanding, we have data to send
    // and send data is acked
    //
    if( ( m_pChannel != NULL ) && ( m_bAttached && !m_bTxOutstanding && ( m_uTxBytesQueued != 0 ) ) )
    {
        unsigned int uTxButesQueued = m_uTxBytesQueued;
        bool bBuffer1Active = m_bBuffer1Active;

        //
        // reset all before send
        //
        m_bBuffer1Active = !m_bBuffer1Active;
        m_uTxBytesQueued = 0;
        m_bTxOutstanding = true;

        if( bBuffer1Active )
        {
            //
            // We will send buffer1 and reset things for buffer 2
            //
            m_pChannel->SendAsync( m_pTxBuffer1, uTxButesQueued, this );
        }
        else
        {
            //
            // We will send buffer2 and reset things for buffer 1
            //
            m_pChannel->SendAsync( m_pTxBuffer2, uTxButesQueued, this );
        }
    }
    m_bInTransmitting = false;
}

void radMemoryMonitorClient::SendStackUsageData( radMemorySpace memSpace, uintptr_t uAddress )
{
#if defined( RAD_WIN32 ) || defined( RAD_UWP )
    rAssert( memSpace == radMemorySpace_Main );
#endif

    uintptr_t uStackPos = -1;        // if we cannot find stack section specified, use -1
    //
    // find out the section host requested, only report if host stack section specified exists
    //
    for ( int nSectionIdx = 0; nSectionIdx < MM_MAX_SECTION; nSectionIdx ++ )
    {
        if ( m_bSectionDataInUse[ nSectionIdx ] )
        {
            if (
                ( m_SectionData[ nSectionIdx ].memorySpace == memSpace ) &&
                ( m_SectionData[ nSectionIdx ].sectionType == MemorySectionType_Stack ) && 
                ( m_SectionData[ nSectionIdx ].address == reinterpret_cast< void * >( uAddress ) )
               )
            {
                //
                // after we found the memory section host requested, let's do search
                //
                uintptr_t uStackEndPos = (uintptr_t)m_SectionData[ nSectionIdx ].address + m_SectionData[ nSectionIdx ].size;
                unsigned char * pStackData = NULL;
                for ( uStackPos = (uintptr_t)m_SectionData[ nSectionIdx ].address; uStackPos < uStackEndPos; uStackPos ++ )
                {
                    pStackData = reinterpret_cast< unsigned char * >( reinterpret_cast< void * >( uStackPos ) );
                    if ( * pStackData != m_cStackInitFillChar )
                    {
                        break;
                    }
                }
            }
        }
    }

    // Get starting position of the buffer
    MM_ReportStackUsageData * pData = NULL;
    AllocateMemoryForSendProtocal( & pData );

    pData->memorySpace    = static_cast< MM_ClientMemorySpace >( radPlatformEndian32( ConvertMemSpc2ClientMemSpc( memSpace ) ) );
    pData->address        = radPlatformEndian( uAddress );
    pData->highWaterMark  = radPlatformEndian( uStackPos );

    InitiateTransmission( );
}

//=============================================================================
// Function:    radMemoryMonitorClient::SendAck
//=============================================================================
// Description: Send ack to host
//              
// Parameters:  
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------
void radMemoryMonitorClient::SendAck( )
{
    rAssert( m_bAttached && m_pChannel );
    //
    // only send ack if attached and has a channel open
    //
    if ( m_bAttached && m_pChannel )
    {
        // Get starting position of the buffer
        MM_Acknowledgement * pAck = NULL;
        AllocateMemoryForSendProtocal( & pAck );

        InitiateTransmission( );
    }
}

//=============================================================================
// Function:    radMemoryMonitorClient::SendSuspendState
//=============================================================================
// Description: Send suspended state to host control
//              
// Parameters:  
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------
void radMemoryMonitorClient::SendSuspendState( )
{
    //
    // send suspend message back to host
    //
    //
    // only send ack if attached and has a channel open
    //
    if ( m_bAttached && m_pChannel )
    {
        // Get starting position of the buffer
        MM_ReportSuspend * pSuspend = NULL;
        AllocateMemoryForSendProtocal( & pSuspend );

        pSuspend->header = static_cast< MM_DataHeader >( radPlatformEndian32( ReportSuspendStateType ) );
        pSuspend->isSuspended = m_bSuspended;

        InitiateTransmission( );
    }
}

//=============================================================================
// Function:    radMemoryMonitorClient::SendMemoryBlock128Byte
//=============================================================================
// Description: sending a block of memory to host, 128 bytes max
//              
// Parameters:  radMemorySpace memorySpace - memory space
//              unsigned int memStartPos   - starting position of memory block
//              unsigned int memLength     - length of memory block
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

bool radMemoryMonitorClient::IsMemoryBlockValid( radMemorySpace memorySpace, uintptr_t memStartPos, unsigned int memLength )
{
    //
    // we use per-recorded section to know if memory address is readable.
    //
    for ( int i = 0; i < MM_MAX_SECTION; i ++ )
    {
        if ( m_bSectionDataInUse[ i ] == true )
        {
            if ( m_SectionData[ i ].memorySpace == memorySpace && memStartPos >= (uintptr_t)m_SectionData[ i ].address && ( memStartPos + memLength ) < ( (uintptr_t)m_SectionData[ i ].address + m_SectionData[ i ].size ) )
            {
                return true;
            }
        }
    }
    return false;
}

//=============================================================================
// Function:    radMemoryMonitorClient::SendMemoryBlock128Byte
//=============================================================================
// Description: sending a block of memory to host, 128 bytes max
//              
// Parameters:  radMemorySpace memorySpace - memory space
//              unsigned int memStartPos   - starting position of memory block
//              unsigned int memLength     - length of memory block
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void radMemoryMonitorClient::SendMemoryBlock128Byte( radMemorySpace memorySpace, uintptr_t memStartPos, unsigned int memLength )
{
    rAssert( memLength <= 128 );

#if defined( RAD_WIN32 ) || defined( RAD_UWP )
    rAssert( memorySpace == radMemorySpace_Main );
#endif

    if ( ! IsMemoryBlockValid( memorySpace, memStartPos, memLength ) )
    {
        return;
    }

    //
    // add send memory block data to the buffer
    //
    MM_SendMemoryBlockData * pData = NULL;
    AllocateMemoryForSendProtocal( & pData );

    pData->header         = static_cast< MM_DataHeader >( radPlatformEndian32( MemoryBlockType ) );
    pData->memorySpace    = static_cast< MM_ClientMemorySpace >( radPlatformEndian32( ConvertMemSpc2ClientMemSpc( memorySpace ) ) );
    pData->memStartPos    = radPlatformEndian( memStartPos );
    pData->memLength      = radPlatformEndian( memLength );

    radRef< IRadMemorySpaceCopyRequest > pCopy = ::radMemorySpaceCopyAsync( pData->memBlock, radMemorySpace_Local, reinterpret_cast< void * >( memStartPos ), memorySpace, memLength );

    if ( pCopy != NULL )
    {
        //
        // do sync copy
        //
        while( ! pCopy->IsDone( ) )
        {
        };
    }

    InitiateTransmission( );
}


//=============================================================================
// Function:    radMemoryMonitorClient::SendMemoryBlock
//=============================================================================
// Description: sending entire block memory content over
//              
// Parameters:  radMemorySpace memorySpace - memory space
//              unsigned int memStartPos   - starting position of memory block
//              unsigned int memLength     - length of memory block
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void radMemoryMonitorClient::SendMemoryBlock( radMemorySpace memorySpace, uintptr_t memStartPos, unsigned int memLength )
{
    //
    // divide memory block into chunck of 128 bytes each, and send one chunck at a time
    //
    int nBlockCount = memLength / 128;
    for ( int i = 0; i < nBlockCount; i ++ )
    {
        SendMemoryBlock128Byte( memorySpace, memStartPos + i * 128, 128 );
    }
}

//=============================================================================
// Function:    radMemoryMonitorClient::SendObjectRefount
//=============================================================================
// Description: send object's refcount value to the host
//              
// Parameters:  
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------
void radMemoryMonitorClient::SendObjectRefount( radMemorySpace memorySpace, void * pObjectPtr, unsigned int * pRefCount )
{
#if defined( RAD_WIN32 ) || defined( RAD_UWP )
    rAssert( memorySpace == radMemorySpace_Main );
#endif

    //
    // add refcount to buffer
    //
    MM_ReportRefCountData * pData = NULL;
    AllocateMemoryForSendProtocal( & pData );

    pData->memorySpace = static_cast< MM_ClientMemorySpace >( radPlatformEndian32( ConvertMemSpc2ClientMemSpc( memorySpace ) ) );
    pData->nObjectPtr = radPlatformEndian( pObjectPtr );
    pData->nRefCountPtr = radPlatformEndian( pRefCount );
    pData->nRefCount = radPlatformEndian( * pRefCount );

    InitiateTransmission( );
}

//=============================================================================
// Function:    radMemoryMonitorClient::GetNextFillBufferStart
//=============================================================================
// Description: get starting position of fill buffer for next item
//              
// Parameters:  unsigned int sizeToAdd - size of data to add to buffer
//
// Returns:     unsigned char * - pointer to buffer start
//
// Notes:
//------------------------------------------------------------------------------
unsigned char * radMemoryMonitorClient::GetNextFillBufferStart( unsigned int sizeToAdd )
{
    //
    // if sync mode, we must wait until previous data has been send to the host before
    // we can start another send!
    //
    while ( m_bSyncMode && m_bTxOutstanding && m_bAttached )
    {
        InitiateTransmission( );
        // spin the dbgCom until data is transfered, and more space is left over
        radDbgComService( );
    }

    //
    // block game if we already attached to host
    //
    while ( ( m_uTxBytesQueued + sizeToAdd ) >= m_uTxBufferSize && m_bAttached )
    {
        InitiateTransmission( );
        // spin the dbgCom until data is transfered, and more space is left over
        radDbgComService( );
    }

    //
    // reserve some memory for allocation happens during connection
    //
    unsigned int uTxBufferWithReservedForDbgCom = m_uTxBufferSize;

    if ( m_bAttemptingConnect == false )
    {
    	rAssertMsg( m_uTxBufferSize > 1024, "radMemoryMonitor : Initialized buffer must be at least 10k, check Initialize( )" );
        uTxBufferWithReservedForDbgCom = m_uTxBufferSize - 1024;
    }

    if ( ( m_uTxBytesQueued + sizeToAdd ) >= uTxBufferWithReservedForDbgCom && ! m_bAttached )
    {
        m_bAttemptingConnect = true;
        //
        // if we never got buffer over run, attempt to connect
        //
        if ( ! m_bOverRunReported )
        {
            unsigned int uBlockStartTime = ::radTimeGetMilliseconds( );
            unsigned int uLastTimeOut = MM_CONNECTION_TIME_OUT / 1000 + 1;
            while( ! m_bAttached && ( ::radTimeGetMilliseconds( ) - uBlockStartTime ) < MM_CONNECTION_TIME_OUT )
            {
                if ( ( ( ::radTimeGetMilliseconds( ) - uBlockStartTime ) % 1000 ) == 0 )
                {
                    unsigned int uCurrTimeOut = ( MM_CONNECTION_TIME_OUT - ( ::radTimeGetMilliseconds( ) - uBlockStartTime ) ) / 1000;
                    if ( uLastTimeOut > uCurrTimeOut )
                    {
                        rDebugPrintf( "MemoryMonitorClient connection time out in [%02d] sec.\n", uCurrTimeOut );
                        uLastTimeOut --;
                    }
                }
                if ( ! m_bCreatingChannel )
                {
                	radDbgComService( );
            	}
            }

            if ( ! m_bAttached )
            {
                rDebugString( "MemoryMonitorClient connection timed out, all stored data is lost.\n" );
                m_bOverRunReported = true;
            }
        }

        //
        // wrap aroud the buffer
        //
        unsigned char * pBuffer = m_bBuffer1Active ? m_pTxBuffer1 : m_pTxBuffer2;

        memset( pBuffer, 0, m_uTxBufferSize );

        m_uTxBytesQueued   = 0;

        return pBuffer + m_uTxBytesQueued;
    }
    else
    {
        return ( m_bBuffer1Active ? m_pTxBuffer1 : m_pTxBuffer2 ) + m_uTxBytesQueued;
    }
}

//=============================================================================
// Function:    radMemoryMonitorClient::AdvanceBufferBy
//=============================================================================
// Description: advance buffer by a size
//              
// Parameters:  unsigned int sizeToAdd - size of data to add to buffer
//
// Returns:     unsigned char * - pointer to buffer start
//
// Notes:
//------------------------------------------------------------------------------
void radMemoryMonitorClient::AdvanceBufferBy( unsigned int sizeToAdd )
{
    m_uTxBytesQueued += sizeToAdd;
}

//=============================================================================
// Function:    radMemoryMonitorClient::Resume
//=============================================================================
// Description: Resume the client/exit infinite loop
//              
// Parameters:  None
//
// Returns:     None
//
// Notes:
//------------------------------------------------------------------------------
void radMemoryMonitorClient::Resume( )
{
    m_bSuspended = false;
    m_bSendSuspendedState = true;
}

//=============================================================================
// Function:    radMemoryMonitorClient::DecalareAllocationFromTraverse
//=============================================================================
// Description: declare allocation from a traverse tree
//              
// Parameters:  None
//
// Returns:     None
//
// Notes:
//------------------------------------------------------------------------------
void radMemoryMonitorClient::DecalareAllocationFromTraverse( radMemorySpace memorySpace, IRadMemoryTraverse* traverse )
{
    rAssert( traverse );
    traverse->Reset( );
 
    void * address = NULL;
    unsigned int size;
    unsigned int group;
    char* name = NULL;
    
    while( traverse->GetNext( & address, & size, & group, name ) )
    {
        DeclareAllocation( address, size, 0, memorySpace );
    }
}

//=============================================================================
// Function:    radMemoryMonitorClient::GetTimeFrame
//=============================================================================
// Description: get time frame for current event
//              
// Parameters:  None
//
// Returns:     None
//
// Notes:
//------------------------------------------------------------------------------
unsigned int radMemoryMonitorClient::GetTimeFrame( )
{
    if ( m_pChannel == NULL )
    {
        return 0;
    }

    if ( m_uTimerOffset == 0 )
    {
        m_uTimerOffset = ::radTimeGetMilliseconds( );
    }

    return ::radTimeGetMilliseconds( ) - m_uTimerOffset;
}

void radMemoryMonitorClient::InsertSection( radMemoryMonitorSectionData & sectionData )
{
    //
    // make sure the section is not already recorded
    //
    for ( int i = 0; i < MM_MAX_SECTION; i ++ )
    {
        if ( ( m_bSectionDataInUse[ i ] == true ) && ( m_SectionData[ i ].address == sectionData.address ) )
        {
            return;
        }
    }

    //
    // add to empty slot
    //
    for ( int j = 0; j < MM_MAX_SECTION; j ++ )
    {
        if ( m_bSectionDataInUse[ j ] == false )
        {
            m_SectionData[ j ] = sectionData;
            m_bSectionDataInUse[ j ] = true;
            return;
        }
    }

    // increase number of sections (MM_MAX_SECTION) defined in the radmemorymonitor.hpp
    rAssert( 0 );
}

void radMemoryMonitorClient::RemoveSection( void* address, radMemorySpace memorySpace )
{
    int i, j;
    for ( i = 0; i < MM_MAX_SECTION; i ++ )
    {
        if ( m_bSectionDataInUse[ i ] == true )
        {
            if ( m_SectionData[ i ].address == address && m_SectionData[ i ].memorySpace == memorySpace )
            {
                m_bSectionDataInUse[ i ] = false;
                break;
            }
        }
    }

    //
    // now compacting, and move all section data to the font
    //
    for ( j = i + 1; j < MM_MAX_SECTION; j ++ )
    {
        if ( m_bSectionDataInUse[ j ] == true )
        {
            m_SectionData[ j - 1 ] = m_SectionData[ j ];
            m_bSectionDataInUse[ j - 1 ] = true;
            m_bSectionDataInUse[ j ] = false;
        }
    }
}

void radMemoryMonitorClient::NameSection( void* address, radMemorySpace memorySpace, const char * name )
{
    //
    // make sure the section is not already recorded
    //
    for ( int i = 0; i < MM_MAX_SECTION; i ++ )
    {
        if ( ( m_bSectionDataInUse[ i ] == true ) && ( m_SectionData[ i ].address == address ) )
        {
            strcpy( m_SectionData[ i ].name, name );
            return;
        }
    }
}

void radMemoryMonitorClient::DeclareStoredMemorySection( )
{
    for ( int i = 0; i < MM_MAX_SECTION; i ++ )
    {
        if ( m_bSectionDataInUse[ i ] == true )
        {
            DeclareSection( m_SectionData[ i ].address, m_SectionData[ i ].size, m_SectionData[ i ].sectionType, m_SectionData[ i ].memorySpace, m_SectionData[ i ].traverse );

            if ( strlen( m_SectionData[ i ].name ) > 0 )
            {
                IdentifySection( m_SectionData[ i ].address, m_SectionData[ i ].name, m_SectionData[ i ].memorySpace );
            }
        }
    }
}

void radMemoryMonitorClient::OnRecieveProtocal( MM_RequestRefCountData * pData )
{
    pData->memorySpace = static_cast< MM_ClientMemorySpace >( radPlatformEndian( pData->memorySpace ) );
    pData->nObjectPtr = radPlatformEndian( pData->nObjectPtr );
    pData->nRefCountPtr = radPlatformEndian( pData->nRefCountPtr );

    SendObjectRefount( ConvertClientMemSpc2MemSpc( pData->memorySpace ), (void*)pData->nObjectPtr, reinterpret_cast< unsigned int * >( pData->nRefCountPtr ) );
}

void radMemoryMonitorClient::OnRecieveProtocal( MM_RequestMemoryBlockData * pData )
{
    pData->memorySpace = static_cast< MM_ClientMemorySpace >( radPlatformEndian( pData->memorySpace ) );
    pData->memStartPos = radPlatformEndian( pData->memStartPos );
    pData->memLength = radPlatformEndian( pData->memLength );

    SendMemoryBlock( ConvertClientMemSpc2MemSpc( pData->memorySpace ), pData->memStartPos, pData->memLength );
}

void radMemoryMonitorClient::OnRecieveProtocal( MM_RequestStackUsageData * pData )
{
    pData->memorySpace = static_cast< MM_ClientMemorySpace >( radPlatformEndian( pData->memorySpace ) );
    pData->address = radPlatformEndian( pData->address );

    SendStackUsageData( ConvertClientMemSpc2MemSpc( pData->memorySpace ), pData->address );
}

void radMemoryMonitorClient::OnRecieveProtocal( MM_RequestSuspendedData * pData )
{
    Suspend( );
}

void radMemoryMonitorClient::OnRecieveProtocal( MM_RequestResumedData * pData )
{
    Resume( );
}

void radMemoryMonitorClient::OnRecieveProtocal( MM_Acknowledgement * pData )
{
}

void radMemoryMonitorClient::PreProcessRxData( )
{
    MM_DataPacket * pRxDataPacket = reinterpret_cast< MM_DataPacket * >( m_pRxBuffer );

    bool bEndOfBuffer = false;

    while( ( radPlatformEndian32( pRxDataPacket->header ) != DummyType && ! bEndOfBuffer ) && m_uRxBytesQueued > 0 )
    {
        switch( radPlatformEndian32( pRxDataPacket->header ) )
        {
            MM_IMPLEMENT_RX_PROTOCAL( RequestMemoryBlockType, MM_RequestMemoryBlockData, pRxDataPacket, bEndOfBuffer, m_uRxBytesProcessed, m_uRxBytesQueued, false );
            MM_IMPLEMENT_RX_PROTOCAL( RequestStackUsageType, MM_RequestStackUsageData, pRxDataPacket, bEndOfBuffer, m_uRxBytesProcessed, m_uRxBytesQueued, false );
            MM_IMPLEMENT_RX_PROTOCAL( RequestSuspendType, MM_RequestSuspendedData, pRxDataPacket, bEndOfBuffer, m_uRxBytesProcessed, m_uRxBytesQueued, false );
            MM_IMPLEMENT_RX_PROTOCAL( RequestResumeType, MM_RequestResumedData, pRxDataPacket, bEndOfBuffer, m_uRxBytesProcessed, m_uRxBytesQueued, false );
            MM_IMPLEMENT_RX_PROTOCAL( AcknowledgementType, MM_Acknowledgement, pRxDataPacket, bEndOfBuffer, m_uRxBytesProcessed, m_uRxBytesQueued, false );

            default:
            {
                rAssertMsg( false, "MemoryMonitorClient : Unknown protocol recieved.\n" );
            }
            break;
        }
    }
}

void radMemoryMonitorClient::ProcessRxData( )
{
    MM_DataPacket * pRxDataPacket = reinterpret_cast< MM_DataPacket * >( m_pRxProcessBuffer );

    bool bEndOfBuffer = false;

    unsigned int uRxBytesProcessed = 0;

    while( ( radPlatformEndian32( pRxDataPacket->header ) != DummyType && ! bEndOfBuffer ) && m_uRxProcessBufferQueued > 0 )
    {
        switch( radPlatformEndian32( pRxDataPacket->header ) )
        {
            MM_IMPLEMENT_RX_PROTOCAL( RequestMemoryBlockType, MM_RequestMemoryBlockData, pRxDataPacket, bEndOfBuffer, uRxBytesProcessed, m_uRxProcessBufferQueued, true );
            MM_IMPLEMENT_RX_PROTOCAL( RequestStackUsageType, MM_RequestStackUsageData, pRxDataPacket, bEndOfBuffer, uRxBytesProcessed, m_uRxProcessBufferQueued, true );
            MM_IMPLEMENT_RX_PROTOCAL( RequestSuspendType, MM_RequestSuspendedData, pRxDataPacket, bEndOfBuffer, uRxBytesProcessed, m_uRxProcessBufferQueued, true );
            MM_IMPLEMENT_RX_PROTOCAL( RequestResumeType, MM_RequestResumedData, pRxDataPacket, bEndOfBuffer, uRxBytesProcessed, m_uRxProcessBufferQueued, true );
            MM_IMPLEMENT_RX_PROTOCAL( AcknowledgementType, MM_Acknowledgement, pRxDataPacket, bEndOfBuffer, uRxBytesProcessed, m_uRxProcessBufferQueued, true );

            default:
            {
                rAssertMsg( false, "MemoryMonitorClient : Unknown protocol recieved.\n" );
            }
            break;
        }
    }
    rAssert( m_uRxProcessBufferQueued == 0 );
}

void radMemoryMonitorClient::ReSyncDataIfNeeded( )
{
    if ( ! m_bOverRunReported && m_uAttachedCount == 1 )
    {
        return;
    }
    else if ( m_bOverRunReported || m_uAttachedCount > 1 )
    {
        //
        // clear Tx buffer completly.
        // Since the data in the buffer is useless after some are
        // lost before host is connected
        //
        memset( m_pTxBuffer1, 0, m_uTxBufferSize * 2 );
        m_bBuffer1Active = true;
        m_uTxBytesQueued = 0;
        m_bTxOutstanding = false;

        DeclarePlatform( );
        DeclareMemSpaceInfo( );
        DeclareStoredMemorySection( );
    }
}

MM_ClientMemorySpace radMemoryMonitorClient::ConvertMemSpc2ClientMemSpc( radMemorySpace memorySpace )
{
	switch( memorySpace )
	{
#if defined( RAD_WIN32 ) || defined( RAD_UWP )
		case radMemorySpace_Main:
		{
			return MM_WIN_MAIN;
		}
		break;
#endif
		default:
		{
            return MM_UNKNOWN;
		}
		break;
	};
    return MM_UNKNOWN;
}

radMemorySpace radMemoryMonitorClient::ConvertClientMemSpc2MemSpc( MM_ClientMemorySpace memorySpace )
{
	switch( memorySpace )
	{
#if defined( RAD_WIN32 ) || defined( RAD_UWP )
		case MM_WIN_MAIN:
		{
			return radMemorySpace_Main;
		}
		break;
#endif
		default:
		{
            rAssert( false );
		}
		break;
	};
    return (radMemorySpace)(0);
}

bool radMemoryMonitorClient::RemoveCorrespondingAddRefInBuffer( void* pObject, void* pReference, radMemorySpace memorySpaceObject )
{
    //
    // First iterate through every packet and find out if any AddRef is corresponded, hmm, evil for non-unified length packet :(
    //
    unsigned char * const pBufferBeg = m_bBuffer1Active ? m_pTxBuffer1 : m_pTxBuffer2;
    unsigned char * const pBufferEnd = & pBufferBeg[ m_uTxBytesQueued ];
    unsigned char * pBufferCur = pBufferBeg;

    bool bFoundMatch = false;

    while( pBufferCur < pBufferEnd && bFoundMatch == false )
    {
        //
        // read one packet at a time
        //
        MM_DataPacket * pPacket = reinterpret_cast< MM_DataPacket * >( pBufferCur );
        if ( radPlatformEndian32( pPacket->header ) == ReportAddRefType )
        {
            MM_ReportAddRefData * pAddRefPacket = static_cast< MM_ReportAddRefData * >( pPacket );
            if ( ( pAddRefPacket->memorySpaceObject == static_cast< MM_ClientMemorySpace >( radPlatformEndian32( ConvertMemSpc2ClientMemSpc( memorySpaceObject ) ) ) ) &&
                 ( pAddRefPacket->object == radPlatformEndian( reinterpret_cast< uintptr_t >( pObject ) ) ) &&
                 ( pAddRefPacket->refObject == radPlatformEndian( reinterpret_cast< uintptr_t >( pReference ) ) ) )
            {
                bFoundMatch = true;
                break;
            }
        }
        pBufferCur += radMemoryMonitorGetPacketSize( (MM_DataHeader)( radPlatformEndian32( pPacket->header ) ) );
    }

    if ( ! bFoundMatch )
    {
        return false;
    }

    const unsigned int uReportAddRefPacketSize = radMemoryMonitorGetPacketSize( ReportAddRefType );

    unsigned char * pBufferDest = pBufferCur;
    unsigned char * pBufferSour = pBufferCur + uReportAddRefPacketSize;
    unsigned int uSizeToCopy = 0;

    MM_DataPacketBlock tempBuffer;

    while( (uintptr_t)pBufferSour < (uintptr_t)pBufferEnd )
    {
        uSizeToCopy = radMemoryMonitorGetPacketSize( (MM_DataHeader)( radPlatformEndian32( reinterpret_cast< MM_DataPacket * >( pBufferSour )->header ) ) );
        rAssert( uSizeToCopy <= sizeof( tempBuffer ) );
        // copy to temp buffer to avoid memcpy overlap
        //
        memcpy( &tempBuffer, pBufferSour, uSizeToCopy );
        memset( pBufferDest, 0, uSizeToCopy + uReportAddRefPacketSize );
        memcpy( pBufferDest, &tempBuffer, uSizeToCopy );

        pBufferSour += uSizeToCopy;
        pBufferDest += uSizeToCopy;
    }

    m_uTxBytesQueued -= uReportAddRefPacketSize;
    return true;
}

#endif // RADMEMORYMONITOR