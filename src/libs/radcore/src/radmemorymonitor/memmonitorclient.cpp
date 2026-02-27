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

#include <string.h>
#include <radstacktrace.hpp>

#include <radtime.hpp>
#include <radplatform.hpp>
#include "memmonitorclient.hpp"
#include "../radprotocols/memorymonitorprotocol.hpp"

#ifdef RADMEMORYMONITOR
//===========================================================================
// Forward declaration
//===========================================================================

//===========================================================================
// Global Data, Local Data, Local Classes
//===========================================================================
//
// singlton object for memory monitor client
//
static radMemoryMonitorClient * s_TheMemoryMonitorClient = NULL;

#ifdef RAD_MW
//
// Just add this to make Metrowerks compile, I don't care if the memory
// monitor runs under it right now. -- DE
//
unsigned int _codestart = 0;
unsigned int _codeend = 0;
unsigned int _datastart = 0;
unsigned int _dataend = 0;
#endif

//===========================================================================
// Static Data Definitions
//===========================================================================

//===========================================================================
// Global Functions
//===========================================================================
//===========================================================================
// radMemoryMonitorInitialize
//===========================================================================
// Description: Initialize memory monitor!!!
//
// Constraints: must not be already initalized
//
// Parameters:  None
//
//===========================================================================
void radMemoryMonitorInitialize( unsigned int bufferSize, radMemoryAllocator alloc, bool SyncMode, bool ReportRefCount )
{
    //
    // Assert that this subsystem has not allready been initialized.
    //
    rAssertMsg( s_TheMemoryMonitorClient == NULL, "Memory Monitor System already initialized");

    //
    // Construct the object.
    //
    radMemoryMonitorClient * pClient = new( alloc ) radMemoryMonitorClient( alloc );
    pClient->Initialize( bufferSize, SyncMode, ReportRefCount );
    s_TheMemoryMonitorClient = pClient;
}

//===========================================================================
// radMemoryMonitorTerminate
//===========================================================================
// Description: Terminate memory monitor!!!
//
// Constraints: must be already initalized
//
// Parameters:  None
//
//===========================================================================
void radMemoryMonitorTerminate( void )
{
    rAssertMsg( s_TheMemoryMonitorClient != NULL, "Memory Monitor System not initialized");

    radMemoryMonitorClient * pOldPtr = s_TheMemoryMonitorClient;
    s_TheMemoryMonitorClient = NULL;
    pOldPtr->Terminate( );
    radRelease( static_cast< IRadMemoryMonitor* >( pOldPtr ), NULL );
}

//===========================================================================
// radMemoryMonitorGet
//===========================================================================
// Description: return the singlton object of memory monitor
//
// Constraints: Not yet
//
// Parameters:  None
//
//===========================================================================
IRadMemoryMonitor* radMemoryMonitorGet( void )
{
    return s_TheMemoryMonitorClient;
}

//===========================================================================
// radMemoryMonitorService
//===========================================================================
// Description: call this function to service the memory monitor
//
// Constraints: Not yet
//
// Parameters:  None
//
//===========================================================================
void radMemoryMonitorService( void )
{
    radDbgComService( );
    s_TheMemoryMonitorClient->Service( );
}


//===========================================================================
// Class Methods
//===========================================================================

//=============================================================================
// Function:    radMemoryMonitorClient::radMemoryMonitorClient
//=============================================================================
// Description: Constructor for the client object.
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------
radMemoryMonitorClient::radMemoryMonitorClient( radMemoryAllocator alloc ) :
    m_alloc( alloc ),
    m_eInitialized( MM_NotInitialized ),
    m_cStackInitFillChar( 0 ),
    m_bAttached( false ),
    m_bSuspended( false ),
    m_bSendSuspendedState( true ),
    m_bTxOutstanding( false ),
    m_uAttachedCount( 0 ),
    m_bInTransmitting( false ),
    m_uCurrEventID( 0 ),
    m_uTimerOffset( 0 ),
    m_bOverRunReported( false ),
    m_bAttemptingConnect( false ),
    m_uRxProcessBufferQueued( 0 ),
    m_uRxBufferSize( sizeof( m_pRxBuffer ) ),
    m_uRxBytesQueued( 0 ),
    m_uRxBytesProcessed( 0 ),
    m_pTxBuffer1( NULL ),
    m_pTxBuffer2( NULL ),
    m_bBuffer1Active( true ),
    m_uTxBufferSize( 0 ),
    m_uTxBytesQueued( 0 ),
    m_bCreatingChannel( false ),
    m_bSyncMode( false ),
    m_bReportRefCountMode( false )
{
}

//=============================================================================
// Function:    radMemoryMonitorClient::~radMemoryMonitorClient
//=============================================================================
// Description: Destructor for the server object. 
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

radMemoryMonitorClient::~radMemoryMonitorClient( void )
{
    //
    // memory monitor must be terminated first
    //
    rAssert( m_eInitialized == MM_NotInitialized );
}

//===========================================================================
// radMemoryMonitorClient::Initialize
//===========================================================================
// Description: Initialize the system. Must call before all other calls.
//
// Constraints: Not yet
//
// Parameters:  unsigned int BufferSize - size of the buffer
//				bool SyncMode
//
//===========================================================================
void radMemoryMonitorClient::Lock( )
{
    radDbgComMutexLock( );
}

//===========================================================================
// radMemoryMonitorClient::Initialize
//===========================================================================
// Description: Initialize the system. Must call before all other calls.
//
// Constraints: Not yet
//
// Parameters:  unsigned int BufferSize - size of the buffer
//				bool SyncMode
//
//===========================================================================
void radMemoryMonitorClient::Unlock( )
{
    radDbgComMutexUnlock( );
}

