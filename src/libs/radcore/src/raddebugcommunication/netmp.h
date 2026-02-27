/* SCEI CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library  Release 1.6
 */
// Version 0.00
// ASCII
// 
// Copyright (C) 1998-1999 Sony Computer Entertainment Inc. All Rights Reserved.
// 
// Version        Date            Design      Log
// --------------------------------------------------------------------
// 0.00           10/13/99        koji        1st version

#ifndef _NETMP_H
#define _NETMP_H

typedef struct {
	u_char code;
	u_char result;
} NETMP_HDR;

/* Message Code */
#define NETMP_CODE_CONNECT	0
#define NETMP_CODE_CONNECTR	1
#define NETMP_CODE_RESET	2
#define NETMP_CODE_RESETR	3
#define NETMP_CODE_MESSAGE	4
#define NETMP_CODE_MESSAGER	5
#define NETMP_CODE_STATUS	6
#define NETMP_CODE_STATUSR	7
#define NETMP_CODE_KILL		8
#define NETMP_CODE_KILLR	9
#define NETMP_CODE_VERSION	10
#define NETMP_CODE_VERSIONR	11

/* Result Code */
#define NETMP_ERR_OK		0
#define NETMP_ERR_INVAL		1
#define NETMP_ERR_BUSY		2
#define NETMP_ERR_NOTCONN	3
#define NETMP_ERR_ALREADYCONN	4
#define NETMP_ERR_NOMEM		5
#define NETMP_ERR_NOPROTO	6

typedef struct {
	u_short reserved;
	u_int param[4];
} NETMP_BOOTP;

typedef struct {
	u_char  pri;
	u_char  reserved;
	u_short proto;
} NETMP_PROTOS;

#define NETMP_PRI_DEFAULT	0x80

#endif 

// Local variables:
// tab-width: 4
// End:
// vi:set tabstop=4:
