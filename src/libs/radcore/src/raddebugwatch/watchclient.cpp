//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        watchclient.cpp
//
// Subsystem:   Foundation Technologies - Debug Watch Client
//
// Description: This file contains the implementation of the Foundation 
//              Technologies debug watch point client
//
// Date:        Jan 18, 2001
//
//=============================================================================

//=============================================================================
// Include Files
//=============================================================================

#include "pch.hpp"
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <raddebug.hpp>
#include <radplatform.hpp>
#include <radmemorymonitor.hpp>
#include "watchclient.hpp"

//=============================================================================
// Local Defintions
//=============================================================================

//=============================================================================
// Static Data Defintions
//=============================================================================

static DebugWatchClient*  s_theDebugWatchClient = NULL;

//=============================================================================
// Function:    radDebugWatchInitialize
//=============================================================================
// Description: Use to initialize this subsystem.
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

void radDebugWatchInitialize
( 
    const char* pNameSpace, 
    unsigned int internalBufferSize, 
    radMemoryAllocator alloc
)
{
    //
    // Assert that this subsystem has not allready been initialized.
    //
    rAssertMsg( s_theDebugWatchClient == NULL, "Debug Watch System already initialized");
    
    //
    // Construct the object.
    //
    s_theDebugWatchClient = new( alloc ) DebugWatchClient( alloc );
        
    s_theDebugWatchClient->Initialize( pNameSpace, internalBufferSize );
}

//=============================================================================
// Function:    radDebugWatchTerminate
//=============================================================================
// Description: Used to terminate this subsystem.
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

void radDebugWatchTerminate( void )
{
    rAssertMsg( s_theDebugWatchClient != NULL, "Debug Watch System not initialized");

    s_theDebugWatchClient->Terminate( );

    radRelease( static_cast< IRadDebugWatch* >( s_theDebugWatchClient ), NULL );

    rAssertMsg( s_theDebugWatchClient == NULL, "Debug Watch System is still in use by someone");
}

//=============================================================================
// Function:    radDebugWatchGet
//=============================================================================
// Description: Used to obtain the singleton object.
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

IRadDebugWatch* radDebugWatchGet( void )
{
    rAssertMsg( s_theDebugWatchClient != NULL, "Debug Watch System not initialized");
    
    return( s_theDebugWatchClient );
}

bool radDebugWatchEnabled()
{
    return ( s_theDebugWatchClient != NULL );
}

//=============================================================================
// Public Member Functions
//=============================================================================

//=============================================================================
// Function:    DebugWatchClient::DebugWatchClient
//=============================================================================
// Description: Constructor for the client object.
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

DebugWatchClient::DebugWatchClient( radMemoryAllocator alloc  )
    :
    m_ReferenceCount( 1 ),
    m_TxBuffer1( NULL ),
    m_TxBuffer2( NULL ),
    m_TxBytesQueued( 0 ),
    m_Buffer1Active( true ),
    m_OverflowReported( false ),
    m_Attached( false ),
    m_Allocator( alloc )
{
    radMemoryMonitorIdentifyAllocation( this, g_nameFTech, "DebugWatchClient" );
    //
    // Get communication channel
    //
    radDbgComTargetCreateChannel( WATCH_PROTOCOL, 
                                  &m_Channel, alloc );  
    
    rAssertMsg( m_Channel != NULL, "Debug Watch Failed: Make sure debug communication system initialized\n");

    m_Channel->RegisterStatusCallback( this );
}

//=============================================================================
// Function:    DebugWatchClient::~DebugWatchClient
//=============================================================================
// Description: Destructor for the server object. 
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

DebugWatchClient::~DebugWatchClient( void )
{
    s_theDebugWatchClient = NULL;

    m_Channel->RegisterStatusCallback( NULL );

    radRelease( m_Channel, this );

    radMemoryFree( m_TxBuffer1 );
    radMemoryFree( m_TxBuffer2 );
}            

//=============================================================================
// Function:    DebugWatchClient::Initialize
//=============================================================================
// Description: Initialize the system. Must frame all other calls.
//
// Parameters:  Default name space.
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

void DebugWatchClient::Initialize
(
    const char*     pNameSpace,
    unsigned int    internalBufferSize
)
{
    //
    // Make sure the reference count is 1. If not caller has not framed
    // things correctly.
    //
    rAssert( m_ReferenceCount == 1 );

    //
    // Lets allocate new buffers for transmission. We use a double buffer scheme.
    //
    rAssert( m_TxBuffer1 == NULL );         // Make sure not already initialized.   

    m_TxBuffer1 = (unsigned char*) radMemoryAlloc( m_Allocator, internalBufferSize );
    rAssert( m_TxBuffer1 != NULL );

    m_TxBuffer2 = (unsigned char*) radMemoryAlloc( m_Allocator, internalBufferSize );
    rAssert( m_TxBuffer2 != NULL );


    //
    // Initialize transmit buffer state variables.
    //
    m_CurrentTxAddress = m_TxBuffer1;
    m_TxBufferSize = internalBufferSize;
    m_TxOutstanding = false;

    //
    // Attach to the host.
    //
    m_Channel->Attach( );

    //
    // First thing we do is send the default name space to the host. Make sure does not exceed
    // our limit.
    //
    rAssert( strlen( pNameSpace ) < WP_MAX_NAMESPACELENGTH );

    //
    // Check if space. If no room, error reported and data lost.
    //
    if( IsThereTxRoom( sizeof( WCNameSpace ) ) )
    {
        //
        // Lets move the data ito the transmit buffer, update state and try to 
        // initiate the send.
        //
        QueueNameSpace( pNameSpace );

        InitiateTransmission( );
    }
}