//===========================================================================
// radMemoryMonitorClient::Initialize
//===========================================================================
// Description: Initialize the system. Must call before all other calls.
//
// Constraints: Not yet
//
// Parameters:  unsigned int BufferSize - size of the buffer
//				bool SyncMode
//
//===========================================================================
void radMemoryMonitorClient::Initialize( unsigned int BufferSize, bool SyncMode, bool ReportRefCount )
{
    radSingleLock< radMemoryMonitorClient > singleLock( this, true );
    //
    // cannot initialize more than once
    //
    rAssert( m_eInitialized == MM_NotInitialized );

    //
    // radMemory must be initialized first.
    //
    // rAssert( ::IsRadMemoryInitialized( ) );

    //
    // set Initializing state to prevent cycluar declare memory allocation
    //
    m_eInitialized = MM_Initializing;

    m_cStackInitFillChar = 0;
    // buffer size must be even number, so we can divide between
    // two double buffer evenly.
    BufferSize += ( BufferSize % 2 );

    //
    // don't initialize debug channel at this time, initialize on demand, and
    // after debug communcation system is initialized.
    //
    m_pChannel              = NULL;
    m_bAttached             = false;
    m_bSuspended            = false;

    m_uCurrEventID          = 1;    // must start with 1
    m_uTimerOffset          = 0;

    m_uRxBufferSize         = MM_RX_BUFFER_SIZE;
    memset( m_pRxBuffer, 0, MM_RX_BUFFER_SIZE );
    m_uRxBytesQueued        = 0;
    m_uRxBytesProcessed     = 0;

    m_pTxBuffer1            = NULL;
    m_pTxBuffer2            = NULL;
    m_bBuffer1Active        = true;     // buffer 1 start first
    m_uTxBufferSize         = BufferSize / 2;
    m_uTxBytesQueued        = 0;
    m_bTxOutstanding        = false;
    m_bOverRunReported      = false;
    m_uAttachedCount        = 0;
    m_bAttemptingConnect    = false;
    m_bInTransmitting       = false;

    m_bSyncMode             = SyncMode;
    m_bReportRefCountMode   = ReportRefCount;

    memset( & m_SectionData, 0, sizeof( m_SectionData ) );
    memset( & m_bSectionDataInUse, 0, sizeof( m_bSectionDataInUse ) );

    //
    // now do allocation of Tx buffer
    //
    m_pTxBuffer1 = static_cast< unsigned char * >( ::radMemoryAlloc( m_alloc, m_uTxBufferSize * 2 ) );
    rAssert( m_pTxBuffer1 != NULL );
    memset( m_pTxBuffer1, 0, m_uTxBufferSize * 2 );
    m_pTxBuffer2 = m_pTxBuffer1 + m_uTxBufferSize;

    //
    // indicate that memory monitor is initialized, and it prevent problem of cycular
    // declare memory allocation.
    //
    m_eInitialized          = MM_Initialized;
    m_bCreatingChannel      = false;

    //
    // Decalre platform data, and memory space data.
    // This must be done right in the font.
    // There's another condition those functions are called as well, it is
    // when host connected to the console system after some data is already
    // lost.
    //
    DeclarePlatform( );
    DeclareMemSpaceInfo( );
    DeclarePreDefinedMemorySection( );

    //
    // declare allocation we did perviously for the Memory Monitor system
    //
    DeclareAllocation( this, sizeof( radMemoryMonitorClient ), MM_MAX_CALLSTACK_DEPTH, radMemorySpace_Local );
    IdentifyAllocation( this, g_nameFTech, "radMemoryMonitorClient", NULL, radMemorySpace_Local );
    //
    // declare allocation we did perviously for the Tx buffer
    //
    DeclareAllocation( m_pTxBuffer1, m_uTxBufferSize * 2, MM_MAX_CALLSTACK_DEPTH, radMemorySpace_Local );
    IdentifyAllocation( m_pTxBuffer1, g_nameFTech, "radmemoryMonitorClient::m_pTxBuffer1", NULL, radMemorySpace_Local );
}

//===========================================================================
// radMemoryMonitorClient::Terminate
//===========================================================================
// Description: Terminate memory monitor system
//
// Constraints: Not yet
//
// Parameters:  
//
//===========================================================================
void radMemoryMonitorClient::Terminate( void )
{
    radSingleLock< radMemoryMonitorClient > singleLock( this, true );
    rAssertMsg( m_eInitialized == MM_Initialized, "radMemoryMonitor not initialized." );

    //
    // let host know that Tx buffer is about to be deleted.
    //
    RescindAllocation( m_pTxBuffer1, radMemorySpace_Local );
    //
    // let host know memory monitor is gone too.
    //
    RescindAllocation( this, radMemorySpace_Local );

    //
    // allow all data to be transfer to the host before freeing Rx buffer
    //
    unsigned int uWhileLoopStartTime = ::radTimeGetMilliseconds( );
    while ( m_uTxBytesQueued > 0 )
    {
        Service( );

        //
        // time out for host to recieve all data before game ends
        //
        if ( ( ::radTimeGetMilliseconds( ) - uWhileLoopStartTime ) > 5000 )
        {
            rDebugString( "radMemoryMonitor : 5 sec Timeout in radMemoryMonitorClient::Terminate( ), not all data are send to the host for processing.\n" );
            break;
        }
    }

    //
    // Do another 100 service just in case some very nasty timing issues.
    // Those nasty timing issues has symptoms that host cannot recieve all
    // data, and some packet get lost. [Not reproducable]
    //
    for ( int i = 0; i < 100; i ++ )
    {
        Service( );
    }

    //
    // Detach from the host.
    //
    if ( m_pChannel != NULL )
    {
        m_pChannel->Detach( );
        m_pChannel->RegisterStatusCallback( NULL );
        m_pChannel = NULL;
    }

    m_eInitialized = MM_NotInitialized;

    //
    // now free the Tx Buffer
    //
    ::radMemoryFree( m_pTxBuffer1 );
}

