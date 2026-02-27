//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        consoleclient.Hpp
//
// Subsystem:	Radical Debug Console
//
// Description:	This file contains all definitions and classes relevant to
//              the debug console implementation.
//
// Revisions:	13-Dec-2000 Creation
//              02-Feb-2001 Flow Control protocol
//
// Notes:       
//
//=============================================================================

#ifndef	CONSOLECLIENT_HPP
#define CONSOLECLIENT_HPP

//=============================================================================
// Include Files
//=============================================================================

#include <raddebugcommunication.hpp>
#include <raddebugconsole.hpp>
#include <radobject.hpp>
#include "../radprotocols/debugconsoleprotocol.hpp"

//=============================================================================
// Forward Class Declarations
//=============================================================================

class ConsoleClient;

//=============================================================================
// Defintions
//=============================================================================

//=============================================================================
// Class Declarations
//=============================================================================

//  
// This is the client object. Just implements the various interfaces and
// routes them to the host server.
//
class DebugConsole :    public IRadDebugConsole,
                        public radObject
{
public:

    //  
    // These are not directly accessed.
    //
	DebugConsole( radMemoryAllocator alloc, bool bSyncMode );
    virtual ~DebugConsole( void );

    //
	// IRefCount
    //
	virtual void AddRef( void );
	virtual void Release( void );
    #ifdef RAD_DEBUG
    virtual void Dump( char* pStringBuffer, unsigned int bufferSize );
    #endif

    //
	// IRadDebugConsole
    //
    virtual void SetTitle( const char* pConsoleTitle );
	virtual void SetBackgroundColor( unsigned int textColor );
	virtual void Clear( void );
    virtual void SetCursorPosition( unsigned int x, unsigned int y );
	virtual void SetTextColor( unsigned int textColor );
	virtual void TextOutAt( const char* pText, int x, int y );
    virtual void TextOut( const char* pText );
	virtual void SetKeyboardInputCallback( IRadDebugConsoleKeyboardInputCallback* pCallback );
	virtual void SetPointerInputCallback( IRadDebugConsolePointerInputCallback* pCallback );

    //
    // Members invoked when keys and mouse clicks arrive
    //
    void OnKeyPress( int character );
    void OnVirtualKeyPress( int vkey, bool ctrl, bool shift, bool alt );
    void OnMouseEvent( int textx, int texty, int screenx, int screeny, char type, char ctrl, char shift, char alt, char lbutton );

    //
    // read user input from buffer and do callback
    //
    void ProcessInputBuffer( );

    //
    // Used to manage list of these consoles. Not reference counted/
    //
    DebugConsole*   m_Next;    

    //
    // Attributes used for Console's style.
    //
    char                    m_szConsoleTitle[ 128 ];
    unsigned int            m_uBkGndColour;
    unsigned int            m_uTextColour;

protected:
    //
    // data structure used for store user input, key press and mouse click
    //
    struct InputBufferEntry
    {
        //
        // what type of buffer is stored in buffer
        //
        enum InputBufferType
        {
            IBT_KeyPress,           // translated key press
            IBT_VirtualKeyPress,    // non-translated key press
            IBT_MouseClick          // mouse click
        } InputType;

        //
        // actual value of the buffer
        //
        union InputBufferData
        {
            int KeyPress;           // IBT_KeyPress

            struct InputVKeyPress
            {
                int vkey;
                char ctrl;
                char shift;
                char alt;
            } VirtualKeyPress;      // IBT_VirtualKeyPress

            struct InputMouseClick
            {
                int textx;
                int texty;
                int screenx;
                int screeny;
                HdcpInputMouseEventType type;
                char ctrl;
                char shift;
                char alt;
                char lbutton;
            } MouseClick;           // IBT_MouseClick

        } InputData;
    };

    //
    // add user input to circular buffer
    //
    void AddInputBuffer( const InputBufferEntry & entry );

    //
    // is circular buffer full.
    //
    bool IsBufferFull( );

private:

    unsigned int m_ReferenceCount;

    //
    // Hold the user supplied callbacks.
    //
    IRadDebugConsolePointerInputCallback*    m_PointerInputCallback;
    IRadDebugConsoleKeyboardInputCallback*   m_KeyboardInputCallback;

    ConsoleClient*  m_pClient;

    //
    // should all console client in Sync mode operation?
    //
    bool            m_bSyncMode;

    //
    // Define const value for buffer size in VC++ 6.0 since it doesn't allow
    // static const int InputBufferSize = 128;
    //
    enum { InputBufferSize = 256 };

    //
    // Input data is stored in a circular buffer pointed by head and tail index.
    //
    InputBufferEntry        m_InputBuffer[ InputBufferSize ];
    unsigned int            m_InputBufferHeadIndex;
    unsigned int            m_InputBufferTailIndex;
    
};

