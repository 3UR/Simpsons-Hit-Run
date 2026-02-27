//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        consoleclient.cpp
//
// Subsystem:	Foundation Technologies - Debug Console
//
// Description:	This file contains the implementation of the Foundation 
//              Technologies debug console client
//
// Date:    	Dec 13, 2000
//              Feb 02, 2001  Flow Control protocol
//
//=============================================================================

//=============================================================================
// Include Files
//=============================================================================

#include "pch.hpp"
#include <stdlib.h>
#include <string.h>
#include <raddebug.hpp>
#include <radplatform.hpp>
#include <radmemorymonitor.hpp>
#include "consoleclient.hpp"

//=============================================================================
// Local Defintions
//=============================================================================

//=============================================================================
// Static Data Defintions
//=============================================================================

static ConsoleClient*  s_theConsoleClient = NULL;

//=============================================================================
// Function:    radDebugConsoleCreate
//=============================================================================
// Description: Use this member to create a debug console. The debug communication
//              system must have been previously initialized.
//
// Parameters:  n/a
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void radDebugConsoleCreate
( 
    IRadDebugConsole** ppConsole, 
    radMemoryAllocator alloc,
    bool bSyncMode
)
{
    //
    // Simply construct the console. Pass the allocator to the object.
    //
    *ppConsole = new( alloc ) DebugConsole( alloc, bSyncMode );
}

//=============================================================================
// Function:    radDebugConsoleService
//=============================================================================
// Description: This member must be invoked by the game periodically to ensure
//              the console operates.
//
// Parameters:  n/a
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void radDebugConsoleService( void )
{
    if( s_theConsoleClient != NULL )
    {
        s_theConsoleClient->Service( );
    }
}

//=============================================================================
// Public Member Functions
//=============================================================================

//=============================================================================
// Function:    DebugConsole::DebugConsole
//=============================================================================
// Description: Constructor for debug console object.
//
// Parameters:  n/a
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

DebugConsole::DebugConsole
(
	radMemoryAllocator alloc,
    bool bSyncMode
)
	:
	m_ReferenceCount( 1 ),
	m_PointerInputCallback( NULL ),
	m_KeyboardInputCallback( NULL ),
    m_bSyncMode( bSyncMode ),
    m_InputBufferHeadIndex( 0 ),
    m_InputBufferTailIndex( 0 )
{
    m_szConsoleTitle[ 0 ] = '\0';

    radMemoryMonitorIdentifyAllocation( this, g_nameFTech, "DebugConsole" );
    //
    // Get reference to the client object.
    //
    ConsoleClient::GetClient( &m_pClient, alloc, m_bSyncMode );

    //
    // Add ourself. This is a weak reference.
    //
    m_pClient->AddConsole( this );

    m_pClient->SendCreate( this );
}

//=============================================================================
// Function:    DebugConsole::~DebugConsole
//=============================================================================
// Description: Destructor for debug console object.
//
// Parameters:  n/a
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

DebugConsole::~DebugConsole
(
	void
)
{
    //
    // Send the destroy message.
    //
    m_pClient->SendDestroy( this );
    
    //
    // Remove our weak reference
    //
    m_pClient->DeleteConsole( this );

    radRelease( static_cast< IRadDbgComChannelSendCallback* >( m_pClient ), s_theConsoleClient );
}

//=============================================================================
// Function:    DebugConsole::AddRef
//=============================================================================
// Description: Reference Management.
//
// Parameters:  n/a7
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void DebugConsole::AddRef
(
	void
)
{
	m_ReferenceCount++;
}

//=============================================================================
// Function:    DebugConsole::Release
//=============================================================================
// Description: Reference Management.
//
// Parameters:  n/a
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void DebugConsole::Release
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
// Function:    DebugConsole::SetTitle
//=============================================================================
// Description: Sets title in window.
//
// Parameters:  pConsoleTitle	pointer to text string as title
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void DebugConsole::SetTitle
( 
	const char* pConsoleTitle 
) 
{ 
    m_pClient->SendTitle( this, pConsoleTitle );
    strncpy( m_szConsoleTitle, pConsoleTitle, sizeof( m_szConsoleTitle ) / sizeof( unsigned char ) );
    m_szConsoleTitle[ sizeof( m_szConsoleTitle ) - 1 ] = '\0';
}

//=============================================================================
// Function:    DebugConsole::SetBackgroundColor
//=============================================================================
// Description: Sets background color of console window.
//
// Parameters:  textColor	value as background color.
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void DebugConsole::SetBackgroundColor
( 
	unsigned int textColor 
) 
{ 
    m_pClient->SendBackgroundColor( this, textColor );
    m_uBkGndColour = textColor;
}

//=============================================================================
// Function:    DebugConsole::Clear
//=============================================================================
// Description: Clears console window (with currently defined background color).
//
// Parameters:  n/a
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void DebugConsole::Clear
( 
	void 
)
{ 
    m_pClient->SendClear( this );
}

//=============================================================================
// Function:    DebugConsole::SetCursorPosition
//=============================================================================
// Description: Establishes the current cursor position (used by TextOut).
//
// Parameters:  x	to represent the text position in character widths.
//				y	to represent the text position in character heights.
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void DebugConsole::SetCursorPosition
(
	unsigned int x, 
	unsigned int y 
)
{ 
    m_pClient->SendCursorPosition( this, x, y );
}

//=============================================================================
// Function:    DebugConsole::SetTextColor
//=============================================================================
// Description: Sets the foreground color of text in the console windows.
//
// Parameters:  textColor	color of the foreground text.
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void DebugConsole::SetTextColor
( 
	unsigned int textColor 
) 
{
    m_pClient->SendTextColor( this, textColor );
    m_uTextColour = textColor;
}