//=============================================================================
// Function:    DebugWatchClient::Terminate
//=============================================================================
// Description: Terminate the watch system
//
// Parameters:  
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

void DebugWatchClient::Terminate( void )
{
    m_Channel->Detach( );
}

//=============================================================================
// Function:    DebugWatchClient::Delete
//=============================================================================
// Description: Remove a watch point.
//
// Parameters:  address of member variable.
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

void DebugWatchClient::Delete
(
    void* pValue
)
{
    //
    // Make sure the reference count greater than 1 or system has not been initialized.
    //
    rAssert( m_ReferenceCount >= 1 );

    //
    // Check if space. If no room, error reported and data lost.
    //
    if( IsThereTxRoom( sizeof( WCDelete ) ) )
    {
        //
        // Lets move the data ito the transmit buffer, update state and try to 
        // initiate the send.
        //
        WCDelete* pPacket = (WCDelete*) m_CurrentTxAddress;
        pPacket->m_Command = (WPCommand) radPlatformEndian32( WPCDelete );
        pPacket->m_Address =  pValue ;

        m_CurrentTxAddress += sizeof( WCDelete );
        m_TxBytesQueued += sizeof( WCDelete );

        InitiateTransmission( );
    }
}

//=============================================================================
// Function:    DebugWatchClient::AddUnsignedInt
//=============================================================================
// Description: Add watch point
//
// Parameters:  
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

void DebugWatchClient::AddUnsignedInt
( 
    unsigned int*   pValue,
    const char*     pName,
    const char*     pNameSpace, 
    RADDEBUGWATCH_CALLBACK callback,
    void*           userData,
    unsigned int    min, 
    unsigned int    max,
    bool            readonly
)
{
    WCCreate* pPacket;

    //  
    // Assert that the max and min are ok. Due to issues with the slider controle
    // in the watch server, the range was limitted here. If this presents a problem,
    // we can revisit.
    //
    rAssert( max <= UINT_MAX / 2 );

    if( NULL != (pPacket = DoCreatePacketPreamble( pValue, pName, pNameSpace, callback, userData) ) )
    {
        //
        // Add stuff specific to the type.
        //
        pPacket->m_DataType = (WPDataType) radPlatformEndian32( WPDUnsignedInt );
        pPacket->m_Attrib.m_ui.m_Max = radPlatformEndian32( max );
        pPacket->m_Attrib.m_ui.m_Min = radPlatformEndian32( min );
        pPacket->m_ReadOnly = readonly;
    
        InitiateTransmission( );
    }
}

//=============================================================================
// Function:    DebugWatchClient::AddInt
//=============================================================================
// Description: Add watch point
//
// Parameters:  
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

void DebugWatchClient::AddInt
( 
    int*            pValue,
    const char*     pName,
    const char*     pNameSpace, 
    RADDEBUGWATCH_CALLBACK callback,
    void*           userData,
    int             min, 
    int             max,
    bool            readonly
)
{
    WCCreate* pPacket;

    //  
    // Assert that the max and min are ok. Due to issues with the slider controle
    // in the watch server, the range was limitted here. If this presents a problem,
    // we can revisit.
    //
    rAssert( max <= INT_MAX / 2 );
    rAssert( min >= INT_MIN / 2 );

    if( NULL != (pPacket = DoCreatePacketPreamble( pValue, pName, pNameSpace, callback, userData) ) )
    {
        //
        // Add stuff specific to the type.
        //
        pPacket->m_DataType = (WPDataType) radPlatformEndian32( WPDInt );
        pPacket->m_Attrib.m_i.m_Max = radPlatformEndian32( max );
        pPacket->m_Attrib.m_i.m_Min = radPlatformEndian32( min );
        pPacket->m_ReadOnly = readonly;
    
        InitiateTransmission( );
    }
}

//=============================================================================
// Function:    DebugWatchClient::AddUnsignedShort
//=============================================================================
// Description: Add watch point
//
// Parameters:  
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

void DebugWatchClient::AddUnsignedShort
( 
    unsigned short* pValue,
    const char*     pName,
    const char*     pNameSpace, 
    RADDEBUGWATCH_CALLBACK callback,
    void*           userData,
    unsigned short  min, 
    unsigned short  max,
    bool            readonly
)
{
    WCCreate* pPacket;

    if( NULL != (pPacket = DoCreatePacketPreamble( pValue, pName, pNameSpace, callback, userData) ) )
    {
        //
        // Add stuff specific to the type.
        //
        pPacket->m_DataType = (WPDataType) radPlatformEndian32(WPDUnsignedShort );
        pPacket->m_Attrib.m_us.m_Max = radPlatformEndian16( max );
        pPacket->m_Attrib.m_us.m_Min = radPlatformEndian16( min );
        pPacket->m_ReadOnly = readonly;
    
        InitiateTransmission( );
    }
}

//=============================================================================
// Function:    DebugWatchClient::AddShort
//=============================================================================
// Description: Add watch point
//
// Parameters:  
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

void DebugWatchClient::AddShort
( 
    short*  pValue,
    const char*     pName,
    const char*     pNameSpace, 
    RADDEBUGWATCH_CALLBACK callback,
    void*           userData,
    short           min, 
    short           max,
    bool            readonly
)
{
    WCCreate* pPacket;

    if( NULL != (pPacket = DoCreatePacketPreamble( pValue, pName, pNameSpace, callback, userData) ) )
    {
        //
        // Add stuff specific to the type.
        //
        pPacket->m_DataType = (WPDataType) radPlatformEndian32( WPDShort );
        pPacket->m_Attrib.m_s.m_Max = radPlatformEndian16( max );
        pPacket->m_Attrib.m_s.m_Min = radPlatformEndian16( min );
        pPacket->m_ReadOnly = readonly;
    
        InitiateTransmission( );
    }
}

