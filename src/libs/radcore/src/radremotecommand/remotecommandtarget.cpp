//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        remotecommandtarget.cpp
//
// Subsystem:	Foundation Technologies - Remote Command Server
//
// Description:	This file contains the implementation of the Foundation 
//              Technologies Remote Command Server
//
// Date:    	07-Feb-2001
//
//=============================================================================

//=============================================================================
// Include Files
//=============================================================================

#include "pch.hpp"
#include <string.h>
#include <radmemorymonitor.hpp>
#include "remotecommand.hpp"
#include <radplatform.hpp>

//=============================================================================
// Member Functions
//=============================================================================

//=============================================================================
// Function:    RemoteCommandTarget::RemoteCommandTarget
//=============================================================================
// Description: Constructor for the RemoteCommandTarget.
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

RemoteCommandTarget::RemoteCommandTarget( radMemoryAllocator alloc )
    :
	radRefCount( 0 ),
    m_Channel( NULL ),
    m_Attached( false ),
    m_TxOutstanding( false ),
    m_TxBytesQueued( 0 ),
    m_CurrentTxAddress( m_TxBuffer1 ),
    m_Buffer1Active( true ),
    m_OverflowReported( false ),
    m_PendingDetach( false ),
    m_Allocator( alloc )
{
    radMemoryMonitorIdentifyAllocation( this, g_nameFTech, "RemoteCommandTarget" );
}

//=============================================================================
// Function:    RemoteCommandTarget::~RemoteCommandTarget
//=============================================================================
// Description: Destructor for the RemoteCommandTarget
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

RemoteCommandTarget::~RemoteCommandTarget( void )
{	
    if( m_Channel != NULL )
    {
        m_Channel->RegisterStatusCallback( NULL );
        m_Channel = NULL;
    }

}            

//=============================================================================
// Function:    RemoteCommandTarget::OnStatusChange
//=============================================================================
// Description: 
//              
// Parameters:  
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void RemoteCommandTarget::OnStatusChange
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
            break;
        }

        case IRadDbgComChannel::Detached :
        {   
            //
            // Set flag inidicating not attached and do a little housekeeping
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
            // If we are attached, do a little housekeeping.
            //
            if( m_Attached )
            {

                m_Attached = false;

                m_TxBytesQueued = 0;
                m_CurrentTxAddress = m_TxBuffer1;
                m_Buffer1Active = true;
                m_TxOutstanding = false;
            }
            break;
        }
        default:
	    {
			//
			// Detaching (or something else).  Nothing to do.
			//
            break;
        }
    }
}

//=============================================================================
// Function:    RemoteCommandTarget::OnReceiveComplete
//=============================================================================
// Description: 
//              
// Parameters:  
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void RemoteCommandTarget::OnReceiveComplete
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

        while( bytesReceived != 0 )
        {
            //
            // Switch on the command code of the buffer.
            // 
            switch( radPlatformEndian32( *((HrcsCommand*) pRxAddress) ) )
            {
				case HrcsList:
				{
					//
					// Ask the RemoteCommandServer to list its registered functions
					//
					m_RemoteCommandServer->ListFunctions();
									
                    //
                    // Update receive pointer and bytes to process.
                    //
                    bytesReceived -= sizeof( HrcsListCommand );
                    pRxAddress += sizeof( HrcsListCommand );
					break;
				}
				case HrcsRemoteFunction:
				{
					// Cast this into something readable
					HrcsRemoteFunctionCommand* pCommand = (HrcsRemoteFunctionCommand*) pRxAddress;
					
					//
					// Ask the RemoteCommandServer to call the function.
					//
					m_RemoteCommandServer->CallFunction
					( 
						pCommand->m_FunctionName,
						radPlatformEndian32( pCommand->m_argc ),
						pCommand->m_argv
					);
														
                    //
                    // Update receive pointer and bytes to process.
                    //
                    bytesReceived -= sizeof( HrcsRemoteFunctionCommand );
                    pRxAddress += sizeof( HrcsRemoteFunctionCommand );
					break;
				}
                default:
                {
                    rAssertMsg( false, "Bad packet received from target");
					break;
                }
       
            }
        }

        //
        // Re-issue the receive buffer.
        //
        m_Channel->ReceiveAsync( m_RxBuffer, sizeof( m_RxBuffer ), this );
    }
}

//=============================================================================
// Function:    RemoteCommandTarget::OnSendComplete
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

void RemoteCommandTarget::OnSendComplete
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

    if( m_PendingDetach && !m_TxOutstanding )
    {
        //
        // Attached false.
        //
        m_Attached = false;
        m_Channel->Detach( );
    }
}


