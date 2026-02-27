//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        memmonitorclient.hpp
//
// Subsystem:	Foundation Technologies - Memory monitor
//
// Description:	This file contains class declaration of memory monitor client
//
// Date:    	Nov 7, 2001
//
// Created By : JT
//
//=============================================================================

#ifndef	MEMORYMONITOR_HPP
#define MEMORYMONITOR_HPP

//=============================================================================
// Include Files
//=============================================================================

#include <radoptions.hpp>

#ifdef RADMEMORYMONITOR

#include <radobject.hpp>
#include <raddebugcommunication.hpp>
#include <radmemorymonitor.hpp>
#include <radmemory.hpp>
#include <radthread.hpp>
#include "../radprotocols/memorymonitorprotocol.hpp"

//=============================================================================
// Forward Class Declarations
//=============================================================================

class radMemoryMonitor;

//=============================================================================
// Defintions
//=============================================================================

//
// max number of memory sections user can define, increase this number if
// you are running out of spaces
//
#define MM_MAX_SECTION      48

//
// reciver doesn't need a large buffer
//
#define MM_RX_BUFFER_SIZE   2 * 1024

#define MM_DECLARE_TX_PROTOCAL( ProtocalData ) \
    void AllocateMemoryForSendProtocal( ProtocalData ** pData ); \

#define MM_IMPLEMENT_TX_PROTOCOL( ProtocalType, ProtocalData ) \
void radMemoryMonitorClient::AllocateMemoryForSendProtocal( ProtocalData ** pData ) \
{ \
    *pData = (ProtocalData *)GetNextFillBufferStart( sizeof( ProtocalData ) ); \
    memset( *pData, 0, sizeof( ProtocalData ) ); \
    (*pData)->header = (MM_DataHeader)::radPlatformEndian32( ProtocalType ); \
    AdvanceBufferBy( sizeof( ProtocalData ) ); \
}

#define MM_DECLARE_RX_PROTOCAL( ProtocalData ) \
    void OnRecieveProtocal( ProtocalData * pData );

#define MM_IMPLEMENT_RX_PROTOCAL( ProtocalType, ProtocalData, pRxDataPacket, bEndOfBuffer, uRxBytesProcessed, uRxBytesQueued, bOnRecieveProtocal ) \
    case ProtocalType : \
    { \
        if ( uRxBytesQueued >= sizeof( ProtocalData ) ) \
        { \
            ProtocalData * pData = reinterpret_cast< ProtocalData * >( pRxDataPacket ); \
            if ( bOnRecieveProtocal ) \
            { \
                OnRecieveProtocal( ( ProtocalData * )pData ); \
            } \
            pRxDataPacket = reinterpret_cast< MM_DataPacket * >( (unsigned char *)( pRxDataPacket ) + sizeof( ProtocalData ) ); \
            uRxBytesProcessed += sizeof( ProtocalData ); \
            uRxBytesQueued -= sizeof( ProtocalData ); \
        } \
        else \
        { \
            bEndOfBuffer = true; \
        } \
    } \
    break;

//
// some where to store section declaretion data
//
struct radMemoryMonitorSectionData
{
    void*               address;
    unsigned int        size;
    IRadMemoryMonitor::MemorySectionType   sectionType;
    char                name[ MM_MAX_NAMELENGTH ];
    radMemorySpace      memorySpace;
    IRadMemoryTraverse* traverse;
};

//=============================================================================
// Class Declarations
//=============================================================================