//=============================================================================
// Function:    DebugWatchClient::AddUnsignedChar
//=============================================================================
// Description: Add watch point
//
// Parameters:  
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

void DebugWatchClient::AddUnsignedChar
( 
    unsigned char*  pValue,
    const char*     pName,
    const char*     pNameSpace, 
    RADDEBUGWATCH_CALLBACK callback,
    void*           userData,
    unsigned char   min, 
    unsigned char   max,
    bool            readonly
)
{
    WCCreate* pPacket;

    if( NULL != (pPacket = DoCreatePacketPreamble( pValue, pName, pNameSpace, callback, userData) ) )
    {
        //
        // Add stuff specific to the type.
        //
        pPacket->m_DataType = (WPDataType) radPlatformEndian32( WPDUnsignedChar );
        pPacket->m_Attrib.m_uc.m_Max = max;
        pPacket->m_Attrib.m_uc.m_Min = min;
        pPacket->m_ReadOnly = readonly;
    
        InitiateTransmission( );
    }
}

//=============================================================================
// Function:    DebugWatchClient::AddChar
//=============================================================================
// Description: Add watch point
//
// Parameters:  
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

void DebugWatchClient::AddChar
( 
    char*   pValue,
    const char*     pName,
    const char*     pNameSpace, 
    RADDEBUGWATCH_CALLBACK callback,
    void*           userData,
    char            min, 
    char            max,
    bool            readonly
)
{
    WCCreate* pPacket;

    if( NULL != (pPacket = DoCreatePacketPreamble( pValue, pName, pNameSpace, callback, userData) ) )
    {
        //
        // Add stuff specific to the type.
        //
        pPacket->m_DataType = (WPDataType) radPlatformEndian32( WPDChar );
        pPacket->m_Attrib.m_c.m_Max = max;
        pPacket->m_Attrib.m_c.m_Min = min;
        pPacket->m_ReadOnly = readonly;
    
        InitiateTransmission( );
    }
}

//=============================================================================
// Function:    DebugWatchClient::AddBoolean
//=============================================================================
// Description: Add watch point
//
// Parameters:  
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

void DebugWatchClient::AddBoolean
( 
    bool*           pValue,
    const char*     pName,
    const char*     pNameSpace,
    RADDEBUGWATCH_CALLBACK callback,
    void*           userData,
    bool            readonly
)
{
    WCCreate* pPacket;

    if( NULL != (pPacket = DoCreatePacketPreamble( pValue, pName, pNameSpace, callback, userData) ) )
    {
        //
        // Add stuff specific to the type.
        //
        pPacket->m_DataType = (WPDataType) radPlatformEndian32( WPDBoolean );
        pPacket->m_ReadOnly = readonly;
    
        InitiateTransmission( );
    }
}

//=============================================================================
// Function:    DebugWatchClient::AddFloat
//=============================================================================
// Description: Add watch point
//
// Parameters:  
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

void DebugWatchClient::AddFloat
( 
    float*  pValue,
    const char*     pName,
    const char*     pNameSpace, 
    RADDEBUGWATCH_CALLBACK callback,
    void*           userData,
    float           min, 
    float           max,
    bool            readonly
)
{
    WCCreate* pPacket;

    if( NULL != (pPacket = DoCreatePacketPreamble( pValue, pName, pNameSpace, callback, userData) ) )
    {
        //
        // Add stuff specific to the type.
        //
        pPacket->m_DataType = (WPDataType) radPlatformEndian32( WPDFloat );
        pPacket->m_Attrib.m_f.m_Max = radPlatformEndianFloat( max );
        pPacket->m_Attrib.m_f.m_Min = radPlatformEndianFloat( min );
        pPacket->m_ReadOnly = readonly;
    
        InitiateTransmission( );
    }
}

//=============================================================================
// Function:    DebugWatchClient::AddFunction
//=============================================================================
// Description: Add function
//
// Parameters:  
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

void DebugWatchClient::AddFunction
( 
    const char* pName, 
    const char* pNameSpace, 
    RADDEBUGWATCH_CALLBACK callback, 
    void* userData
)
{
    WCCreate* pPacket;
    
    rAssert( callback != NULL );
   
    //  
    // To keep things simple, pass the function as the value
    //
    if( NULL != (pPacket = DoCreatePacketPreamble( (void*) callback, pName, pNameSpace, callback, userData) ) )
    {
        //
        // Add stuff specific to the type.
        //
        pPacket->m_DataType = (WPDataType) radPlatformEndian32( WPDFunction );
    
        InitiateTransmission( );
    }
}

//=============================================================================
// Function:    DebugWatchClient::AddVector
//=============================================================================
// Description: Add watch point
//
// Parameters:  
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

void DebugWatchClient::AddVector
( 
    float*  pValue,
    const char*     pName,
    const char*     pNameSpace, 
    RADDEBUGWATCH_CALLBACK callback,
    void*           userData,
    float           min, 
    float           max,
    bool            readonly
)
{
    WCCreate* pPacket;

    if( NULL != (pPacket = DoCreatePacketPreamble( pValue, pName, pNameSpace, callback, userData) ) )
    {
        //
        // Add stuff specific to the type.
        //
        pPacket->m_DataType = (WPDataType) radPlatformEndian32(WPDVector);
        pPacket->m_Attrib.m_vector.m_Max = radPlatformEndianFloat( max );
        pPacket->m_Attrib.m_vector.m_Min = radPlatformEndianFloat( min );
        pPacket->m_ReadOnly = readonly;
    
        InitiateTransmission( );
    }
}