//=============================================================================
// Function:    DebugConsole::TextOutAt
//=============================================================================
// Description: Prints text out at a given x,y coordinate on the console windows.
//
// Parameters:  pText	pointer to text string to display.
//				x		x position of text in character widths.
//				y		y position of text in character heights.
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void DebugConsole::TextOutAt
( 
	const char * pText, 
	int    x, 
	int    y 
)
{ 
    m_pClient->SendTextOutAt( this, pText, x, y );
}

//=============================================================================
// Function:    DebugConsole::TextOut
//=============================================================================
// Description: Prints text starting at the current cursor position.
//
// Parameters:  pText	pointer to text string.
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void DebugConsole::TextOut
(
	const char * pText 
)
{
    m_pClient->SendTextOut( this, pText );
}


//=============================================================================
// Function:    DebugConsole::SetKeyboardInputCallback
//=============================================================================
// Description: Establishes the function keyboard input callback.
//
// Parameters:  Pointer to callback.
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void DebugConsole::SetKeyboardInputCallback
( 
	IRadDebugConsoleKeyboardInputCallback * pCallback 
)
{ 
    //
    // Simply save the pointer.
    //
    m_KeyboardInputCallback = pCallback;
}

//=============================================================================
// Function:    DebugConsole::SetPointerInputCallback
//=============================================================================
// Description: Establishes the function pointer input callback.
//
// Parameters:  Pointer to callback.
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void DebugConsole::SetPointerInputCallback
( 
	IRadDebugConsolePointerInputCallback * pCallback 
)
{
    //
    // Simply save the pointer.
    //
    m_PointerInputCallback = pCallback;

}

//=============================================================================
// Function:    DebugConsole::OnKeyPress
//=============================================================================
// Description: Invoked when keyboard input received.
//
// Parameters:  character.
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void DebugConsole::OnKeyPress( int character )
{
    //
    // Check if we have callback. If not, just ignore.
    //
    if( m_KeyboardInputCallback != NULL )
    {
        InputBufferEntry entry;
        entry.InputType = InputBufferEntry::IBT_KeyPress;
        entry.InputData.KeyPress = character;

        rDebugPrintf( "%c", character );
        AddInputBuffer( entry );
    }
}

//=============================================================================
// Function:    DebugConsole::OnVirtualKeyPress
//=============================================================================
// Description: Invoked when virtual key input received.
//
// Parameters:  vkey - virtual key, see VK_xxxxxx
//              ctrl
//              shift
//              alt
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void DebugConsole::OnVirtualKeyPress( int vkey, bool ctrl, bool shift, bool alt )
{
    //
    // Check if we have callback. If not, just ignore.
    //
    if( m_KeyboardInputCallback != NULL )
    {
        InputBufferEntry entry;
        entry.InputType = InputBufferEntry::IBT_VirtualKeyPress;
        entry.InputData.VirtualKeyPress.vkey = vkey;
        entry.InputData.VirtualKeyPress.ctrl = ctrl;
        entry.InputData.VirtualKeyPress.shift = shift;
        entry.InputData.VirtualKeyPress.alt = alt;

        AddInputBuffer( entry );
    }
}

//=============================================================================
// Function:    DebugConsole::OnMouseClick
//=============================================================================
// Description: Invoked when mouse input received.
//
// Parameters:  x, y
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void DebugConsole::OnMouseEvent( int textx, int texty, int screenx, int screeny, char type, char ctrl, char shift, char alt, char lbutton )
{
    //
    // Check if we have callback. If not, just ignore.
    //
    if( m_PointerInputCallback != NULL )
    {
        InputBufferEntry entry;
        entry.InputType = InputBufferEntry::IBT_MouseClick;
        entry.InputData.MouseClick.textx = textx;
        entry.InputData.MouseClick.texty = texty;
        entry.InputData.MouseClick.screenx = screenx;
        entry.InputData.MouseClick.screeny = screeny;
        entry.InputData.MouseClick.type = (HdcpInputMouseEventType)type;
        entry.InputData.MouseClick.ctrl = ctrl;
        entry.InputData.MouseClick.shift = shift;
        entry.InputData.MouseClick.alt = alt;
        entry.InputData.MouseClick.lbutton = lbutton;

        AddInputBuffer( entry );
    }
}