//=============================================================================
// Function:    RemoteCommandTarget::InitiateTransmission
//=============================================================================
// Description: Try to send data if we can.
//              
// Parameters:  
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void RemoteCommandTarget::InitiateTransmission( void )
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
// Function:    RemoteCommandTarget::SendListReply
//=============================================================================
// Description: Try to send data if we can.
//              
// Parameters:  unsigned int	numItems		- Number of registered functions
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void RemoteCommandTarget::SendListReply( unsigned int numItems )
{
    //
    // Check if room in the active buffer to add 
    //
    if( m_TxBytesQueued + sizeof( HrcsListReply ) <= RCS_BUFFER_SIZE )
    {
        //
        // We have room. Add the data,
        //
        HrcsListReplyCommand* pCommand = (HrcsListReplyCommand*) m_CurrentTxAddress;
		pCommand->m_Command = (HrcsCommand) radPlatformEndian32( HrcsListReply );
		pCommand->m_NumItems = radPlatformEndian32( numItems );

        m_CurrentTxAddress += sizeof( HrcsListReplyCommand );
        m_TxBytesQueued += sizeof( HrcsListReplyCommand );

        InitiateTransmission( );
    }
    else if( !m_OverflowReported )
    {
        //
        // Here we don't have room. Print warning message and return.
        //
        rDebugString("RemoteCommandTarget lost data. Increase buffer size\n");
        m_OverflowReported = true;
    }
}


//=============================================================================
// Function:    RemoteCommandTarget::SendListItemCommand
//=============================================================================
// Description: Try to send data if we can.
//              
// Parameters:  char*	name		- Name of a registered function
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------
void RemoteCommandTarget::SendListItemCommand( char* name )
{
    //
    // Check if room in the active buffer to add 
    //
    if( m_TxBytesQueued + sizeof( HrcsListItemCommand ) <= RCS_BUFFER_SIZE )
    {
        //
        // We have room. Add the data,
        //
        HrcsListItemCommand* pCommand = (HrcsListItemCommand*) m_CurrentTxAddress;
		pCommand->m_Command = (HrcsCommand) radPlatformEndian32( HrcsListItem );
		strcpy( pCommand->m_FunctionName, name );

        m_CurrentTxAddress += sizeof( HrcsListItemCommand );
        m_TxBytesQueued += sizeof( HrcsListItemCommand );

        InitiateTransmission( );
    }
    else if( !m_OverflowReported )
    {
        //
        // Here we don't have room. Print warning message and return.
        //
        rDebugString("RemoteCommandTarget lost data. Increase buffer size\n");
        m_OverflowReported = true;
    }
}

//=============================================================================
// Function:    RemoteCommandTarget::SendRemoteFunctionReply
//=============================================================================
// Description: Try to send data if we can.
//              
// Parameters:  HrcsResultCode	result    - Result of the Remote function call request
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------
void RemoteCommandTarget::SendRemoteFunctionReply( HrcsResultCode result )
{
    //
    // Check if room in the active buffer to add 
    //
    if( m_TxBytesQueued + sizeof( HrcsResultCommand ) <= RCS_BUFFER_SIZE )
    {
        //
        // We have room. Add the data,
        //
        HrcsResultCommand* pCommand = (HrcsResultCommand*) m_CurrentTxAddress;
		pCommand->m_Command = (HrcsCommand) radPlatformEndian32( HrcsResult );
		pCommand->m_ResultCode = (HrcsResultCode) radPlatformEndian32( result );

        m_CurrentTxAddress += sizeof( HrcsResultCommand );
        m_TxBytesQueued += sizeof( HrcsResultCommand );

        InitiateTransmission( );
    }
    else if( !m_OverflowReported )
    {
        //
        // Here we don't have room. Print warning message and return.
        //
        rDebugString("RemoteCommandTarget lost data. Increase buffer size\n");
        m_OverflowReported = true;
    }
}

//=============================================================================
// Function:    RemoteCommandTarget::SetRemoteCommandServer
//=============================================================================
// Description: Set the remoteCommandServer member
//              
// Parameters:  RemoteCommandServer * rcs
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------
void RemoteCommandTarget::SetRemoteCommandServer( RemoteCommandServer * rcs )
{
	//
	// Need to keep this reference
	//
	m_RemoteCommandServer = rcs;

	//
	// Get an rDebugComTarget interface.  Initialize the target and create a channel.
	// After we have a channel, we are finished with target reference.
	//
    radDbgComTargetCreateChannel( HOST_REMOTE_COMMAND_PROTOCOL, &m_Channel, m_Allocator );

	//
	// Verify that the channel actually exists
	//
    if( m_Channel == NULL )
    {        
        rDebugString( "Debug Console Failed: Make sure debug communication system initialized\n");
        return;
    }

	//
	// Register this class as a status callback and try to attach to the channel
	//
    m_Channel->RegisterStatusCallback( this );
	m_Channel->Attach( );
	
	//
	// Now we wait for the RemoteCommandConsole to connect...
	//
}


void RemoteCommandTarget::UnSetRemoteCommandServer( void )
{
	//
	// No more server
	//
	m_RemoteCommandServer = NULL;

	//
	// Detach the channel
	//
    if( m_Channel != NULL )
    {
		if( m_TxOutstanding )
		{
			m_PendingDetach = true;
		}
		else
		{
			m_Channel->Detach();
		}
    }
}