//=============================================================================
// Function:    DebugWatchClient::AddSting
//=============================================================================
// Description: Add watch point
//
// Parameters:  
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

void DebugWatchClient::AddString
( 
    char*           pValue,
    unsigned int    stringLength,
    const char*     pName,
    const char*     pNameSpace,
    RADDEBUGWATCH_CALLBACK callback,
    void*           userData,
    bool            readonly
)
{
    WCCreate* pPacket;

    if( NULL != (pPacket = DoCreatePacketPreamble( pValue, pName, pNameSpace, callback, userData) ) )
    {
        //
        // Add stuff specific to the type.
        //
        pPacket->m_DataType = (WPDataType) radPlatformEndian32( WPDString );
        pPacket->m_Attrib.m_string.m_Length = radPlatformEndian32( stringLength );
        pPacket->m_ReadOnly = readonly;
    
        InitiateTransmission( );
    }
}

//=============================================================================
// Function:    DebugWatchClient::AddEnumeration
//=============================================================================
// Description: Add watch point
//
// Parameters:  
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

void DebugWatchClient::AddEnumeration
( 
    int*                        pValue,
    const EnumerationElement*   pEnumerationInfo,
    unsigned int                numberOfElements, 
    const char*     pName, 
    const char*     pNameSpace,
    RADDEBUGWATCH_CALLBACK callback,
    void*           userData,
    bool            readonly
)
{
    //
    // Make sure the reference count greater than 1 or system has not been initialized.
    //
    rAssert( m_ReferenceCount >= 1 );
    rAssert( numberOfElements != 0 );

    //
    // Lets calculate the size amount of data in the enuration info. It is string - int pairs
    // followed by a null char.
    //
    unsigned int enumInfoLength = 1;
    unsigned int i;
    for( i = 0 ; i < numberOfElements; i++ )
    {
        enumInfoLength += (strlen( pEnumerationInfo[i].m_Name) + 1 + sizeof( int ) );
    }
    
    //
    // Make sure we keep things aligned to 4 bytes.
    //
    if(( enumInfoLength & 0x03 ) != 0 )
    {
        enumInfoLength = (enumInfoLength & 0xfffffffc) + 0x4;        
    }    

    //
    // If name space specified, make sure room. We send it in a seperate packet proceding
    // the regular packet.
    //
    rAssert( strlen( pName ) < WP_MAX_NAMELENGTH );
    
    if( pNameSpace != NULL )
    {
        rAssert( strlen( pNameSpace ) < WP_MAX_NAMESPACELENGTH );
                
        if( !IsThereTxRoom( sizeof( WCNameSpace ) + sizeof( WCCreateEnum ) + enumInfoLength ) )
        {
            return;
        }
        QueueNameSpace( pNameSpace );
    }
    else
    {
        //
        // Make sure room for the create enum packet.
        //
        if( !IsThereTxRoom( sizeof( WCCreateEnum ) + enumInfoLength ) )
        {
            return;
        }
    }

    //
    // Lets move the command create data into the transmit buffer
    //
    WCCreateEnum* pPacket = (WCCreateEnum*) m_CurrentTxAddress;
    pPacket->m_Command = (WPCommand) radPlatformEndian32( WPCCreateEnum );
    strcpy( (char*) pPacket->m_Name, pName );
    pPacket->m_Address = pValue;
    pPacket->m_Callback = callback;
    pPacket->m_UserData = userData;
    pPacket->m_Length = radPlatformEndian32( enumInfoLength );
    pPacket->m_ReadOnly = readonly;

    m_CurrentTxAddress += sizeof( WCCreateEnum );
    m_TxBytesQueued += sizeof( WCCreateEnum );
   
    char* txAddress = (char*) m_CurrentTxAddress;
 
    //
    // Copy the data into the current tx address.
    //
    for( i = 0 ; i < numberOfElements; i++ )
    {
        strcpy( txAddress, pEnumerationInfo[i].m_Name);
        txAddress += strlen( pEnumerationInfo[i].m_Name) + 1;
        
        //
        // Since data may be unaligned, we must perform this asignment using memcpy.
        //            
        memcpy( txAddress, &pEnumerationInfo[i].m_Value, 4 );  
        txAddress += sizeof( int );
    }
    
    *txAddress = '\0';
    txAddress++;

    m_TxBytesQueued += enumInfoLength;
    m_CurrentTxAddress += enumInfoLength;

    InitiateTransmission( );
}

//=============================================================================
// Function:    DebugWatchClient::IssueHostRequest
//=============================================================================
// Description: See data to host.
//
// Parameters:  
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

void DebugWatchClient::IssueHostRequest
( 
    unsigned int Id, 
    void*        pData,
    unsigned int length
)
{
    //
    // Make sure the reference count greater than 1 or system has not been initialized.
    //
    rAssert( m_ReferenceCount >= 1 );

    //
    // Make sure room for the create enum packet.
    //
    if( !IsThereTxRoom( sizeof( WCHostRequest ) + length ) )
    {
        return;
    }

    //
    // Lets move the data into thge tx buffer.
    //
    WCHostRequest* pPacket = (WCHostRequest*) m_CurrentTxAddress;
    pPacket->m_Command = (WPCommand) radPlatformEndian32( WPCHostRequest );
    pPacket->m_Id = radPlatformEndian32(Id );
    pPacket->m_Length = radPlatformEndian32( length );

    m_CurrentTxAddress += sizeof( WCHostRequest );
    m_TxBytesQueued += sizeof( WCHostRequest );
   
    memcpy(m_CurrentTxAddress, pData, length );

    m_CurrentTxAddress += length;
    m_TxBytesQueued += length;

    InitiateTransmission( );
}