//=============================================================================
// Function:    DebugConsole::ProcessInputBuffer
//=============================================================================
// Description: Process input data in the circular buffer
//
// Parameters:  none
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void DebugConsole::ProcessInputBuffer( void )
{
    // Precondition check for member data
    rAssert( m_InputBufferHeadIndex < InputBufferSize );
    rAssert( m_InputBufferTailIndex < InputBufferSize );

    //
    // Loop until all input data is processed
    //
    while ( m_InputBufferHeadIndex != m_InputBufferTailIndex )
    {
        //
        // read last entry in the circular buffer.
        //
        InputBufferEntry * pInputData = & m_InputBuffer[ m_InputBufferTailIndex ];

        //
        // increment tail index, wrap if neccessary.
        //
        m_InputBufferTailIndex ++;
        m_InputBufferTailIndex = m_InputBufferTailIndex % InputBufferSize;

        //
        // Now Process the user input data from the buffer to do the callback
        //
        switch( pInputData->InputType )
        {
            case InputBufferEntry::IBT_KeyPress :
            {
                if ( m_KeyboardInputCallback )
                {
                    m_KeyboardInputCallback->OnChar( pInputData->InputData.KeyPress );
                }
            }
            break;

            case InputBufferEntry::IBT_VirtualKeyPress: 
            {
                if ( m_KeyboardInputCallback )
                {
                    m_KeyboardInputCallback->OnVKey( pInputData->InputData.VirtualKeyPress.vkey,
                                                     pInputData->InputData.VirtualKeyPress.ctrl != 0,
                                                     pInputData->InputData.VirtualKeyPress.shift != 0,
                                                     pInputData->InputData.VirtualKeyPress.alt != 0 );
                }
            }
            break;

            case InputBufferEntry::IBT_MouseClick:
            {
                if ( m_PointerInputCallback )
                {
                    switch( pInputData->InputData.MouseClick.type )
                    {
                        case HdcpMouseMove :    // MouseMove
                        {
                            m_PointerInputCallback->OnButtonMove( pInputData->InputData.MouseClick.textx,
                                                                  pInputData->InputData.MouseClick.texty,
                                                                  pInputData->InputData.MouseClick.screenx,
                                                                  pInputData->InputData.MouseClick.screeny,
                                                                  pInputData->InputData.MouseClick.ctrl != 0,
                                                                  pInputData->InputData.MouseClick.shift != 0,
                                                                  pInputData->InputData.MouseClick.alt != 0,
                                                                  pInputData->InputData.MouseClick.lbutton != 0 );
                        }
                        break;

                        case HdcpMouseLeftButtonDown :    // left button down
                        {
                            m_PointerInputCallback->OnButtonDown( pInputData->InputData.MouseClick.textx,
                                                                  pInputData->InputData.MouseClick.texty,
                                                                  pInputData->InputData.MouseClick.screenx,
                                                                  pInputData->InputData.MouseClick.screeny,
                                                                  pInputData->InputData.MouseClick.ctrl != 0,
                                                                  pInputData->InputData.MouseClick.shift != 0,
                                                                  pInputData->InputData.MouseClick.alt != 0 );
                        }
                        break;

                        case HdcpMouseLeftButtonUp :    // left button up
                        {
                            m_PointerInputCallback->OnButtonUp( pInputData->InputData.MouseClick.textx,
                                                                pInputData->InputData.MouseClick.texty,
                                                                pInputData->InputData.MouseClick.screenx,
                                                                pInputData->InputData.MouseClick.screeny,
                                                                pInputData->InputData.MouseClick.ctrl != 0,
                                                                pInputData->InputData.MouseClick.shift != 0,
                                                                pInputData->InputData.MouseClick.alt != 0 );
                        }
                        break;

                        case HdcpMouseLeftButtonClick :    // left button click
                        {
                            m_PointerInputCallback->OnButtonClick( pInputData->InputData.MouseClick.textx,
                                                                   pInputData->InputData.MouseClick.texty,
                                                                   pInputData->InputData.MouseClick.screenx,
                                                                   pInputData->InputData.MouseClick.screeny,
                                                                   pInputData->InputData.MouseClick.ctrl != 0,
                                                                   pInputData->InputData.MouseClick.shift != 0,
                                                                   pInputData->InputData.MouseClick.alt != 0 );
                        }
                        break;

                        default:
                        {
                            rAssertMsg( 0, "Cannot determine DebugConsole Mouse Input Event type." );
                        }
                        break;
                    }
                }
            }
            break;
        }
    }
}

//=============================================================================
// Function:    DebugConsole::AddInputBuffer
//=============================================================================
// Description: Add a user input to buffer
//
// Parameters:  none
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void DebugConsole::AddInputBuffer( const InputBufferEntry & entry )
{
    // Precondition check for member data.
    rAssert( m_InputBufferHeadIndex < InputBufferSize );
    rAssert( m_InputBufferTailIndex < InputBufferSize );

    //
    // if buffer is full, don't add.
    //
    if ( ! IsBufferFull( ) )
    {
        m_InputBuffer[ m_InputBufferHeadIndex ] = entry;
        m_InputBufferHeadIndex ++;
        m_InputBufferHeadIndex = m_InputBufferHeadIndex % InputBufferSize;
    }
    else
    {
        rDebugPrintf( "DebugConsole : Keyboard/Mouse Input buffer overrun.\n" );
    }
}

//=============================================================================
// Function:    DebugConsole::IsBufferFull
//=============================================================================
// Description: Check if buffer is full, not a so good algorithm.
//
// Parameters:  none
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

bool DebugConsole::IsBufferFull( )
{
    // Precondition check for member data.
    rAssert( m_InputBufferHeadIndex < InputBufferSize );
    rAssert( m_InputBufferTailIndex < InputBufferSize );

    if ( m_InputBufferHeadIndex != ( InputBufferSize - 1 ) )
    {
        if ( ( m_InputBufferTailIndex - m_InputBufferHeadIndex ) == 1 )
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        if ( m_InputBufferTailIndex == 0 )
        {
            return true;
        }
        else
        {
            return false;
        }
    }
}

//=============================================================================
// Function:    DebugConsole::Dump
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

void DebugConsole::Dump( char* pStringBuffer, unsigned int bufferSize )
{
    sprintf( pStringBuffer, "Object: [DebugConsole] At Memory Location:[%p]\n", this );
}

#endif


//=============================================================================
// Function:    ConsoleClient::ConsoleClient
//=============================================================================
// Description: Constructor for the client object.
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

ConsoleClient::ConsoleClient( radMemoryAllocator alloc, bool bSyncMode )
    :
    m_ReferenceCount( 1 ),
    m_Channel( NULL ),
    m_Attached( false ),
    m_TxOutstanding( false ),
    m_TxBytesQueued( 0 ),
    m_CurrentTxAddress( m_TxBuffer1 ),
    m_Buffer1Active( true ),
    m_ConsoleListHead( NULL ),
    m_OverflowReported( false ),
    m_PendingDetach( false ),
    m_bSyncMode( bSyncMode )
{
    radMemoryMonitorIdentifyAllocation( this, g_nameFTech, "ConsoleClient" );
    //
    // Just get a reference to the channel. Do not initiate connection until 
    // console added.
    //
    radDbgComTargetCreateChannel( HOST_DEBUG_CONSOLE_PROTOCOL, 
                                  &m_Channel, alloc );
    if( m_Channel == NULL )
    {        
        rDebugString( "Debug Console Failed: Make sure debug communication system initialized\n");
        return;
    }

    m_Channel->RegisterStatusCallback( this );
}

