//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        debugconsoleprotocol.hpp
//
// Subsystem:	Radical Debug Console System - Host Communication Protocol
//
// Description:	This file contains the definitions of the packets used for
//              communication between the host and the target.
//
// Revisions:	V1.00	Dec 11, 2000      Creation
//	                    Feb 02, 2001      Added HostRecvReady for flow control
//
//=============================================================================

#ifndef	DEBUGCONSOLEPROTOCOL_HPP
#define DEBUGCONSOLEPROTOCOL_HPP

//=============================================================================
// Include Files
//=============================================================================

//=============================================================================
// Defintions
//=============================================================================

//
// This is the protocol number we use for connect with the host via the IOP
//
#define HOST_DEBUG_CONSOLE_PROTOCOL (0xE123 + 5)         

//
// This constant govers the size of the buffer used for transfering data.
// between target and host.
//
#define DBGCNSL_BUFFER_SIZE  (10 * 1024)

//
// This enumeration defines the various commands we use in the protocol, The
// protocol is very simple. Simple packets are used.
//
enum HdcpCommand
{
    HdcsCreate,                 
    HdcsTitle,
	HdcsBackgroundColor,
	HdcsClear,
    HdcsCursorPosition,
	HdcsTextColor,
	HdcsTextOutAt,
    HdcsTextOut,
	HdcsClose,
	HdcsKeyboardInput,
    HdcsVirtualKeyInput,
	HdcsPointerInput,
	HdcsClientRecvReady,
    HdcsHostRecvReady
};

struct HdcpCreateCmd
{
    HdcpCommand     m_Command; 
    void*           m_ConsoleId;
};	

struct HdcpTitleCmd
{
    HdcpCommand     m_Command; 
    void*           m_ConsoleId;
    char            m_Title[ 256 ];    // Title of window, null terminated.
};

struct HdcpBackgroundColorCmd
{
    HdcpCommand     m_Command; 
    void*           m_ConsoleId;
    unsigned int    m_BackgroundColor;                
};

struct HdcpClearCmd
{
    HdcpCommand     m_Command; 
    void*           m_ConsoleId;
};

struct HdcpCursorPositionCmd
{
    HdcpCommand     m_Command; 
    void*           m_ConsoleId;
	unsigned int    m_XPosition;
	unsigned int    m_YPosition;
};

struct HdcpKeyboardInputCmd
{
    HdcpCommand     m_Command; 
    void*           m_ConsoleId;
	unsigned int    m_AsciiKey;
};

struct HdcpVirtualKeyInputCmd
{
    HdcpCommand     m_Command; 
    void*           m_ConsoleId;
	unsigned int    m_VirtualKey;
    char            m_Ctrl;
    char            m_Shift;
    char            m_Alt;
};

enum HdcpInputMouseEventType
{
    HdcpMouseMove,
    HdcpMouseLeftButtonDown,
    HdcpMouseLeftButtonUp,
    HdcpMouseLeftButtonClick
};

struct HdcpPointerInputCmd
{
    HdcpCommand     m_Command; 
    void*           m_ConsoleId;              
	unsigned int    m_XTextPixels;
	unsigned int    m_YTextPixels;
	unsigned int    m_XScreenPixels;
	unsigned int    m_YScreenPixels;
    HdcpInputMouseEventType            m_Type;
    char            m_Ctrl;
    char            m_Shift;
    char            m_Alt;
    char            m_LButton;
};

struct HdcpTextColorCmd
{
    HdcpCommand     m_Command; 
    void*           m_ConsoleId;              
    unsigned int    m_TextColor;                
};

struct HdcpTextOutAtCmd
{
    HdcpCommand     m_Command; 
    void*           m_ConsoleId;              
    unsigned int    m_XPosition;
	unsigned int    m_YPosition;
	char            m_Text[ 84 ];
};

struct HdcpTextOutCmd
{
    HdcpCommand     m_Command; 
    void*           m_ConsoleId;              
	char            m_Text[ 84 ];
};

struct HdcpCloseCmd
{
    HdcpCommand     m_Command; 
    void*           m_ConsoleId;              
};

struct HdcpClientRecvReadyCmd
{
    HdcpCommand     m_Command; 
};

struct HdcpHostRecvReadyCmd
{
    HdcpCommand     m_Command; 
};

#endif