//=============================================================================
// Function:    DebugWatchClient::AssociateFile
//=============================================================================
// Description: Associate a file with the name space to save data in.
//
// Parameters:  
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

void DebugWatchClient::AssociateFile
( 
    const char* pNameSpace,
    const char* pFilename,
    FileType    fileType
)
{
    //
    // Make sure the reference count greater than 1 or system has not been initialized.
    //
    rAssert( m_ReferenceCount >= 1 );

    //
    // Make sure namespace and filename lengths are valid.
    //
    rAssert( strlen( pNameSpace ) < WP_MAX_NAMESPACELENGTH );
    rAssert( strlen( pFilename ) < WP_MAX_FILENAMELENGTH );

    //
    // Make sure room for the packet.
    //
    if( !IsThereTxRoom( sizeof( WCAssociateFile ) ) )
    {
        return;
    }

    //
    // Lets move the data into thge tx buffer.
    //
    WCAssociateFile* pPacket = (WCAssociateFile*) m_CurrentTxAddress;
    pPacket->m_Command = (WPCommand) radPlatformEndian32( WPCAssociateFile );
    pPacket->m_FileType = radPlatformEndian32( fileType );
    strcpy( pPacket->m_NameSpace, pNameSpace );
    strcpy( pPacket->m_Filename, pFilename );

    m_CurrentTxAddress += sizeof( WCAssociateFile );
    m_TxBytesQueued += sizeof( WCAssociateFile );

    InitiateTransmission( );
}

//=============================================================================
// Function:    DebugWatchClient::DoCreatePacketPreamble
//=============================================================================
// Description: Simple helper function.
//
// Parameters:  
//              
// Returns:     N/A
//
// Notes:       Just done 
//------------------------------------------------------------------------------

WCCreate* DebugWatchClient::DoCreatePacketPreamble
( 
    void*           pValue,
    const char*     pName, 
    const char*     pNameSpace,
    RADDEBUGWATCH_CALLBACK callback,
    void*           userData
)
{
    //
    // Make sure the reference count greater than 1 or system has not been initialized.
    //
    rAssert( m_ReferenceCount >= 1 );

    //
    // If name space specified, make sure room. We send it in a seperate packet proceding
    // the regular packet.
    //
    rAssert( strlen( pName ) < WP_MAX_NAMELENGTH );
    
    if( pNameSpace != NULL )
    {
        rAssert( strlen( pNameSpace ) < WP_MAX_NAMESPACELENGTH );
                
        if( !IsThereTxRoom( sizeof( WCNameSpace ) + sizeof( WCCreate ) ) )
        {
            return(NULL);
        }
        QueueNameSpace( pNameSpace );
    }
    else
    {
        //
        // Make sure room for the create packet.
        //
        if( !IsThereTxRoom( sizeof( WCCreate ) ) )
        {
            return(NULL);
        }
    }

    //
    // Lets move the command create data into the transmit buffer
    //
    WCCreate* pPacket = (WCCreate*) m_CurrentTxAddress;
    pPacket->m_Command = (WPCommand) radPlatformEndian32( WPCCreate );
    strcpy( (char*) pPacket->m_Name, pName );
    pPacket->m_Address = pValue;
    pPacket->m_Callback = callback;
    pPacket->m_UserData = userData;

    m_CurrentTxAddress += sizeof( WCCreate );
    m_TxBytesQueued += sizeof( WCCreate );

    return( pPacket );
}

//=============================================================================
// Function:    DebugWatchClient::QueueNameSpace
//=============================================================================
// Description: Simple helper function.
//
// Parameters:  
//              
// Returns:     N/A
//
// Notes:       We have already verified there is space.
//------------------------------------------------------------------------------

void DebugWatchClient::QueueNameSpace( const char* pNameSpace )
{
    WCNameSpace* pPacket = (WCNameSpace*) m_CurrentTxAddress;
    pPacket->m_Command = (WPCommand) radPlatformEndian32( WPCNameSpace );
    strcpy( (char*) pPacket->m_NameSpace, pNameSpace );

    m_CurrentTxAddress += sizeof( WCNameSpace );
    m_TxBytesQueued += sizeof( WCNameSpace );
}

//=============================================================================
// Function:    DebugWatchClient::IsThereTxRoom
//=============================================================================
// Description: Check if room to add next packet. If not report an error if
//              not already done so.
//
// Parameters:  size
//              
// Returns:     true if room.
//
// Notes:
//------------------------------------------------------------------------------

bool DebugWatchClient::IsThereTxRoom( unsigned int size )
{
    //
    // Check if room in the active buffer to add 
    //
    if( (m_TxBytesQueued + size) <= m_TxBufferSize )
    {
        return( true );
    }
    else
    {
        //
        // report error is not already done so.
        //
        if( !m_OverflowReported )
        {
            rDebugString("DebugWatchClient has lost watch point data. Increase buffer size\n");
            m_OverflowReported = true;
        }
        return( false );
    }
}

