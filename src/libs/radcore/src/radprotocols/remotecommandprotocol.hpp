//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        remotecommandprotool.Hpp
//
// Subsystem:	Radical Remote Command Server - Host Communication Protocol
//
// Description:	This file contains the definitions of the packets used for
//              communication between the host and the target.
//
// Revisions:	06-Feb-2001      Creation
//
//=============================================================================

#ifndef	RREMOTECOMMANDPROTO_HPP
#define RREMOTECOMMANDPROTO_HPP

//=============================================================================
// Include Files
//=============================================================================

#include <radremotecommand.hpp>

//=============================================================================
// Defintions
//=============================================================================

//
// This is the protocol number we use for connect with the host via the IOP
//
#define HOST_REMOTE_COMMAND_PROTOCOL (0xE666)         

//
// This constant govers the size of the buffer used for transfering data.
// between target and host.
//
#define RCS_BUFFER_SIZE  (10 * 1024)

//
// These constants how long the string of args can be
//
#define RCS_MAX_ARG_LENGTH 256

//
// This enumeration defines the various commands we use in the protocol, The
// protocol is very simple. Simple packets are used.
//
enum HrcsCommand
{
    HrcsList,
	HrcsListReply,
	HrcsListItem,
	HrcsRemoteFunction,
	HrcsResult
};

struct HrcsListCommand
{
    HrcsCommand     m_Command; 
};	

struct HrcsRemoteFunctionCommand
{
	HrcsCommand		m_Command;
	char			m_FunctionName[40];
	int				m_argc;
	char			m_argv[RCS_MAX_ARG_LENGTH];
};

struct HrcsResultCommand
{
	HrcsCommand		m_Command;
	HrcsResultCode  m_ResultCode;
};

struct HrcsListReplyCommand
{
	HrcsCommand		m_Command;
	unsigned int	m_NumItems;
};

struct HrcsListItemCommand
{
	HrcsCommand		m_Command;
	char			m_FunctionName[40];
};


#endif