//
// This class represents to object used to communicated with the host.
// It is a singleton and used by all debug consoles.
//
class ConsoleClient : public IRadDbgComChannelStatusCallback,
                      public IRadDbgComChannelReceiveCallback,
                      public IRadDbgComChannelSendCallback,
                      public radObject
{
    public:

    //
    // Constructor. Nothing too interesting.
    //
    ConsoleClient( radMemoryAllocator alloc, bool bSyncMode );
    virtual ~ConsoleClient( void );
  
    //
    // Static member used to serve up singleton.
    //
 
    static void GetClient( ConsoleClient** pClient, radMemoryAllocator alloc, bool bSyncMode );

    //
    // Status callback
    //
    virtual void OnStatusChange( IRadDbgComChannel::ConnectionState connectionState,
                                 const char* Message );
       
    virtual void OnReceiveComplete( bool successful, unsigned int bytesReceived );
    virtual void OnSendComplete( bool successful );

    //
    // IRefCount
    //
    virtual void AddRef( void );
    virtual void Release( void );

    #ifdef RAD_DEBUG
    virtual void Dump( char* pStringBuffer, unsigned int bufferSize );
    #endif

    //
    // Members to add and remove debug consoles from this object. These are not refernce counted.
    //
    void AddConsole( DebugConsole* pConsole );     
    void DeleteConsole( DebugConsole* pConsole );     
    
    //
    // Members to send data to the host.
    //
    void SendCreate( DebugConsole* pConsole );
    void SendDestroy( DebugConsole* pConsole );
    void SendTitle( DebugConsole* pConsole, const char* title );
    void SendBackgroundColor( DebugConsole* pConsole, unsigned int color );
    void SendClear( DebugConsole* pConsole );
    void SendCursorPosition( DebugConsole* pConsole, unsigned int x, unsigned int y );
	void SendTextColor( DebugConsole* pConsole, unsigned int textColor );
	void SendTextOutAt( DebugConsole* pConsole, const char* pText, int x, int y );
    void SendTextOut( DebugConsole* pConsole, const char* pText );
    void SendClientRecvReadyCommand( void );

    void Service( void );

    private:

    //
    // Private used to kick start transmission,
    //
    void InitiateTransmission( void );

    bool IsValidConsole( DebugConsole* pConsole );

    bool IsCompletePacket( unsigned int bytesRemaining, unsigned char* pRxBuffer );

    //
    // This member maintains the reference count of this object.
    //
    unsigned int m_ReferenceCount;    

    //
    // Channel used for communication.
    //
    IRadDbgComChannel*     m_Channel;
   
    //
    // Buffer used to receive messages from the target.
    //
    unsigned char m_RxBuffer[ DBGCNSL_BUFFER_SIZE ];

    //
    // Buffer used for transmitting data. Double buffered so we can
    // queue information to send.
    //
    unsigned char m_TxBuffer1[ DBGCNSL_BUFFER_SIZE ];
    unsigned char m_TxBuffer2[ DBGCNSL_BUFFER_SIZE ];

    //
    // Boolean indicating that we are attached.
    //
    bool    m_Attached;
 
    //
    // Used to control transmit state machine.    
    //
    bool            m_TxOutstanding;
    unsigned int    m_TxBytesQueued;
    unsigned char*  m_CurrentTxAddress;
    bool            m_Buffer1Active;

    //
    // Manages a list of consoles. Is not referece counter.
    //
    DebugConsole*   m_ConsoleListHead;

    //
    // Prevents the display of too many error messages.
    //
    bool            m_OverflowReported;

    //
    // Indicates we want to detached when send completes.
    //
    bool            m_PendingDetach;

    //
    // Set true to enable sync mode operation, no data is lost
    //
    bool            m_bSyncMode;
};

#endif