//=============================================================================
// Function:    DebugWatchClient::OnStatusChange
//=============================================================================
// Description: 
//              
// Parameters:  
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void DebugWatchClient::OnStatusChange
( 
    IRadDbgComChannel::ConnectionState connectionState,
    const char* Message 
)
{
   //
    // Switch on status message.
    //  
    switch( connectionState )
    {
        case IRadDbgComChannel::Attached :
        {   
            //
            // Simply issue a receive to wait for data.
            //
            m_Channel->ReceiveAsync( m_RxBuffer, sizeof( m_RxBuffer ), this );

            m_Attached = true;
            
            InitiateTransmission( );

            break;
        }

        case IRadDbgComChannel::Detached :
        {   
            //
            // Set flag inidicating not attached. Reset transmitter
            //
            m_Attached = false;
   
            m_TxBytesQueued = 0;
            m_CurrentTxAddress = m_TxBuffer1;
            m_Buffer1Active = true;
            m_TxOutstanding = false;

            break;
        }
    
        case IRadDbgComChannel::Attaching :
        {
            //
            // If we are attached. Print message that we lost communication.
            //
            if( m_Attached )
            {
                m_Attached = false;

                m_TxBytesQueued = 0;
                m_CurrentTxAddress = m_TxBuffer1;
                m_Buffer1Active = true;
                m_TxOutstanding = false;
                
                rDebugString( "Warning: Existing watch point data will be lost\n");
            }

            break;
        }

        default:
        {
            break;
        }
    }
}