//=============================================================================
// Function:    ConsoleClient::~ConsoleClient
//=============================================================================
// Description: Destructor for the server object. 
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

ConsoleClient::~ConsoleClient( void )
{
    if( m_Channel != NULL )
    {
        m_Channel->RegisterStatusCallback( NULL );

        radRelease( m_Channel, this );
    }
    s_theConsoleClient = NULL;
}            

//=============================================================================
// Function:    ConsoleClient::GetClient
//=============================================================================
// Description: Gets a singleton object.
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

void ConsoleClient::GetClient( ConsoleClient** pClient, radMemoryAllocator alloc, bool bSyncMode )
{
    if( s_theConsoleClient == NULL )
    {
        s_theConsoleClient = new(alloc) ConsoleClient( alloc, bSyncMode );
    }
    else
    {
        radAddRef( static_cast< IRadDbgComChannelReceiveCallback* >( s_theConsoleClient ), NULL );
    }

    *pClient = s_theConsoleClient;
}            

//=============================================================================
// Function:    ConsoleClient::AddConsole
//=============================================================================
// Description: This member adds the console to the linked list. Initiate the
//              attachment, no hard if already doing so.
//              
// Parameters:  Console 
//
// Returns:     
//
// Notes:
//------------------------------------------------------------------------------

void ConsoleClient::AddConsole
(
    DebugConsole*   pConsole 
)
{
    pConsole->m_Next = m_ConsoleListHead;
    m_ConsoleListHead = pConsole;

    if( m_Channel != NULL )
    {
        m_Channel->Attach( );
    }
}

//=============================================================================
// Function:    ConsoleClient::DeleteConsole
//=============================================================================
// Description: This member deletes the specified console. If may not exist
//              in the list as we may have experience a loss of communication
//              causing the entry to be removed.
//              
// Parameters:  Console 
//
// Returns:     
//
// Notes:
//------------------------------------------------------------------------------

void ConsoleClient::DeleteConsole
(
    DebugConsole*   pConsole 
)
{
    //
    // Remove ourself from the list.
    //
    DebugConsole** ppPrevious = &m_ConsoleListHead;
    
    while( *ppPrevious != NULL )
    {
        if( *ppPrevious == pConsole )
        {
            *ppPrevious = (*ppPrevious)->m_Next;
            break;        
        }
        ppPrevious = &((*ppPrevious)->m_Next);
    }
    
    //
    // Check if we should detach.
    //
    if( m_ConsoleListHead == NULL )
    {
        
        //
        // If outstanding transmit, wait for it to complete before detaching.
        //
        if( !m_TxOutstanding )
        {
            if( m_Channel != NULL )
            {
                m_Attached = false;
                m_Channel->Detach( );
            }                
        }
        else
        {
            m_PendingDetach = true;
        }
    }
}        