//
// radMemoryMonitorClient -
//      Memory monitor client is the console side object constantly sending
//      information and data about memory usage and memory data back to the
//      host (PC program)
//
class radMemoryMonitorClient :
    public radRefCount,
    public IRadMemoryMonitor,
    public IRadThreadMutex,
    public IRadDbgComChannelStatusCallback,
    public IRadDbgComChannelReceiveCallback,
    public IRadDbgComChannelSendCallback
{
public:

	IMPLEMENT_REFCOUNTED( "radMemoryMonitor" )

    enum MM_InitState
    {
        MM_NotInitialized,
        MM_Initializing,
        MM_Initialized
    };

    //
    // IRadMemoryMonitor Interfaces
    //
    radMemoryMonitorClient( radMemoryAllocator alloc );

    virtual ~radMemoryMonitorClient( );
    
    virtual void Lock( );

    virtual void Unlock( );

    virtual void Initialize( unsigned int BufferSize, bool SyncMode, bool ReportRefCount );

    virtual void Terminate( void );

    virtual void Service( void );

    virtual void Suspend( void );

    virtual void DeclareSection( void* address, unsigned int size, MemorySectionType sectionType, 
                                 radMemorySpace memorySpace, IRadMemoryTraverse* traverse );

    virtual void RescindSection( void* address, radMemorySpace memorySpace );

    virtual void IdentifySection( void* address, const char* name, radMemorySpace memorySpace );

    virtual void DeclareAllocation( void* address, unsigned int size, unsigned int callStackDepth,
                                    radMemorySpace memorySpace );

    virtual void RescindAllocation( void* address, radMemorySpace memorySpace );

    virtual void IdentifyAllocation( void* address, const char* group, const char* name,
                                     unsigned int* pReferenceCount, radMemorySpace memorySpace );

    virtual void IssueFlag( const char* flag );

    virtual void ReportAddRef( void* pObject, void* pReference, radMemorySpace memorySpaceObject );

    virtual void ReportRelease( void* pObject, void* pReference, radMemorySpace memorySpaceObject );

    virtual void SetStackInitialFillChar( char c );

    //
    // Callbacks from the communication system
    //
    virtual void OnStatusChange( IRadDbgComChannel::ConnectionState connectionState,
                                 const char* Message );
    virtual void OnReceiveComplete( bool successful, unsigned int bytesReceived );

    virtual void OnSendComplete( bool successful );

protected:

    //
    // declare Tx protocal
    //
    MM_DECLARE_TX_PROTOCAL( MM_DeclarePlatformData )
    MM_DECLARE_TX_PROTOCAL( MM_DeclareSpaceData )

    MM_DECLARE_TX_PROTOCAL( MM_DeclareSectionData )
    MM_DECLARE_TX_PROTOCAL( MM_RescindSectionData )
    MM_DECLARE_TX_PROTOCAL( MM_IdenitfySectionData )

    MM_DECLARE_TX_PROTOCAL( MM_DeclareAllocationData )
    MM_DECLARE_TX_PROTOCAL( MM_RescindAllocationData )
    MM_DECLARE_TX_PROTOCAL( MM_IdentifyAllocationData )

    MM_DECLARE_TX_PROTOCAL( MM_ReportRefCountData )
    MM_DECLARE_TX_PROTOCAL( MM_ReportAddRefData )
    MM_DECLARE_TX_PROTOCAL( MM_ReportReleaseData )

    MM_DECLARE_TX_PROTOCAL( MM_IssueFlagData )

    MM_DECLARE_TX_PROTOCAL( MM_SendMemoryBlockData )

    MM_DECLARE_TX_PROTOCAL( MM_ReportSuspend )
    MM_DECLARE_TX_PROTOCAL( MM_ReportStackUsageData )

    MM_DECLARE_TX_PROTOCAL( MM_Acknowledgement )

    //
    // declare Rx protocal
    //
    MM_DECLARE_RX_PROTOCAL( MM_RequestRefCountData )

    MM_DECLARE_RX_PROTOCAL( MM_RequestMemoryBlockData );

    MM_DECLARE_RX_PROTOCAL( MM_RequestStackUsageData );

    MM_DECLARE_RX_PROTOCAL( MM_RequestSuspendedData );

    MM_DECLARE_RX_PROTOCAL( MM_RequestResumedData );

    MM_DECLARE_RX_PROTOCAL( MM_Acknowledgement );

    //
    // Rx/Tx related function
    //
    void InitiateTransmission( );

    // get starting position of fill buffer for next item
    unsigned char * GetNextFillBufferStart( unsigned int sizeToAdd );

    void AdvanceBufferBy( unsigned int sizeToAdd );

    void PreProcessRxData( );

    void ProcessRxData( );

    void ReSyncDataIfNeeded( );

    void SendAck( );

    //
    // internal client information declaration
    //
	void DeclareMemSpaceInfo( radMemorySpace memSpace, uintptr_t startAddr, unsigned int size );

    void DeclareMemSpaceInfo( );

    void DeclarePlatform( );

    void DeclarePreDefinedMemorySection( );
    
    void DeclareStoredMemorySection( );

    void SendSuspendState( );
    
    void SendStackUsageData( radMemorySpace memSpace, uintptr_t uAddress );

    // used for DeclareSection()
    void DecalareAllocationFromTraverse( radMemorySpace memorySpace, IRadMemoryTraverse* traverse );

    void InsertSection( radMemoryMonitorSectionData & sectionData );

    void RemoveSection( void* address, radMemorySpace memorySpace );

    void NameSection( void* address, radMemorySpace memorySpace, const char * name );

    void SendMemoryBlock128Byte( radMemorySpace memorySpace, uintptr_t memStartPos, unsigned int memLength );

    void SendMemoryBlock( radMemorySpace memorySpace, uintptr_t memStartPos, unsigned int memLength );

    void SendObjectRefount( radMemorySpace memorySpace, void * pObjectPtr, unsigned int * pRefCount );

    //
    // miscillous function
    //
    unsigned int GetTimeFrame( );

    MM_ClientMemorySpace ConvertMemSpc2ClientMemSpc( radMemorySpace memorySpace );

    radMemorySpace ConvertClientMemSpc2MemSpc( MM_ClientMemorySpace memorySpace );

    // resume the console operation, in pair with Suspend( )
    void Resume( );

    bool IsMemoryBlockValid( radMemorySpace memorySpace, uintptr_t memStartPos, unsigned int memLength );

    bool RemoveCorrespondingAddRefInBuffer( void* pObject, void* pReference, radMemorySpace memorySpaceObject );

private:

    //
    // allocator
    //
    radMemoryAllocator          m_alloc;

    //
    // Is memory monitor initalized
    //
    MM_InitState                m_eInitialized;

    char                        m_cStackInitFillChar;

	//
	// debug communication channel stuff
	//
    radRef< IRadDbgComChannel >    m_pChannel;
    bool                        m_bAttached;
    bool                        m_bSuspended;
    bool                        m_bSendSuspendedState;
    bool                        m_bTxOutstanding;
    unsigned int                m_uAttachedCount;

    bool                        m_bInTransmitting;

    //
    // event id/record id that identifies each allocation/deallocation.
    //
    unsigned int                m_uCurrEventID;

    unsigned int                m_uTimerOffset;

    bool                        m_bOverRunReported;

    bool                        m_bAttemptingConnect;

    //
    // Buffer used to receive/transfer messages from/to host. Fixed size.
    // When data is recieved, it just put recieved data into m_pRxProcessBuffer,
    // set m_bRxProcessBufferQueued flag and wait until Service( ) to process the
    // data
    //
    unsigned char               m_pRxBuffer[ MM_RX_BUFFER_SIZE ];

    unsigned int                m_uRxProcessBufferQueued;
    unsigned char               m_pRxProcessBuffer[ MM_RX_BUFFER_SIZE ];

    unsigned int                m_uRxBufferSize;
    unsigned int                m_uRxBytesQueued;
    unsigned int                m_uRxBytesProcessed;

    unsigned char *             m_pTxBuffer1;
    unsigned char *             m_pTxBuffer2;
    bool                        m_bBuffer1Active;

    unsigned int                m_uTxBufferSize;        // length of each Tx buffer
    unsigned int                m_uTxBytesQueued;

    radMemoryMonitorSectionData m_SectionData[ MM_MAX_SECTION ];    // store user defined section
    bool                        m_bSectionDataInUse[ MM_MAX_SECTION ];  // true/false indicate if section data is valid
    bool                        m_bCreatingChannel;
    bool                        m_bSyncMode;            // should system enforce send previous data before trying send next data
    bool                        m_bReportRefCountMode;  // should system report all reference count data
};

#endif // RADMEMORYMONITOR
#endif // MEMORYMONITOR_HPP