//=============================================================================
// Function:    DebugWatchClient::OnReceiveComplete
//=============================================================================
// Description: 
//              
// Parameters:  
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void DebugWatchClient::OnReceiveComplete
( 
    bool            successful, 
    unsigned int    bytesReceived
)
{
    if( successful )
    {
        //
        // Here we have received a buffer. Process the data. Simply process each packet
        // of data in the buffer. 
        //
        unsigned char* pRxAddress = m_RxBuffer;

        while( bytesReceived != 0 && IsCompletePacket( bytesReceived, pRxAddress ) )
        {
            //
            // Switch on the command code of the buffer.
            // 
            switch( radPlatformEndian32( *((WPCommand*) pRxAddress) ) )
            {
                //
                // Received request to read a value,
                //
                case WPCRequestValue :
                {
                    WCRequestValue* pCommand = (WCRequestValue*) pRxAddress;

                    //
                    // Update receive pointer and bytes to process.
                    //
                    bytesReceived -= sizeof( WCRequestValue );
                    pRxAddress += sizeof( WCRequestValue );

                    //
                    // Here we have received a request for a data value. Make sure room 
                    // for the reply. If not, just display message and ignore request.
                    //
                    if( !IsThereTxRoom( sizeof( WCValue ) ) )
                    {
                        break;
                    }

                    //
                    // We have room. Get pointer to the tx and set up command data
                    //
                    WCValue* pReply = (WCValue*) m_CurrentTxAddress;
                    pReply->m_Command = (WPCommand) radPlatformEndian32( WPCValue );
                    pReply->m_DataType = pCommand->m_DataType;
                    pReply->m_Address = pCommand->m_Address;

                    //
                    // Move the various data types approiately.
                    //
                    switch( radPlatformEndian32( pCommand->m_DataType ) )
                    {
                        case WPDUnsignedInt :
                        {
                            pReply->m_Attrib.m_ui.m_Value = radPlatformEndian32( *((unsigned int*)(pCommand->m_Address)) );
                            break;
                        }

                        case WPDInt :
                        {
                            pReply->m_Attrib.m_i.m_Value = radPlatformEndian32( *((int*)(pCommand->m_Address)) );
                            break;
                        }

                        case WPDUnsignedShort :
                        {
                            pReply->m_Attrib.m_us.m_Value = radPlatformEndian16( *((unsigned short*)(pCommand->m_Address)) );
                            break;
                        }

                        case WPDShort :
                        {
                            pReply->m_Attrib.m_s.m_Value = radPlatformEndian16( *((short*)(pCommand->m_Address)) );
                            break;
                        }

                        case WPDUnsignedChar :
                        {
                            pReply->m_Attrib.m_uc.m_Value = *((unsigned char*)(pCommand->m_Address));
                            break;
                        }

                        case WPDChar :
                        {
                            pReply->m_Attrib.m_c.m_Value = *((char*)(pCommand->m_Address));
                            break;
                        }

                        case WPDBoolean :
                        {
                            pReply->m_Attrib.m_bool.m_Value = radPlatformEndian32( *((bool*)(pCommand->m_Address)) );
                            break;
                        }

                        case WPDFloat :
                        {
                            pReply->m_Attrib.m_f.m_Value = radPlatformEndianFloat( *((float*)(pCommand->m_Address)));
                            break;
                        }

                        case WPDEnum :
                        {
                            pReply->m_Attrib.m_enum.m_Value = *((int*)(pCommand->m_Address));
                            break;
                        }

                        case WPDVector :
                        {
                            pReply->m_Attrib.m_vector.m_x = radPlatformEndianFloat( *((float*)(pCommand->m_Address) + 0) );
                            pReply->m_Attrib.m_vector.m_y = radPlatformEndianFloat(*((float*)(pCommand->m_Address) + 1) );
                            pReply->m_Attrib.m_vector.m_z = radPlatformEndianFloat(*((float*)(pCommand->m_Address) + 2) );
                            break;
                        }

                        case WPDString :
                        {
                            //
                            // Make sure we have room.
                            //
                            unsigned int length = strlen((const char*) pCommand->m_Address) + 1;
                            
                            //
                            // Lets keep things align to 4
                            //
                            if(( length & 0x03 ) != 0 )
                            {
                                length = (length & 0xfffffffc) + 0x4;        
                            }    
 
                            if( !IsThereTxRoom( sizeof( WCValue ) ) )
                            {
                                continue;
                            }
                            strcpy( (char*) m_CurrentTxAddress + sizeof( WCValue ), (const char*) pCommand->m_Address );
                            m_CurrentTxAddress += length;
                            m_TxBytesQueued += length;
                            pReply->m_Attrib.m_string.m_Length = radPlatformEndian32(length );
                            break;
                        }

                        default:
                        {
                            rAssertMsg( false, "Bad packet received from target");
                        }
                    }
    
                    //
                    // Update transmit address and send the reply.
                    //      
                    m_CurrentTxAddress += sizeof( WCValue );
                    m_TxBytesQueued += sizeof( WCValue );

                    InitiateTransmission( );

                    break;

                }    
        
                //
                // Received a request to write a value
                //
                case WPCWriteValue:
                {
                    WCWriteValue* pCommand = (WCWriteValue*) pRxAddress;

                    //
                    // Switch on the data type and write the value. This area
                    // is a little dangerous as in fact the object may have been delete
                    // and the packet has not yet received the host. Deal with this latter if
                    // this occurs.
                    //
                    switch( radPlatformEndian32( pCommand->m_DataType ) )
                    {
                        case WPDUnsignedInt :
                        {
                            *((unsigned int*)(pCommand->m_Address)) = radPlatformEndian32( pCommand->m_Attrib.m_ui.m_Value );
                            break;
                        }

                        case WPDInt :
                        {
                            *((int*)(pCommand->m_Address))  = radPlatformEndian32( pCommand->m_Attrib.m_i.m_Value );
                            break;
                        }

                        case WPDUnsignedShort :
                        {
                            *((unsigned short*)(pCommand->m_Address)) = radPlatformEndian16(pCommand->m_Attrib.m_us.m_Value );
                            break;
                        }

                        case WPDShort :
                        {
                            *((short*)(pCommand->m_Address)) = radPlatformEndian16( pCommand->m_Attrib.m_s.m_Value );
                            break;
                        }

                        case WPDUnsignedChar :
                        {
                            *((unsigned char*)(pCommand->m_Address)) = pCommand->m_Attrib.m_uc.m_Value;
                            break;
                        }

                        case WPDChar :
                        {
                            *((char*)(pCommand->m_Address)) = pCommand->m_Attrib.m_c.m_Value;
                            break;
                        }

                        case WPDBoolean :
                        {
                            *((bool*)(pCommand->m_Address)) = pCommand->m_Attrib.m_bool.m_Value != 0 ;
                            break;
                        }

                        case WPDFloat :
                        {
                            *((float*)(pCommand->m_Address)) = radPlatformEndianFloat( pCommand->m_Attrib.m_f.m_Value );
                            break;
                        }

                        case WPDEnum :
                        {
                            *((int*)(pCommand->m_Address)) = pCommand->m_Attrib.m_enum.m_Value;
                            break;
                        }

                        case WPDVector :
                        {
                            *((float*)(pCommand->m_Address) + 0) = radPlatformEndianFloat( pCommand->m_Attrib.m_vector.m_x );
                            *((float*)(pCommand->m_Address) + 1) = radPlatformEndianFloat( pCommand->m_Attrib.m_vector.m_y );
                            *((float*)(pCommand->m_Address) + 2) = radPlatformEndianFloat( pCommand->m_Attrib.m_vector.m_z );
                            break;
                        }

                        case WPDString :
                        {
                            strcpy( (char*) pCommand->m_Address, (const char*) pRxAddress + sizeof( WCWriteValue ) );
                            bytesReceived -= radPlatformEndian32( pCommand->m_Attrib.m_string.m_Length );
                            pRxAddress += radPlatformEndian32( pCommand->m_Attrib.m_string.m_Length );
                            break;
                        }

                        case WPDFunction:
                        {
                            //
                            // Nothing to write.
                            //
                            break;
                        }

                        default:
                        {
                            rAssertMsg( false, "Bad packet received from target");
                        }
                    }
                            
                    //
                    // Check if the caller has specified a notification function. If so, invoke it now.
                    //
                    if( pCommand->m_Callback != NULL )
                    {
                        (pCommand->m_Callback)( pCommand->m_UserData );
                    }

                    //
                    // Update receive pointer and bytes to process.
                    //
                    bytesReceived -= sizeof( WCWriteValue );
                    pRxAddress += sizeof( WCWriteValue );

                    break;
                }    

                default:
                {
                    rAssertMsg( false, "Bad packet received from target");
                }
       
            }
        }

        //
        // Lets acknowledge the receipt to the packet.
        //
        if( IsThereTxRoom( sizeof( WCPacketReceived ) ) )
        {
            WCPacketReceived* pReply = (WCPacketReceived*) m_CurrentTxAddress;
            pReply->m_Command = (WPCommand) radPlatformEndian32( WPCPacketReceived );

            m_CurrentTxAddress += sizeof( WCPacketReceived );
            m_TxBytesQueued += sizeof( WCPacketReceived );

            InitiateTransmission( );
        }

        //
        // Re-issue the receive buffer.
        //
        if ( bytesReceived > 0 )
        {
            memcpy( m_RxBuffer, pRxAddress, bytesReceived );
        }

        memset( m_RxBuffer + bytesReceived, 0, sizeof( m_RxBuffer ) - bytesReceived );
        m_Channel->ReceiveAsync( m_RxBuffer + bytesReceived, sizeof( m_RxBuffer ) - bytesReceived, this );
    }
}