//=============================================================================
// Function:    Console::OnStatusChange
//=============================================================================
// Description: 
//              
// Parameters:  
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void ConsoleClient::OnStatusChange
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

            DebugConsole * pDebugConsole = m_ConsoleListHead;

            while( pDebugConsole )
            {
                SendCreate( pDebugConsole );
                SendTitle( pDebugConsole, pDebugConsole->m_szConsoleTitle );
                SendBackgroundColor( pDebugConsole, pDebugConsole->m_uBkGndColour );
                SendTextColor( pDebugConsole, pDebugConsole->m_uTextColour );    
                pDebugConsole = pDebugConsole->m_Next;
            }

            InitiateTransmission( );

            break;
        }

        case IRadDbgComChannel::Detached :
        {   
            //
            // Set flag inidicating not attached and clear the list of consoles.
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
            // If we are attached, clear consoles. Print message that we lost communication.
            //
            if( m_Attached )
            {
                m_Attached = false;

                m_TxBytesQueued = 0;
                m_CurrentTxAddress = m_TxBuffer1;
                m_Buffer1Active = true;
                m_TxOutstanding = false;

                rDebugString( "Warning: Existing console output will be lost\n");
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
// Function:    Console::OnReceiveComplete
//=============================================================================
// Description: 
//              
// Parameters:  
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void ConsoleClient::OnReceiveComplete
( 
    bool            successful, 
    unsigned int    bytesReceived
)
{
    static int bytesLeftOver = 0;

    if( successful )
    {
        //
        // Here we have received a buffer. Process the data. Simply process each packet
        // of data in the buffer. 
        //
        unsigned char* pRxAddress = m_RxBuffer;

        while( IsCompletePacket( bytesReceived, pRxAddress ) )
        {
            //
            // Switch on the command code of the buffer.
            // 
            switch( radPlatformEndian32( *((HdcpCommand*) pRxAddress) ) )
            {
                case HdcsKeyboardInput :
                {
                    HdcpKeyboardInputCmd* pCommand = (HdcpKeyboardInputCmd*) pRxAddress;

                    //
                    // Attempt to verify that the id is still valid.
                    //
                    DebugConsole* pConsole = (DebugConsole*) pCommand->m_ConsoleId;
                    if( IsValidConsole( pConsole ) )
                    {
                        pConsole->OnKeyPress( radPlatformEndian32( pCommand->m_AsciiKey ) );
                    }

                    //
                    // Update receive pointer and bytes to process.
                    //
                    bytesReceived -= sizeof( HdcpKeyboardInputCmd );
                    pRxAddress += sizeof( HdcpKeyboardInputCmd );

                    break;
                }

                case HdcsVirtualKeyInput :
                {
                    HdcpVirtualKeyInputCmd* pCommand = (HdcpVirtualKeyInputCmd*) pRxAddress;
                    
                    //
                    // Attempt to verify that the id is still valid.
                    //
                    DebugConsole* pConsole = (DebugConsole*) pCommand->m_ConsoleId;
                    if( IsValidConsole( pConsole ) )
                    {
                        pConsole->OnVirtualKeyPress( radPlatformEndian32( pCommand->m_VirtualKey ), 
                                                     radPlatformEndian32( pCommand->m_Ctrl ) != 0,
                                                     radPlatformEndian32( pCommand->m_Shift ) != 0,
                                                     radPlatformEndian32( pCommand->m_Alt ) != 0 );
                    }
                    //
                    // Update receive pointer and bytes to process.
                    //
                    bytesReceived -= sizeof( HdcpVirtualKeyInputCmd );
                    pRxAddress += sizeof( HdcpVirtualKeyInputCmd );

                    break;
                }

                case HdcsPointerInput :
                {
                    HdcpPointerInputCmd* pCommand = (HdcpPointerInputCmd*) pRxAddress;

                    //
                    // Attempt to verify that the id is still valid.
                    //
                    DebugConsole* pConsole = (DebugConsole*) pCommand->m_ConsoleId;
                    if( IsValidConsole( pConsole ) )
                    {
                        pConsole->OnMouseEvent( radPlatformEndian32( pCommand->m_XTextPixels ),
                                                radPlatformEndian32( pCommand->m_YTextPixels ),
                                                radPlatformEndian32( pCommand->m_XScreenPixels ),
                                                radPlatformEndian32( pCommand->m_YScreenPixels ),
                                                radPlatformEndian32( pCommand->m_Type ),
                                                radPlatformEndian32( pCommand->m_Ctrl ),
                                                radPlatformEndian32( pCommand->m_Shift ),
                                                radPlatformEndian32( pCommand->m_Alt ),
                                                radPlatformEndian32( pCommand->m_LButton ) );
                    }

                    //
                    // Update receive pointer and bytes to process.
                    //
                    bytesReceived -= sizeof( HdcpPointerInputCmd );
                    pRxAddress += sizeof( HdcpPointerInputCmd );

                    break;
                }

                default:
                {
                    rAssertMsg( false, "Bad packet received from target");
                }
       
            }
        }

        SendClientRecvReadyCommand( );

        //
        // Re-issue the receive buffer. Move any bytes remaining to the front of the buffer.
        //
        if( bytesReceived != 0 )
        {
            bytesLeftOver = bytesReceived;
            memmove( m_RxBuffer, pRxAddress, bytesLeftOver );
        }  

        //
        // Re-issue the receive buffer.
        //
        m_Channel->ReceiveAsync( m_RxBuffer + bytesLeftOver, sizeof( m_RxBuffer ) - bytesLeftOver, this );
    }
}

//=============================================================================
// Function:    ConsoleClient::OnSendComplete
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

void ConsoleClient::OnSendComplete
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
// Function:    ConsoleClient::SendClientRecvReadyCommand
//=============================================================================
// Description: Add ClientRecvReady command to TxBuffer.
//              Initiate send if necessary.
//              
// Parameters:  
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void ConsoleClient::SendClientRecvReadyCommand( void )
{
    //
    // if sync mode spin until data is sent over
    //
    if ( m_bSyncMode )
    {
        while( m_TxBytesQueued + sizeof( HdcpClientRecvReadyCmd ) > DBGCNSL_BUFFER_SIZE && m_Attached )
        {
            ::radDbgComService( );
        }
    }

    if( ( m_TxBytesQueued + sizeof( HdcpClientRecvReadyCmd ) ) <= DBGCNSL_BUFFER_SIZE )
    {
        //
        // We have room. Add the data,
        //
        HdcpClientRecvReadyCmd* pCommand = (HdcpClientRecvReadyCmd*) m_CurrentTxAddress;
        pCommand->m_Command = (HdcpCommand)radPlatformEndian32( HdcsClientRecvReady );

        m_CurrentTxAddress += sizeof( HdcpClientRecvReadyCmd );
        m_TxBytesQueued += sizeof( HdcpClientRecvReadyCmd );

        InitiateTransmission( );
    }
    else if( !m_OverflowReported )
    {
        //
        // Here we don't have room. Print warning message and return.
        //
        rDebugString("DebugConsoleClient lost data. Increase buffer size\n");
        m_OverflowReported = true;
    }
}

//=============================================================================
// Function:    Console::SendCreate
//=============================================================================
// Description: Send the console create command.
//              
// Parameters:  console
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

bool ConsoleClient::IsCompletePacket( unsigned int bytesRemaining, unsigned char* pRxBuffer )
{
    if ( bytesRemaining < sizeof( HdcpCommand ) )
    {
        return false;
    }

    switch( radPlatformEndian32( *( ( HdcpCommand * )( pRxBuffer ) ) ) )
    {
        case HdcsCreate :
        {
            return bytesRemaining >= sizeof( HdcpCreateCmd );
        }
        break;

        case HdcsTitle :
        {
            return bytesRemaining >= sizeof( HdcpTitleCmd );
        }
        break;

        case HdcsBackgroundColor :
        {
            return bytesRemaining >= sizeof( HdcpBackgroundColorCmd );
        }
        break;

        case HdcsClear :
        {
            return bytesRemaining >= sizeof( HdcpClearCmd );
        }
        break;

        case HdcsCursorPosition :
        {
            return bytesRemaining >= sizeof( HdcpCursorPositionCmd );
        }
        break;

        case HdcsTextColor :
        {
            return bytesRemaining >= sizeof( HdcpTextColorCmd );
        }
        break;

        case HdcsTextOutAt :
        {
            return bytesRemaining >= sizeof( HdcpTextOutAtCmd );
        }
        break;

        case HdcsTextOut :
        {
            return bytesRemaining >= sizeof( HdcpTextOutCmd );
        }
        break;

        case HdcsClose :
        {
            return bytesRemaining >= sizeof( HdcpCloseCmd );
        }
        break;

        case HdcsKeyboardInput :
        {
            return bytesRemaining >= sizeof( HdcpKeyboardInputCmd );
        }
        break;

        case HdcsVirtualKeyInput :
        {
            return bytesRemaining >= sizeof( HdcpVirtualKeyInputCmd );
        }
        break;

        case HdcsPointerInput :
        {
            return bytesRemaining >= sizeof( HdcpPointerInputCmd );
        }
        break;

        case HdcsClientRecvReady :
        {
            return bytesRemaining >= sizeof( HdcpClientRecvReadyCmd );
        }
        break;

        case HdcsHostRecvReady :
        {
            return bytesRemaining >= sizeof( HdcpHostRecvReadyCmd );
        }
        break;

        default:
        {
            rAssert( false );
        }
        break;
    }

    return false;
}

//=============================================================================
// Function:    Console::SendCreate
//=============================================================================
// Description: Send the console create command.
//              
// Parameters:  console
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void ConsoleClient::SendCreate( DebugConsole *pConsole )
{
    //
    // if sync mode spin until data is sent over
    //
    if ( m_bSyncMode )
    {
        while( m_TxBytesQueued + sizeof( HdcpCreateCmd ) > DBGCNSL_BUFFER_SIZE && m_Attached )
        {
            ::radDbgComService( );
        }
    }

    //
    // Check if room in the active buffer to add 
    //
    if( m_TxBytesQueued + sizeof( HdcpCreateCmd ) <= DBGCNSL_BUFFER_SIZE )
    {
        //
        // We have room. Add the data,
        //
        HdcpCreateCmd* pCommand = (HdcpCreateCmd*) m_CurrentTxAddress;
        pCommand->m_Command = (HdcpCommand) radPlatformEndian32( HdcsCreate );
        pCommand->m_ConsoleId = pConsole;

        m_CurrentTxAddress += sizeof( HdcpCreateCmd );
        m_TxBytesQueued += sizeof( HdcpCreateCmd );

        InitiateTransmission( );
    }
    else if( !m_OverflowReported )
    {
        //
        // Here we don't have room. Print warning message and return.
        //
        rDebugString("DebugConsoleClient lost data. Increase buffer size\n");
        m_OverflowReported = true;
    }

}

//=============================================================================
// Function:    Console::SendDestroy
//=============================================================================
// Description: Send the console destroy command.
//              
// Parameters:  console
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void ConsoleClient::SendDestroy( DebugConsole *pConsole )
{
    //
    // if sync mode spin until data is sent over
    //
    if ( m_bSyncMode )
    {
        while( m_TxBytesQueued + sizeof( HdcpCloseCmd ) > DBGCNSL_BUFFER_SIZE && m_Attached )
        {
            ::radDbgComService( );
        }
    }

    //
    // Check if room in the active buffer to add 
    //
    if( m_TxBytesQueued + sizeof( HdcpCloseCmd ) <= DBGCNSL_BUFFER_SIZE )
    {
        //
        // We have room. Add the data,
        //
        HdcpCloseCmd* pCommand = (HdcpCloseCmd*) m_CurrentTxAddress;
        pCommand->m_Command = (HdcpCommand) radPlatformEndian32( HdcsClose );
        pCommand->m_ConsoleId = pConsole;

        m_CurrentTxAddress += sizeof( HdcpCloseCmd );
        m_TxBytesQueued += sizeof( HdcpCloseCmd );

        InitiateTransmission( );
    }
    else if( !m_OverflowReported )
    {
        //
        // Here we don't have room. Print warning message and return.
        //
        rDebugString("DebugConsoleClient lost data. Increase buffer size\n");
        m_OverflowReported = true;
    }

}

//=============================================================================
// Function:    Console::SendTitle
//=============================================================================
// Description: 
//              
// Parameters:  
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void ConsoleClient::SendTitle( DebugConsole *pConsole, const char* title )
{
    //
    // if sync mode spin until data is sent over
    //
    if ( m_bSyncMode )
    {
        while( m_TxBytesQueued + sizeof( HdcpTitleCmd ) > DBGCNSL_BUFFER_SIZE && m_Attached )
        {
            ::radDbgComService( );
        }
    }

    //
    // Check if room in the active buffer to add 
    //
    if( m_TxBytesQueued + sizeof( HdcpTitleCmd ) <= DBGCNSL_BUFFER_SIZE )
    {
        //
        // We have room. Add the data,
        //
        HdcpTitleCmd* pCommand = (HdcpTitleCmd*) m_CurrentTxAddress;
        pCommand->m_Command = (HdcpCommand) radPlatformEndian32( HdcsTitle );
        pCommand->m_ConsoleId = pConsole;

        strncpy( pCommand->m_Title, title, sizeof( pCommand->m_Title ) );
        pCommand->m_Title[ sizeof( pCommand->m_Title ) - 1 ] = '\0';

        m_CurrentTxAddress += sizeof( HdcpTitleCmd );
        m_TxBytesQueued += sizeof( HdcpTitleCmd );

        InitiateTransmission( );
    }
    else if( !m_OverflowReported )
    {
        //
        // Here we don't have room. Print warning message and return.
        //
        rDebugString("DebugConsoleClient lost data. Increase buffer size\n");
        m_OverflowReported = true;
    }

}

//=============================================================================
// Function:    Console::SendBackgroundColor
//=============================================================================
// Description: 
//              
// Parameters:  
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void ConsoleClient::SendBackgroundColor( DebugConsole *pConsole, unsigned int color )
{
    //
    // if sync mode spin until data is sent over
    //
    if ( m_bSyncMode )
    {
        while( m_TxBytesQueued + sizeof( HdcpBackgroundColorCmd ) > DBGCNSL_BUFFER_SIZE && m_Attached )
        {
            ::radDbgComService( );
        }
    }

    //
    // Check if room in the active buffer to add 
    //
    if( m_TxBytesQueued + sizeof( HdcpBackgroundColorCmd ) <= DBGCNSL_BUFFER_SIZE )
    {
        //
        // We have room. Add the data,
        //
        HdcpBackgroundColorCmd* pCommand = (HdcpBackgroundColorCmd*) m_CurrentTxAddress;
        pCommand->m_Command = (HdcpCommand) radPlatformEndian32( HdcsBackgroundColor );
        pCommand->m_ConsoleId = pConsole;
        pCommand->m_BackgroundColor = radPlatformEndian32( color );

        m_CurrentTxAddress += sizeof( HdcpBackgroundColorCmd );
        m_TxBytesQueued += sizeof( HdcpBackgroundColorCmd );

        InitiateTransmission( );
    }
    else if( !m_OverflowReported )
    {
        //
        // Here we don't have room. Print warning message and return.
        //
        rDebugString("DebugConsoleClient lost data. Increase buffer size\n");
        m_OverflowReported = true;
    }
}

//=============================================================================
// Function:    Console::SendClear
//=============================================================================
// Description: 
//              
// Parameters:  
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void ConsoleClient::SendClear( DebugConsole *pConsole )
{
    //
    // if sync mode spin until data is sent over
    //
    if ( m_bSyncMode )
    {
        while( m_TxBytesQueued + sizeof( HdcpClearCmd ) > DBGCNSL_BUFFER_SIZE && m_Attached )
        {
            ::radDbgComService( );
        }
    }

    //
    // Check if room in the active buffer to add 
    //
    if( m_TxBytesQueued + sizeof( HdcpClearCmd ) <= DBGCNSL_BUFFER_SIZE )
    {
        //
        // We have room. Add the data,
        //
        HdcpClearCmd* pCommand = (HdcpClearCmd*) m_CurrentTxAddress;
        pCommand->m_Command = (HdcpCommand) radPlatformEndian32( HdcsClear );
        pCommand->m_ConsoleId = pConsole;

        m_CurrentTxAddress += sizeof( HdcpClearCmd );
        m_TxBytesQueued += sizeof( HdcpClearCmd );

        InitiateTransmission( );
    }
    else if( !m_OverflowReported )
    {
        //
        // Here we don't have room. Print warning message and return.
        //
        rDebugString("DebugConsoleClient lost data. Increase buffer size\n");
        m_OverflowReported = true;
    }
}

//=============================================================================
// Function:    Console::SendCursorPosition
//=============================================================================
// Description: 
//              
// Parameters:  
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void ConsoleClient::SendCursorPosition( DebugConsole *pConsole, unsigned int x, unsigned int y )
{
    //
    // if sync mode spin until data is sent over
    //
    if ( m_bSyncMode )
    {
        while( m_TxBytesQueued + sizeof( HdcpCursorPositionCmd ) > DBGCNSL_BUFFER_SIZE && m_Attached )
        {
            ::radDbgComService( );
        }
    }

    //
    // Check if room in the active buffer to add 
    //
    if( m_TxBytesQueued + sizeof( HdcpCursorPositionCmd ) <= DBGCNSL_BUFFER_SIZE )
    {
        //
        // We have room. Add the data,
        //
        HdcpCursorPositionCmd* pCommand = (HdcpCursorPositionCmd*) m_CurrentTxAddress;
        pCommand->m_Command = (HdcpCommand) radPlatformEndian32( HdcsCursorPosition );
        pCommand->m_ConsoleId = pConsole;
        pCommand->m_XPosition = radPlatformEndian32( x );
        pCommand->m_YPosition = radPlatformEndian32( y );
    
        m_CurrentTxAddress += sizeof( HdcpCursorPositionCmd );
        m_TxBytesQueued += sizeof( HdcpCursorPositionCmd );

        InitiateTransmission( );
    }
    else if( !m_OverflowReported )
    {
        //
        // Here we don't have room. Print warning message and return.
        //
        rDebugString("DebugConsoleClient lost data. Increase buffer size\n");
        m_OverflowReported = true;
    }
}

//=============================================================================
// Function:    Console::SendTextColor
//=============================================================================
// Description: 
//              
// Parameters:  
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void ConsoleClient::SendTextColor( DebugConsole *pConsole, unsigned int color )
{
    //
    // if sync mode spin until data is sent over
    //
    if ( m_bSyncMode )
    {
        while( m_TxBytesQueued + sizeof( HdcpTextColorCmd ) > DBGCNSL_BUFFER_SIZE && m_Attached )
        {
            ::radDbgComService( );
        }
    }

    //
    // Check if room in the active buffer to add 
    //
    if( m_TxBytesQueued + sizeof( HdcpTextColorCmd ) <= DBGCNSL_BUFFER_SIZE )
    {
        //
        // We have room. Add the data,
        //
        HdcpTextColorCmd* pCommand = (HdcpTextColorCmd*) m_CurrentTxAddress;
        pCommand->m_Command = (HdcpCommand) radPlatformEndian32( HdcsTextColor );
        pCommand->m_ConsoleId = pConsole;
        pCommand->m_TextColor = radPlatformEndian32( color );

        m_CurrentTxAddress += sizeof( HdcpTextColorCmd );
        m_TxBytesQueued += sizeof( HdcpTextColorCmd );

        InitiateTransmission( );
    }
    else if( !m_OverflowReported )
    {
        //
        // Here we don't have room. Print warning message and return.
        //
        rDebugString("DebugConsoleClient lost data. Increase buffer size\n");
        m_OverflowReported = true;
    }
}

//=============================================================================
// Function:    Console::SendTextOutAt
//=============================================================================
// Description: 
//              
// Parameters:  
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void ConsoleClient::SendTextOutAt( DebugConsole *pConsole, const char* title, int x, int y )
{
    //
    // if sync mode spin until data is sent over
    //
    if ( m_bSyncMode )
    {
        while( ( m_TxBytesQueued + sizeof( HdcpTextOutAtCmd ) ) > DBGCNSL_BUFFER_SIZE && m_Attached )
        {
            ::radDbgComService( );
        }
    }

    //
    // Check if room in the active buffer to add 
    //
    if( m_TxBytesQueued + sizeof( HdcpTextOutAtCmd ) <= DBGCNSL_BUFFER_SIZE )
    {
        //
        // We have room. Add the data,
        //
        HdcpTextOutAtCmd* pCommand = (HdcpTextOutAtCmd*) m_CurrentTxAddress;
        pCommand->m_Command = (HdcpCommand) radPlatformEndian32( HdcsTextOutAt );
        pCommand->m_ConsoleId = pConsole;
        pCommand->m_XPosition = radPlatformEndian32( x );
        pCommand->m_YPosition = radPlatformEndian32( y );

        rWarningMsg( strlen( title ) < sizeof( pCommand->m_Text ), "String too long\n"); 

        strncpy( pCommand->m_Text, title, sizeof( pCommand->m_Text ) );
        pCommand->m_Text[ sizeof( pCommand->m_Text ) - 1 ] = '\0';

        m_CurrentTxAddress += sizeof( HdcpTextOutAtCmd );
        m_TxBytesQueued += sizeof( HdcpTextOutAtCmd );

        InitiateTransmission( );
    }
    else if( !m_OverflowReported )
    {
        //
        // Here we don't have room. Print warning message and return.
        //
        rDebugString("DebugConsoleClient lost data. Increase buffer size\n");
        m_OverflowReported = true;
    }

}

//=============================================================================
// Function:    Console::SendTextOut
//=============================================================================
// Description: 
//              
// Parameters:  
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void ConsoleClient::SendTextOut( DebugConsole *pConsole, const char* title )
{
    //
    // if sync mode spin until data is sent over
    //
    if ( m_bSyncMode )
    {
        while( m_TxBytesQueued + sizeof( HdcpTextOutCmd ) > DBGCNSL_BUFFER_SIZE && m_Attached )
        {
            ::radDbgComService( );
        }
    }

    //
    // Check if room in the active buffer to add 
    //
    if( m_TxBytesQueued + sizeof( HdcpTextOutCmd ) <= DBGCNSL_BUFFER_SIZE )
    {
        //
        // We have room. Add the data,
        //
        HdcpTextOutCmd* pCommand = (HdcpTextOutCmd*) m_CurrentTxAddress;
        pCommand->m_Command = (HdcpCommand) radPlatformEndian32( HdcsTextOut );
        pCommand->m_ConsoleId = pConsole;

        rWarningMsg( strlen( title ) < sizeof( pCommand->m_Text ), "String too long\n "); 

        strncpy( pCommand->m_Text, title, sizeof( pCommand->m_Text ) );
        pCommand->m_Text[ sizeof( pCommand->m_Text ) - 1 ] = '\0';

        m_CurrentTxAddress += sizeof( HdcpTextOutCmd );
        m_TxBytesQueued += sizeof( HdcpTextOutCmd );

        InitiateTransmission( );
    }
    else if( !m_OverflowReported )
    {
        //
        // Here we don't have room. Print warning message and return.
        //
        rDebugString("DebugConsoleClient lost data. Increase buffer size\n");
        m_OverflowReported = true;
    }

}

//=============================================================================
// Function:    Console::InitiateTransmission
//=============================================================================
// Description: Try to send data if we can.
//              
// Parameters:  
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void ConsoleClient::InitiateTransmission( void )
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
// Function:    Console::IsValidConsole
//=============================================================================
// Description: Verify that the console still exists.
//              
// Parameters:  DebugConsole
//
// Returns:     boolean
//
// Notes:
//------------------------------------------------------------------------------

bool ConsoleClient::IsValidConsole
( 
    DebugConsole* pConsole
)
{
    DebugConsole* pC = m_ConsoleListHead;
    
    while( pC != NULL )
    {
        if( pC == pConsole )
        {
            return( true );
        }
        pC = pC->m_Next;
    }
    
    return( false );

}

//=============================================================================
// Function:    ConsoleClient::AddRef
//=============================================================================
// Description: This member is used to update the reference count of the object.
//
// Parameters:  none
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------
    
void ConsoleClient::AddRef( void )
{
    m_ReferenceCount++;
}

//=============================================================================
// Function:    ConsoleClient::Service
//=============================================================================
// Description: This member is used to drive processing. Just ensure communicaiton
//              system is serviced.
//
// Parameters:  none
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------
    
void ConsoleClient::Service( void )
{
    DebugConsole * pListEntry = m_ConsoleListHead;
    while ( pListEntry != NULL )
    {
        pListEntry->ProcessInputBuffer( );
        pListEntry = pListEntry->m_Next;
    }
}

//=============================================================================
// Function:    ConsoleClient::Release
//=============================================================================
// Description: This member is used to update the reference count of the object.
//
// Parameters:  none
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------
    
void ConsoleClient::Release( void )
{
    m_ReferenceCount--;
    if( m_ReferenceCount == 0 )
    {
        delete this;
    }   
}

//=============================================================================
// Function:    ConsoleClient::Dump
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

void ConsoleClient::Dump( char* pStringBuffer, unsigned int bufferSize )
{
    sprintf( pStringBuffer, "Object: [ConsoleClient] At Memory Location:[%p]\n", this );
}

#endif