//===========================================================================
// radMemoryMonitorClient::Service
//===========================================================================
// Description: Service routine, get's called every sometime
//
// Constraints: Not yet
//
// Parameters:  None
//
//===========================================================================
void radMemoryMonitorClient::Service( void )
{
    radSingleLock< radMemoryMonitorClient > singleLock( this, true );

    rAssertMsg( m_eInitialized == MM_Initialized, "radMemoryMonitor not initialized." );

    if ( m_uRxProcessBufferQueued > 0 )
    {
        ProcessRxData( );
        //
        // hand shake, single side only
        //
        SendAck( );
    }

    if ( m_bSendSuspendedState )
    {
        SendSuspendState( );
        m_bSendSuspendedState = false;
    }

    //
    // can only suspend the game while channel is open
    //
    while ( m_bSuspended && m_pChannel && m_bAttached )
    {
#if defined( RAD_WIN32 ) || defined( RAD_UWP )
        ::Sleep( 5 );   // if on PC, don't take entire CPU cycle
#endif
        radDbgComService( );
        InitiateTransmission( );

        if ( m_uRxProcessBufferQueued > 0 )
        {
            ProcessRxData( );
            //
            // hand shake, single side only
            //
            SendAck( );
        }

        if ( m_bSendSuspendedState )
        {
            SendSuspendState( );
            m_bSendSuspendedState = false;
        }
    }
}

//===========================================================================
// radMemoryMonitorClient::Suspend
//===========================================================================
// Description: Suspend memory monitor until host issue resume command
//
// Constraints: Not yet
//
// Parameters:  None
//
//===========================================================================
void radMemoryMonitorClient::Suspend( void )
{
    radSingleLock< radMemoryMonitorClient > singleLock( this, true );

    rAssertMsg( m_eInitialized == MM_Initialized, "radMemoryMonitor not initialized." );

    //
    // cannot suspend before initialization
    //
    if ( m_pChannel == NULL )
    {
        rDebugString( "warning : radMemoryMonitor : cannot suspended before system is initialized.\n" );
        return;
    }

    m_bSuspended = true;
    m_bSendSuspendedState = true;
}

//===========================================================================
// radMemoryMonitorClient::DeclarePlatform
//===========================================================================
// Description: declare platform type
//
// Constraints: Not yet
//
// Parameters:  
//
//===========================================================================
void radMemoryMonitorClient::DeclarePlatform( )
{
    radSingleLock< radMemoryMonitorClient > singleLock( this, true );

    rAssertMsg( m_eInitialized == MM_Initialized, "radMemoryMonitor not initialized." );

    if ( m_bOverRunReported == true )
    {
        return;
    }

    MM_DeclarePlatformData * pPlatform = NULL;
    AllocateMemoryForSendProtocal( & pPlatform );

    pPlatform->eventID  = radPlatformEndian32( m_uCurrEventID ); m_uCurrEventID ++;
    pPlatform->timeStamp = radPlatformEndian32( GetTimeFrame( ) );
#if defined( RAD_WIN32 ) || defined( RAD_UWP )
    pPlatform->platform = static_cast< MM_ClientPlatform >( radPlatformEndian32( MM_Platform_WIN ) );
    pPlatform->userData = 0;
#endif
    InitiateTransmission( );
}

//===========================================================================
// radMemoryMonitorClient::DeclareMemSpaceInfo
//===========================================================================
// Description: declare memory space
//
// Constraints: Not yet
//
// Parameters:  
//
//===========================================================================
void radMemoryMonitorClient::DeclareMemSpaceInfo( radMemorySpace memSpace, uintptr_t startAddr, unsigned int size )
{
    radSingleLock< radMemoryMonitorClient > singleLock( this, true );
    rAssertMsg( m_eInitialized == MM_Initialized, "radMemoryMonitor not initialized." );

    if ( m_bOverRunReported == true )
    {
        return;
    }

    MM_DeclareSpaceData * pData = NULL;
    AllocateMemoryForSendProtocal( & pData );

    pData->timeStamp    = radPlatformEndian( GetTimeFrame( ) );
    pData->eventID      = radPlatformEndian( m_uCurrEventID ); m_uCurrEventID ++;
    pData->addrStart    = radPlatformEndian( startAddr );
    pData->size         = radPlatformEndian( size );
    pData->memorySpace  = static_cast< MM_ClientMemorySpace >( radPlatformEndian( ConvertMemSpc2ClientMemSpc( memSpace ) ) );

    InitiateTransmission( );
}

//===========================================================================
// radMemoryMonitorClient::DeclarePlatform
//===========================================================================
// Description: declare all memory space in the platform
//
// Constraints: Not yet
//
// Parameters:  
//
//===========================================================================
void radMemoryMonitorClient::DeclareMemSpaceInfo( )
{
    radSingleLock< radMemoryMonitorClient > singleLock( this, true );
    rAssertMsg( m_eInitialized == MM_Initialized, "radMemoryMonitor not initialized." );

    if ( m_bOverRunReported == true )
    {
        return;
    }

#if defined( RAD_WIN32 ) || defined( RAD_UWP )
    DeclareMemSpaceInfo( radMemorySpace_Main, 0x00000000, 0xffffffff );
#endif
}