//=============================================================================
// Function:    DebugWatchClient::OnSendComplete
//=============================================================================
// Description: Check if successful, If so, see if more to send and do so if
//              ready.
//              
// Parameters:  
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void DebugWatchClient::OnSendComplete
( 
    bool successful
)
{
    m_TxOutstanding = false;

    if( successful )
    {
        //
        // Initiate send possible,
        //
        InitiateTransmission( );
    }      
}


//=============================================================================
// Function:    DebugWatchClient::InitiateTransmission
//=============================================================================
// Description: Try to send data if we can.
//              
// Parameters:  
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void DebugWatchClient::InitiateTransmission( void )
{
    //
    // Check if attached, and no transmit outstanding and that we have data to send.,
    //
    if( m_Attached && !m_TxOutstanding && (m_TxBytesQueued != 0) )
    {
        if( m_Buffer1Active )
        {
            //
            // We will send buffer1 and reset things for buffer 2
            //
            m_Channel->SendAsync( m_TxBuffer1, m_TxBytesQueued, this );
            m_CurrentTxAddress = m_TxBuffer2;                
        }
        else
        {
            //
            // We will send buffer2 and reset things for buffer 1
            //
            m_Channel->SendAsync( m_TxBuffer2, m_TxBytesQueued, this );
            m_CurrentTxAddress = m_TxBuffer1;                

        }
        m_Buffer1Active = !m_Buffer1Active;
        m_TxBytesQueued = 0;
        m_TxOutstanding = true;
    
    }
}

//=============================================================================
// Function:    DebugWatchClient::AddRef
//=============================================================================
// Description: Reference Management.
//
// Parameters:  n/a
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void DebugWatchClient::AddRef
(
    void
)
{
    m_ReferenceCount++;
}

//=============================================================================
// Function:    DebugWatchClient::Release
//=============================================================================
// Description: Reference Management.
//
// Parameters:  n/a
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void DebugWatchClient::Release
(
    void
)
{
    m_ReferenceCount--;

    if ( m_ReferenceCount == 0 )
    {
        delete this;
    }
}

//=============================================================================
// Function:    DebugWatchClient::IsCompletePacket
//=============================================================================
// Description: is the packet complete in Rx buffer?
//
// Parameters:  none
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------
bool DebugWatchClient::IsCompletePacket(unsigned int bytesRemaining, unsigned char *pRxBuffer)
{
    //
    // First make sure we have a command code.
    //
    if( bytesRemaining < sizeof( WPCommand ) )
    {
        return( false );
    }

    switch( ( WPCommand )radPlatformEndian32(*((WPCommand*) pRxBuffer ) ) )
    {
        case WPCNameSpace :
        {
            if( bytesRemaining < sizeof( WCNameSpace ) )
            {
                return( false );
            }
            break;
        }

        case WPCCreate :
        {
            if( bytesRemaining < sizeof( WCCreate ) )
            {
                return( false );
            }
            break;
        }

        case WPCCreateEnum :
        {
            if( bytesRemaining < sizeof( WCCreateEnum ) )
            {
                return( false );
            }
            WCCreateEnum* pPacket = (WCCreateEnum*) pRxBuffer;
            if( pPacket->m_Length + sizeof( WCCreateEnum ) > bytesRemaining )
            {
                return( false );
            }
    
            break;
        }

        case WPCDelete :
        {
            if( bytesRemaining < sizeof( WCDelete ) )
            {
                return( false );
            }
            break;
        }

        case WPCAssociateFile :
        {
            if( bytesRemaining < sizeof( WCAssociateFile ) )
            {
                return( false );
            }
            break;
        }

        case WPCPacketReceived :
        {
            if( bytesRemaining < sizeof( WCPacketReceived ) )
            {
                return( false );
            }
            break;
        }
        
        case WPCRequestValue:
        {
            if( bytesRemaining < sizeof( WCRequestValue ) )
            {
                return( false );
            }
        }
        break;

        case WPCWriteValue:
        {
            if( bytesRemaining < sizeof( WCWriteValue ) )
            {
                return( false );
            }
            WCWriteValue* pPacket = (WCWriteValue*) pRxBuffer;
            if ( pPacket->m_DataType == WPDString )
            {
                if( pPacket->m_Attrib.m_string.m_Length + sizeof( WCWriteValue ) > bytesRemaining )
                {
                    return( false );
                }
            }

        }
        break;

        case WPCValue :
        {
            if( bytesRemaining < sizeof( WCValue ) )
            {
                return( false );
            }
            WCValue* pPacket = (WCValue*) pRxBuffer;
            if( pPacket->m_DataType == WPDString )
            {
                if( pPacket->m_Attrib.m_string.m_Length + sizeof( WCValue ) > bytesRemaining )
                {
                    return( false );
                }
           
            }
            break;              
        }

        case WPCHostRequest :
        {
            if( bytesRemaining < sizeof( WCHostRequest ) )
            {
                return( false );
            }
            WCHostRequest* pPacket = (WCHostRequest*) pRxBuffer;
            if( pPacket->m_Length + sizeof( WCHostRequest ) > bytesRemaining )
            {
                return( false );
            }
    
            break;
        }

        default:
        {
            rAssert( false );
            break;
        }

    }

    return( true );
}

//=============================================================================
// Function:    DebugWatchClient::Dump
//=============================================================================
// Description: This member is used to display object info
//
// Parameters:  none
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

#ifdef RAD_DEBUG

void DebugWatchClient::Dump( char* pStringBuffer, unsigned int bufferSize )
{
    sprintf( pStringBuffer, "Object: [DebugWatchClient] At Memory Location:[%p]\n", this );
}

#endif
