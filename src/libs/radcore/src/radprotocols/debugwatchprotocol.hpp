//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        debugwatchprotocol.hpp
//
// Subsystem:	Radical Watch System - Host Communication Protocol
//
// Description:	This file contains all definitions shared between the host
//				watch server and the runtime libraries
//
// Author:		Peter Mielcarski
//
// Revisions:	V1.00	Jan 17, 2001      Creation
//
//=============================================================================

#ifndef	WATCHPROTOCOL_HPP
#define WATCHPROTOCOL_HPP

//=============================================================================
// Include Files
//=============================================================================

//=============================================================================
// Defintions
//=============================================================================

//
// This is the protocol number we use for communication between the host and the
// target.
//
#define WATCH_PROTOCOL (0xEEE3)         

//
// This is the size of the largest packet we will ever send between the host 
// and the target.
//
#define WP_MAX_PACKET (8 * 1024)

//
// The protocol is very simple. Simple operations are queued into large buffers
// and transmitted.
//
enum WPCommand
{
	WPCNameSpace,				// Specify default name space. First name space received is default - sent by target
	WPCCreate,					// Creation of a watchpoint - sent by target
	WPCCreateEnum,				// Creation of a watchpoint - sent by target
	WPCDelete,					// Destruction of a watchpoint - sent by target
	WPCValue,					// Current value of a watchpoint - sent by target
	WPCRequestValue,			// Sent by the host to request value 
	WPCWriteValue,				// Send by host to update value
    WPCPacketReceived,          // Send by target to acknowlege receipt of data.
    WPCHostRequest,             // Send by target to issue generic request.
    WPCAssociateFile            // Send by target to associate file with name space
};

//
// Represents the various data types.
//
enum WPDataType
{
	WPDUnsignedInt,
	WPDInt,
	WPDUnsignedShort,
	WPDShort,
	WPDUnsignedChar,
	WPDChar,
	WPDBoolean,
	WPDFloat,
	WPDEnum,
	WPDString,
	WPDVector,
    WPDFunction
};

//
// These constants govern the maximum lenght a name and namespace can be.
// Just arbritary.
//
#define WP_MAX_NAMELENGTH		32
#define	WP_MAX_NAMESPACELENGTH	256
#define WP_MAX_FILENAMELENGTH   128

//=============================================================================
// Stucture Declarations
//=============================================================================

struct WCNameSpace						// Defines namespace of next create.  Initial one is default namespace
{
	WPCommand		m_Command;	
	char	        m_NameSpace[ WP_MAX_NAMESPACELENGTH ];
};

struct WCCreate							// Watchpoint create, except for enum
{
	WPCommand		m_Command;	
	WPDataType		m_DataType;	
    void            (*m_Callback)( void* );
    void*           m_UserData;
	char	        m_Name[ WP_MAX_NAMELENGTH ];
	void*			m_Address;
    bool            m_ReadOnly;
	union
	{
		struct
		{
			unsigned int	m_Max;
			unsigned int	m_Min;
		} m_ui;
		struct
		{
			int				m_Max;
			int				m_Min;
		} m_i;
		struct
		{
			unsigned short	m_Max;
			unsigned short	m_Min;
		} m_us;
		struct
		{
			short			m_Max;
			short			m_Min;
		} m_s;
		struct 
		{
			unsigned char	m_Max;
			unsigned char	m_Min;
		} m_uc;
		struct 
		{
			char			m_Max;
			char			m_Min;
		} m_c;
		struct 
		{
			float			m_Max;
			float			m_Min;
		} m_f;
		struct 
		{	
			unsigned int	m_Length;
		} m_string;
		struct 
		{
			float			m_Max;
			float			m_Min;
		} m_vector;
	} m_Attrib;
};

struct WCCreateEnum							// Used to create enumeration
{
	WPCommand			m_Command;	
    void                (*m_Callback)( void* );
    void*               m_UserData;
	char		        m_Name[ WP_MAX_NAMELENGTH ];
	void*				m_Address;
	unsigned int		m_Length;			// Length of enumeration data immedidately following
    bool                m_ReadOnly;
};

struct WCDelete								// Delete a watch point
{
	WPCommand			m_Command;	
	void*				m_Address;
};

union WCData
{
	struct
	{
		unsigned int	m_Value;
	}m_ui;
	struct
	{
		int				m_Value;
	}m_i;
	struct
	{
		unsigned short	m_Value;
	}m_us;
	struct
	{
		short			m_Value;
	}m_s;
	struct
	{
		unsigned char	m_Value;
	}m_uc;
	struct
	{
		char			m_Value;
	}m_c;
	struct
	{
		float			m_Value;
	}m_f;
	struct
	{	
		unsigned int	m_Length;	// Data will follow this structure
	} m_string;
	struct 
	{
		float			m_x;
		float			m_y;
		float			m_z;
	} m_vector;
	struct
	{
		int 			m_Value;
	} m_bool;
	struct 
	{
		int				m_Value;
	} m_enum;
};

struct WCValue								// Data sent by target
{
	WPCommand		m_Command;	
	WPDataType		m_DataType;	
	void*			m_Address;
	WCData			m_Attrib;
};

struct WCRequestValue							// Used to ask for value
{
	WPCommand			m_Command;	
	WPDataType			m_DataType;	
	void*				m_Address;
};

struct WCWriteValue								// Data sent by host
{
	WPCommand		m_Command;	
	WPDataType		m_DataType;	
    void            (*m_Callback)( void* );
    void*           m_UserData;
	void*			m_Address;
	WCData			m_Attrib;
};

//
// This packet is sent by the target when it receives a packet form the host.
// It is used to add flow control.
//
struct WCPacketReceived
{
	WPCommand		m_Command;	
};

//
// This packet is used to send data from the game to the host application. Currently
// not used by game tuner. Data follows command.
//
struct WCHostRequest
{
	WPCommand		m_Command;	
    unsigned int    m_Id;
    unsigned int    m_Length;
};

//
// Packet used for associating file with namespace.
//
struct WCAssociateFile
{
	WPCommand		m_Command;	
    unsigned int    m_FileType;     // 0 = xml, 1 = #define, 2 = const
	char	        m_NameSpace[ WP_MAX_NAMESPACELENGTH ];
    char            m_Filename[ WP_MAX_FILENAMELENGTH ];
};

#endif 