//===========================================================================
// radMemoryMonitorClient::DeclarePreDefinedMemorySection
//===========================================================================
// Description: declare all memory space in the platform
//
// Constraints: Not yet
//
// Parameters:  
//
//===========================================================================
void radMemoryMonitorClient::DeclarePreDefinedMemorySection( )
{
    radSingleLock< radMemoryMonitorClient > singleLock( this, true );
    rAssertMsg( m_eInitialized == MM_Initialized, "radMemoryMonitor not initialized." );

    if ( m_bOverRunReported == true )
    {
        return;
    }

#if defined( RAD_WIN32 ) || defined( RAD_UWP )

    DeclareSection( (void*)(uintptr_t)0x00000000, 0xcfffffff, MemorySectionType_DynamicData, radMemorySpace_Main, NULL );

    DeclareSection( (void*)(uintptr_t)0xd0000000, 0xffffffff - 0xd0000000, MemorySectionType_Stack, radMemorySpace_Main, NULL );

#endif
}

//===========================================================================
// radMemoryMonitorClient::DeclareSection
//===========================================================================
// Description: Delcare a section of memory with information
//
// Constraints: Not yet
//
// Parameters:  void* address - starting position of the section
//              unsigned int size - size of the section
//              MemorySectionType sectionType - section type id
//              radMemorySpace memorySpace - memory space id
//              IRadMemoryTraverse* traverse - memory traverse interface
//===========================================================================
void radMemoryMonitorClient::DeclareSection( void* address, unsigned int size, MemorySectionType sectionType, 
                                             radMemorySpace memorySpace, IRadMemoryTraverse* traverse )
{
    radSingleLock< radMemoryMonitorClient > singleLock( this, true );
    rAssertMsg( m_eInitialized == MM_Initialized, "radMemoryMonitor not initialized." );

    if ( m_bOverRunReported == true )
    {
        return;
    }

    MM_DeclareSectionData * pDeclareSectionData = NULL;
    AllocateMemoryForSendProtocal( & pDeclareSectionData );

    // copy over all the data
    pDeclareSectionData->eventID        = radPlatformEndian( m_uCurrEventID );   m_uCurrEventID++;
    pDeclareSectionData->timeStamp      = radPlatformEndian( GetTimeFrame( ) );
    pDeclareSectionData->address        = radPlatformEndian( reinterpret_cast< uintptr_t >( address ) );
    pDeclareSectionData->size           = radPlatformEndian( size );
    pDeclareSectionData->memorySpace    = static_cast< MM_ClientMemorySpace >( radPlatformEndian( ConvertMemSpc2ClientMemSpc( memorySpace ) ) );
    pDeclareSectionData->sectionType    = static_cast< MemorySectionType >( radPlatformEndian( sectionType ) );

    InitiateTransmission( );

    //
    // add this section to local data structure and keep track of
    //
    radMemoryMonitorSectionData sectionData;
    sectionData.address = address;
    sectionData.size    = size;
    strcpy( sectionData.name, "" );
    sectionData.sectionType = sectionType;
    sectionData.memorySpace = memorySpace;
    sectionData.traverse = traverse;
    InsertSection( sectionData );

    //
    // copy over all allocation within the section
    //
    if ( traverse != NULL )
    {
        DecalareAllocationFromTraverse( memorySpace, traverse );
    }

    //
    // service itself, so radMemoryMonitorService( ) doesn't needs to be called all the time.
    //
    Service( );
}

//===========================================================================
// radMemoryMonitorClient::RescindSection
//===========================================================================
// Description: Rescind a section of memory
//
// Constraints: Not yet
//
// Parameters:  void* address - starting position of the section
//              MemorySectionType sectionType - section type id
//===========================================================================
void radMemoryMonitorClient::RescindSection( void* address, radMemorySpace memorySpace )
{
    radSingleLock< radMemoryMonitorClient > singleLock( this, true );
    Service( );

    rAssertMsg( m_eInitialized == MM_Initialized, "radMemoryMonitor not initialized." );

    if ( m_bOverRunReported == true )
    {
        return;
    }

    MM_RescindSectionData * pRescindSectionData = NULL;
    AllocateMemoryForSendProtocal( & pRescindSectionData );

    pRescindSectionData->eventID        = radPlatformEndian( m_uCurrEventID );   m_uCurrEventID++;
    pRescindSectionData->timeStamp      = radPlatformEndian( GetTimeFrame( ) );
    pRescindSectionData->address        = radPlatformEndian( reinterpret_cast< uintptr_t >( address ) );
    pRescindSectionData->memorySpace    = static_cast< MM_ClientMemorySpace >( radPlatformEndian( ConvertMemSpc2ClientMemSpc( memorySpace ) ) );

    RemoveSection( address, memorySpace );

    InitiateTransmission( );

    //
    // service itself, so radMemoryMonitorService( ) doesn't needs to be called all the time.
    //
    Service( );
}

//===========================================================================
// radMemoryMonitorClient::IdentifySection
//===========================================================================
// Description: Identify memory section
//
// Constraints: Not yet
//
// Parameters:  void* address - starting position of the section
//              const char * name - name of the section
//              radMemorySpace memorySpace - memory space id
//===========================================================================
void radMemoryMonitorClient::IdentifySection( void* address, const char* name, radMemorySpace memorySpace )
{
    radSingleLock< radMemoryMonitorClient > singleLock( this, true );
    Service( );

    rAssertMsg( m_eInitialized == MM_Initialized, "radMemoryMonitor not initialized." );

    if ( m_bOverRunReported == true )
    {
        return;
    }

    MM_IdenitfySectionData * pIdentifySectionData = NULL;
    AllocateMemoryForSendProtocal( & pIdentifySectionData );

    pIdentifySectionData->eventID   = radPlatformEndian( m_uCurrEventID );   m_uCurrEventID++;
    pIdentifySectionData->timeStamp = radPlatformEndian( GetTimeFrame( ) );
    pIdentifySectionData->address   = radPlatformEndian( reinterpret_cast< uintptr_t >( address ) );
    pIdentifySectionData->memorySpace    = static_cast< MM_ClientMemorySpace >( radPlatformEndian32( ConvertMemSpc2ClientMemSpc( memorySpace ) ) );
    
    if ( name != NULL )
    {
        strcpy( pIdentifySectionData->name, name );
    }
    else
    {
        strcpy( pIdentifySectionData->name, "" );
    }

    NameSection( address, memorySpace, name );

    InitiateTransmission( );

    //
    // service itself, so radMemoryMonitorService( ) doesn't needs to be called all the time.
    //
    Service( );
}

//===========================================================================
// radMemoryMonitorClient::DeclareAllocation
//===========================================================================
// Description: Declare allocation of memory
//
// Constraints: Not yet
//
// Parameters:  void* address - where the allocation of memory take place
//              unsigned int size - size of the allocation in bytes
//              unsigned int callStackDepth - depth of the call stack for this allocation
//              radMemorySpace memorySpace - memory space type where this allocation take plaec
//===========================================================================
void radMemoryMonitorClient::DeclareAllocation( void* address, unsigned int size,
                                unsigned int callStackDepth, radMemorySpace memorySpace )
{
    rAssert( reinterpret_cast< uintptr_t >( address ) != 0x1d0228a );

    radSingleLock< radMemoryMonitorClient > singleLock( this, true );
    Service( );
    // rDebugPrintf( "Declare: [0x%x] Size: [0x%x] Space: [0x%x]\n", address, size, memorySpace );

    if ( address == NULL )
    {
        return;
    }

    if ( m_bOverRunReported == true )
    {
        return;
    }

    if ( m_eInitialized != MM_Initialized )
    {
        return;
    }

    MM_DeclareAllocationData * pDeclareAllocationData = NULL;
    AllocateMemoryForSendProtocal( & pDeclareAllocationData );

    pDeclareAllocationData->eventID         = radPlatformEndian( m_uCurrEventID );   m_uCurrEventID++;
    pDeclareAllocationData->timeStamp       = radPlatformEndian( GetTimeFrame( ) );
    pDeclareAllocationData->address         = radPlatformEndian( reinterpret_cast< uintptr_t >( address ) );
    pDeclareAllocationData->size            = radPlatformEndian( size );
    pDeclareAllocationData->callStackDepth  = radPlatformEndian( callStackDepth );
    pDeclareAllocationData->memorySpace     = static_cast< MM_ClientMemorySpace >( radPlatformEndian( ConvertMemSpc2ClientMemSpc( memorySpace ) ) );
    
    if ( callStackDepth > MM_MAX_CALLSTACK_DEPTH )
    {
        callStackDepth = MM_MAX_CALLSTACK_DEPTH;
    }

    uintptr_t uCallStack[ MM_MAX_CALLSTACK_DEPTH + 2 ] = { 0 };

    // copy call stack over to the buffer
    radStackTraceGet( uCallStack, callStackDepth + 2 );
    
    for ( unsigned int i = 0; i < callStackDepth; i ++ )
    {
    	pDeclareAllocationData->callStack[ i ] = radPlatformEndian( uCallStack[ i + 2 ] );
    }

    InitiateTransmission( );

    //
    // service itself, so radMemoryMonitorService( ) doesn't needs to be called all the time.
    //
    Service( );
}

//===========================================================================
// radMemoryMonitorClient::RescindAllocation
//===========================================================================
// Description: Rescind allocation of memory
//
// Constraints: Not yet
//
// Parameters:  void* address - where the allocaton occurs
//              radMemorySpace memorySpace - memory space type
//===========================================================================
void radMemoryMonitorClient::RescindAllocation( void* address, radMemorySpace memorySpace )
{
    radSingleLock< radMemoryMonitorClient > singleLock( this, true );
    Service( );

    // rDebugPrintf( "Rescind: [0x%x] Space: [0x%x]\n", address, memorySpace );

    rAssertMsg( m_eInitialized == MM_Initialized, "radMemoryMonitor not initialized." );

    if ( address == NULL )
    {
        return;
    }

    if ( m_bOverRunReported == true )
    {
        return;
    }

    MM_RescindAllocationData * pRescindAllocationData = NULL;
    AllocateMemoryForSendProtocal( & pRescindAllocationData );

    pRescindAllocationData->eventID         = radPlatformEndian( m_uCurrEventID );   m_uCurrEventID++;
    pRescindAllocationData->timeStamp       = radPlatformEndian( GetTimeFrame( ) );
    pRescindAllocationData->address         = radPlatformEndian( reinterpret_cast< uintptr_t >( address ) );
    pRescindAllocationData->memorySpace     = static_cast< MM_ClientMemorySpace >( radPlatformEndian( ConvertMemSpc2ClientMemSpc( memorySpace ) ) );

    InitiateTransmission( );

    //
    // service itself, so radMemoryMonitorService( ) doesn't needs to be called all the time.
    //
    Service( );
}

