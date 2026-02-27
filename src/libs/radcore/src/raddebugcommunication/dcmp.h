/* SCEI CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library  Release 1.6
 */
// Version 0.00
// ASCII
// 
// Copyright (C) 1999 Sony Computer Entertainment Inc. All Rights Reserved.
// 
// Version        Date            Design      Log
// --------------------------------------------------------------------
// 0.00           10/13/99        koji        1st version

#if !defined(_DCMP_H)
#define _DCMP_H

typedef struct {
	byte type;		/* Message Type */
	byte code;		/* Message Code */
	half unused;		/* (unused) */
} DCMP_HDR;

/* Message Type */
#define DCMP_TYPE_CONNECT	0	/* connect/disconnect req/rep */
#define DCMP_TYPE_ECHO		1	/* echo req/rep */
#define DCMP_TYPE_STATUS	2	/* status is changed */
#define DCMP_TYPE_ERROR		3	/* error report */

/* Message Code for DCMP_TYPE_CONNECT */
#define DCMP_CODE_CONNECT	0	/* connect request */
#define DCMP_CODE_CONNECTR	1	/* connect reply */
#define DCMP_CODE_DISCONNECT	2	/* disconnect request */
#define DCMP_CODE_DISCONNECTR	3	/* disconnect reply */

typedef struct {
	byte result;
	byte unused[3];
} DCMP_CONNECT_DATA;

typedef struct {
	word param[4];
} DCMP_BOOTP;

/* Result Code for DCMP_TYPE_CONNECT */
#define DCMP_ERR_GOOD		0	/* good */
#define DCMP_ERR_INVALDEST	1	/* invalid destination */
#define DCMP_ERR_ALREADYCONN	2	/* already connected */
#define DCMP_ERR_NOTCONNECT	3	/* not connected */

/* Message Code for DCMP_TYPE_ECHO */
#define DCMP_CODE_ECHO		0	/* echo request */
#define DCMP_CODE_ECHOR		1	/* echo reply */

typedef struct {
	half id;			/* identifier */
	half seq;			/* sequence number */
	/* byte data[0-16]; */
} DCMP_ECHO_DATA;

/* Message Code for DCMP_TYPE_STATUS */
#define DCMP_CODE_CONNECTED	0	/* no-connect -> connect */
#define DCMP_CODE_PROTO		1	/* no-proto -> proto */
#define DCMP_CODE_UNLOCKED	2	/* locked -> unlocked */
#define DCMP_CODE_SPACE		3	/* no-space -> space */
#define DCMP_CODE_ROUTE		4	/* no-route -> route */

typedef struct {
	half proto;			/* protocol/node-id */
} DCMP_STATUS_DATA;

/* Message Code for DCMP_TYPE_ERROR */
#define DCMP_CODE_NOROUTE	0	/* no route */
#define DCMP_CODE_NOPROTO	1	/* protocol unreachable */
#define DCMP_CODE_LOCKED	2	/* locked */
#define DCMP_CODE_NOSPACE	3	/* no space */
#define DCMP_CODE_INVALHEAD	4	/* invalid header */
#define DCMP_CODE_NOCONNECT	5	/* not connected */

typedef struct {
	DECI2_HDR orig_hdr;	/* original DECI2 header */
	byte orig_data[16];	/* 1st 128bits of original datagram */
} DCMP_ERROR_DATA;

#endif	/* !_DCMP_H */