//===========================================================================
// radMemoryMonitorClient::IdentifyAllocation
//===========================================================================
// Description: Identify allocation of memory
//
// Constraints: Not yet
//
// Parameters:  void* address - where the allocaton occurs
//              group - group id of the allocation
//              name - name of the allocation
//              pReferenceCount - reference count of the allocation
//              radMemorySpace memorySpace - memory space type
//===========================================================================
void radMemoryMonitorClient::IdentifyAllocation( void* address, const char * group, const char* name,
                                                 unsigned int* pReferenceCount, radMemorySpace memorySpace )
{
    radSingleLock< radMemoryMonitorClient > singleLock( this, true );
    Service( );

    // rDebugPrintf( "Identify: [0x%x] Group: [%s] Name: [%s] Space: [0x%x]\n",
    //    address, group, name, memorySpace );

    if ( m_eInitialized != MM_Initialized )
    {
        rAssertMsg( m_eInitialized == MM_Initialized, "radMemoryMonitor not initialized." );
        return;
    }

    if ( m_bOverRunReported == true )
    {
        return;
    }

    MM_IdentifyAllocationData * pData = NULL;
    AllocateMemoryForSendProtocal( & pData );

    pData->eventID         = radPlatformEndian( m_uCurrEventID );   m_uCurrEventID++;
    pData->timeStamp       = radPlatformEndian( GetTimeFrame( ) );
    pData->address         = radPlatformEndian( reinterpret_cast< uintptr_t >( address ) );
    pData->memorySpace     = static_cast< MM_ClientMemorySpace >( radPlatformEndian( ConvertMemSpc2ClientMemSpc( memorySpace ) ) );
    if ( pReferenceCount == NULL )
    {
        pData->referenceCount     = 0;
        pData->referenceCountPtr  = 0;
    }
    else
    {
        pData->referenceCount     = radPlatformEndian( * pReferenceCount );
        pData->referenceCountPtr  = radPlatformEndian( pReferenceCount );
    }

    if ( name == NULL )
    {
        strcpy( pData->name, "" );
    }
    else
    {
        strncpy( pData->name, name, sizeof( pData->name ) );
        pData->name[ sizeof( pData->name ) - 1 ] = '\0';
    }

    if ( group == NULL )
    {
        strcpy( pData->group, "" );
    }
    else
    {
        strncpy( pData->group, group, sizeof( pData->group ) );
        pData->group[ sizeof( pData->group ) - 1 ] = '\0';
    }
    InitiateTransmission( );

    //
    // service itself, so radMemoryMonitorService( ) doesn't needs to be called all the time.
    //
    Service( );
}

//===========================================================================
// radMemoryMonitorClient::IssueFlag
//===========================================================================
// Description: Issue a flag associated with name
//
// Constraints: Not yet
//
// Parameters:  const char* flag - name of the flag
//              unsigned int timeStamp - time stamp for this event
//===========================================================================
void radMemoryMonitorClient::IssueFlag( const char* flag )
{
    radSingleLock< radMemoryMonitorClient > singleLock( this, true );
    Service( );

    rAssertMsg( m_eInitialized == MM_Initialized, "radMemoryMonitor not initialized." );

    if ( m_bOverRunReported == true )
    {
        return;
    }

    MM_IssueFlagData * pIssueFlagData = NULL;
    AllocateMemoryForSendProtocal( & pIssueFlagData );

    rAssertMsg( flag != NULL, "radMemoryMonitor : flag name cannot be NULL." );

    pIssueFlagData->eventID     = radPlatformEndian32( m_uCurrEventID );   m_uCurrEventID++;
    pIssueFlagData->timeStamp   = radPlatformEndian32( GetTimeFrame( ) );
    strncpy( pIssueFlagData->name, flag, sizeof( pIssueFlagData->name ) );
    pIssueFlagData->name[ sizeof( pIssueFlagData->name ) - 1 ] = '\0';

    InitiateTransmission( );

    Service( );
}

//===========================================================================
// radMemoryMonitorClient::ReportAddRef
//===========================================================================
// Description: Report an object has just AddRef to another object
//
// Constraints: Not yet
//
// Parameters:  void** pReference - the referenced object
//              void* pObject - the object is attempting to release another
//===========================================================================
void radMemoryMonitorClient::ReportAddRef( void* pObject, void* pReference, radMemorySpace memorySpaceObject )
{
    radSingleLock< radMemoryMonitorClient > singleLock( this, true );
    Service( );

    rAssertMsg( m_eInitialized == MM_Initialized, "radMemoryMonitor not initialized." );

	//
	// if initalized not to use refcount data, don't report it.
	//
    if ( ! m_bReportRefCountMode )
    {
        return;
    }

    if ( m_bOverRunReported == true )
    {
        return;
    }

    unsigned int uObjectPtr = 0;
    if ( pObject != NULL )
    {
        uObjectPtr  = reinterpret_cast< uintptr_t >( pObject );
    }

    MM_ReportAddRefData * pReportAddRefData = NULL;
    AllocateMemoryForSendProtocal( & pReportAddRefData );

    pReportAddRefData->eventID      = radPlatformEndian( m_uCurrEventID );   m_uCurrEventID++;
    pReportAddRefData->timeStamp    = radPlatformEndian( GetTimeFrame( ) );
    pReportAddRefData->memorySpaceRefObject = static_cast< MM_ClientMemorySpace >( radPlatformEndian( ConvertMemSpc2ClientMemSpc( memorySpaceObject ) ) );
    pReportAddRefData->refObject    = radPlatformEndian( reinterpret_cast< uintptr_t >( pReference ) );
    pReportAddRefData->memorySpaceObject = static_cast< MM_ClientMemorySpace >( radPlatformEndian( ConvertMemSpc2ClientMemSpc( memorySpaceObject ) ) );
    pReportAddRefData->object       = radPlatformEndian( uObjectPtr );

    pReportAddRefData->callStackDepth = radPlatformEndian( MM_MAX_CALLSTACK_DEPTH );
    //
    // get call stack from addref, and see who called addref
    //
    uintptr_t uCallStack[ MM_MAX_CALLSTACK_DEPTH + 2 ] = { 0 };

    // copy call stack over to the buffer, 2 offset is used so we don't include radMemoryMonitorXxxx functions
    radStackTraceGet( uCallStack, MM_MAX_CALLSTACK_DEPTH + 2 );

    for ( unsigned int i = 0; i < MM_MAX_CALLSTACK_DEPTH; i ++ )
    {
    	pReportAddRefData->callStack[ i ] = radPlatformEndian( uCallStack[ i + 2 ] );
    }

    InitiateTransmission( );

    //
    // service itself, so radMemoryMonitorService( ) doesn't needs to be called all the time.
    //
    Service( );
}

//===========================================================================
// radMemoryMonitorClient::ReportRelease
//===========================================================================
// Description: Report an object has just release reference to another object
//
// Constraints: Not yet
//
// Parameters:  void** pReference - the referenced object
//              void* pObject - the object is attempting to release another
//===========================================================================
void radMemoryMonitorClient::ReportRelease( void* pObject, void* pReference, radMemorySpace memorySpaceObject )
{
    radSingleLock< radMemoryMonitorClient > singleLock( this, true );
    Service( );

    rAssertMsg( m_eInitialized == MM_Initialized, "radMemoryMonitor not initialized." );

	//
	// if initalized not to use refcount data, don't report it.
	//
    if ( ! m_bReportRefCountMode )
    {
        return;
    }

    if ( m_bOverRunReported == true )
    {
        return;
    }

    unsigned int uObjectPtr = 0;
    if ( pObject != NULL )
    {
        uObjectPtr  = reinterpret_cast< uintptr_t >( pObject );
    }

    //
    // Check if buffer contain addref. If does, remove it.
    //
    if ( RemoveCorrespondingAddRefInBuffer( pObject, pReference, memorySpaceObject ) )
    {
        return;
    }

    MM_ReportReleaseData * pReportReleaseData = NULL;
    AllocateMemoryForSendProtocal( & pReportReleaseData );

    pReportReleaseData->eventID      = radPlatformEndian( m_uCurrEventID );   m_uCurrEventID++;
    pReportReleaseData->timeStamp    = radPlatformEndian( GetTimeFrame( ) );
    pReportReleaseData->memorySpaceRefObject = static_cast< MM_ClientMemorySpace >( radPlatformEndian( ConvertMemSpc2ClientMemSpc( memorySpaceObject ) ) );
    pReportReleaseData->refObject    = radPlatformEndian( reinterpret_cast< uintptr_t >( pReference ) );
    pReportReleaseData->memorySpaceObject = static_cast< MM_ClientMemorySpace >( radPlatformEndian( ConvertMemSpc2ClientMemSpc( memorySpaceObject ) ) );
    pReportReleaseData->object       = radPlatformEndian( uObjectPtr );
    pReportReleaseData->callStackDepth = radPlatformEndian( MM_MAX_CALLSTACK_DEPTH );

    //
    // get call stack from addref, and see who called addref
    //
    uintptr_t uCallStack[ MM_MAX_CALLSTACK_DEPTH + 2 ] = { 0 };

    // copy call stack over to the buffer
    radStackTraceGet( uCallStack, MM_MAX_CALLSTACK_DEPTH + 2 );

    for ( unsigned int i = 0; i < MM_MAX_CALLSTACK_DEPTH; i ++ )
    {
    	pReportReleaseData->callStack[ i ] = radPlatformEndian( uCallStack[ i + 2 ] );
    }

    InitiateTransmission( );

    //
    // service itself, so radMemoryMonitorService( ) doesn't needs to be called all the time.
    //
    Service( );
}

//===========================================================================
// radMemoryMonitorClient::SetStackInitialFillChar
//===========================================================================
// Description: 
//
// Constraints: Not yet
//
// Parameters:  
//===========================================================================
void radMemoryMonitorClient::SetStackInitialFillChar( char c )
{
    radSingleLock< radMemoryMonitorClient > singleLock( this, true );
    m_cStackInitFillChar = c;
}

//===========================================================================
// radMemoryMonitorClient::OnStatusChange
//===========================================================================
// Description: Callback when client status changes
//
// Constraints: Not yet
//
// Parameters:  IRadDbgComChannel::ConnectionState connectionState
//              const char* Message
//
//===========================================================================
void radMemoryMonitorClient::OnStatusChange( IRadDbgComChannel::ConnectionState connectionState,
                             const char* Message )
{
    radSingleLock< radMemoryMonitorClient > singleLock( this, true );
    //
    // Switch on status message.
    //  
    switch( connectionState )
    {
        case IRadDbgComChannel::Attached :
        {
            m_uAttachedCount ++;
            //
            // Simply issue a receive to wait for data.
            //
            m_pChannel->ReceiveAsync( m_pRxBuffer, sizeof( m_pRxBuffer ), this );

            m_bAttached = true;

            rReleaseString( "Memory monitor Attached\n");

            ReSyncDataIfNeeded( );

            InitiateTransmission( );

            m_bAttemptingConnect = false;

            m_bOverRunReported = false;

            m_bSendSuspendedState = true;       // send suspended state

            break;
        }

        case IRadDbgComChannel::Detached :
        {   
            //
            // Set flag inidicating not attached. Reset transmitter
            //
            m_bAttached = false;

            m_uRxBytesQueued = 0;

            m_uRxProcessBufferQueued = 0;

            m_uRxBytesProcessed = 0;

            m_uTxBytesQueued = 0;

            m_bBuffer1Active = true;

            m_bTxOutstanding = false;

            m_bAttemptingConnect = false;

            rReleaseString( "Memory monitor detached [");
            rReleaseString( Message );
            rReleaseString( "].\n" );
            break;
        }
    
        case IRadDbgComChannel::Attaching :
        {
            //
            // If we are attached. Print message that we lost communication.
            //
            if( m_bAttached )
            {
                m_bAttached = false;

                rReleaseString( "Warning: Re-attaching memory monitor to the host.\n");
            }

            break;
        }

        default:
        {
            break;
        }
    }
}

//===========================================================================
// radMemoryMonitorClient::OnReceiveComplete
//===========================================================================
// Description: Callback when data receive is completed
//
// Constraints: Not yet
//
// Parameters:  bool successful - true if data receive is completed successful
//              unsigned int bytesReceived - number of bytes received
//===========================================================================
void radMemoryMonitorClient::OnReceiveComplete( bool successful, unsigned int bytesReceived )
{
    radSingleLock< radMemoryMonitorClient > singleLock( this, true );
    //========================================================================
    // Make sure nothing here calls radDbgComService( );
    //========================================================================
    if( successful )
    {
        //====================================================================
        // read the data and put into m_pRxProcessBuffer. Wait until Service
        // gets called then process the data.
        //====================================================================

        m_uRxBytesQueued += bytesReceived;
        m_uRxBytesProcessed = 0;

        PreProcessRxData( );

        //
        // calculate how many bytes is in the process buffer
        // and copy memory over for later process.
        //
        rAssert( m_uRxProcessBufferQueued == 0 );
        m_uRxProcessBufferQueued = bytesReceived - m_uRxBytesQueued;
        memcpy( m_pRxProcessBuffer, m_pRxBuffer, m_uRxProcessBufferQueued );

        //
        // reset the reciever buffer
        //
        if ( m_uRxBytesQueued > 0 )
        {
            memcpy( m_pRxBuffer, ( m_pRxBuffer + m_uRxBytesProcessed ), m_uRxBytesQueued );
        }

        memset( ( m_pRxBuffer + m_uRxBytesQueued ), 0, sizeof( m_pRxBuffer ) - m_uRxBytesQueued );

        //
        // Re-issue the receive buffer.
        //
        m_pChannel->ReceiveAsync( m_pRxBuffer + m_uRxBytesQueued, sizeof( m_pRxBuffer ) - m_uRxBytesQueued, this );
    }
}

//===========================================================================
// radMemoryMonitorClient::OnSendComplete
//===========================================================================
// Description: Callback when data send is completed
//
// Constraints: Not yet
//
// Parameters:  bool successful - true if data sending is completed successful
//
//===========================================================================
void radMemoryMonitorClient::OnSendComplete( bool successful )
{
    radSingleLock< radMemoryMonitorClient > singleLock( this, true );
    //========================================================================
    // Make sure nothing here calls radDbgComService( );
    //========================================================================
    m_bTxOutstanding = false;

    if( successful )
    {
        // clear the memory buffer for the just transmitted buffer
        memset( ( ! m_bBuffer1Active ) ? m_pTxBuffer1 : m_pTxBuffer2, 0, m_uTxBufferSize );
        //
        // Initiate send possible,
        //
        InitiateTransmission( );
    }
}
#endif // RADMEMORYMONITOR

//===========================================================================
// radMemoryMonitorGetPacketSize
//===========================================================================
// Description: Get packet size
//
// Constraints: n/a
//
// Parameters: const MM_DataHeader header - packet header type
//
// return:  size of the packet
//
//===========================================================================
unsigned int radMemoryMonitorGetPacketSize( const MM_DataHeader header )
{
    switch( header )
    {
        case RequestSuspendStateType:
        case DummyType:
        {
            return sizeof( MM_DataPacket );
        }
        break;

        case DeclarePlatformType:
        {
            return sizeof( MM_DeclarePlatformData );
        }
        break;

        case DeclareSpaceDataType:
        {
            return sizeof( MM_DeclareSpaceData );
        }
        break;

        case DeclareSectionType:
        {
            return sizeof( MM_DeclareSectionData );
        }
        break;

        case RescindSectionType:
        {
            return sizeof( MM_RescindSectionData );
        }
        break;

        case IdentifySectionType:
        {
            return sizeof( MM_IdenitfySectionData );
        }
        break;

        case DeclareAllocationType:
        {
            return sizeof( MM_DeclareAllocationData );
        }
        break;

        case RescindAllocationType:
        {
            return sizeof( MM_RescindAllocationData );
        }
        break;

        case IdentifyAllocationType:
        {
            return sizeof( MM_IdentifyAllocationData );
        }
        break;
        
        case IssueFlagType:
        {
            return sizeof( MM_IssueFlagData );
        }
        break;
        
        case ReportAddRefType:
        {
            return sizeof( MM_ReportAddRefData );
        }
        break;
        
        case ReportReleaseType:
        {
            return sizeof( MM_ReportReleaseData );
        }
        break;
        
        case RequestMemoryBlockType:
        {
            return sizeof( MM_RequestMemoryBlockData );
        }
        break;
        
        case RequestResumeType:
        {
            return sizeof( MM_RequestResumedData );
        }
        break;

        case RequestSuspendType:
        {
            return sizeof( MM_RequestSuspendedData );
        }
        break;
        
        case RequestMemoryTableType:
        {
            return sizeof( MM_RequestSuspendedData );
        }
        break;

        case AcknowledgementType:
        {
            return sizeof( MM_Acknowledgement );
        }
        break;

        case ReportSuspendStateType:
        {
            return sizeof( MM_ReportSuspend );
        }
        break;

        case MemoryBlockType:
        {
            return sizeof( MM_SendMemoryBlockData );
        }
        break;

        case ReportStackUsageType:
        {
            return sizeof( MM_ReportStackUsageData );
        }
        break;

        case RequestStackUsageType:
        {
            return sizeof( MM_RequestStackUsageData );
        }
        break;

        case RequestRefCountType:
        {
            return sizeof( MM_RequestRefCountData );
        }
        break;

        case ReportRefCountType:
        {
            return sizeof( MM_ReportRefCountData );
        }
        break;

        default:
        {
            rAssert( false );
        }
        break;
    }
    return 0;
}