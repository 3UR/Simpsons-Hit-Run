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

#if !defined(_DECI2X_H)
#define _DECI2X_H

typedef struct {
	half length;		/* Packet Length (includes this header) */
	half reserved;		/* (reserved) */
	half protocol;		/* Protocol */
	byte source;		/* Source Node ID */
	byte destination;	/* Destination Node ID */
} DECI2_HDR;

/* length */
#define DECI2_MAX_LEN		(0x10000 - 1)

/* Node ID (source, destination) */
#define DECI2_NODE_IOP		'I'	/* (0x49) IOP */
#define DECI2_NODE_EE		'E'	/* (0x45) EE */
#define DECI2_NODE_HOST		'H'	/* (0x48) Host */

/* protocol */
#define DECI2_PROTO_ANY		0x0000	/* Any */
#define DECI2_PROTO_DCMP	0x0001	/* Control Message */
#define DECI2_PROTO_I0TTYP	0x0110	/* IOP TTY 0 */
#define DECI2_PROTO_I1TTYP	0x0111	/* IOP TTY 1 */
#define DECI2_PROTO_I2TTYP	0x0112	/* IOP TTY 2 */
#define DECI2_PROTO_I3TTYP	0x0113	/* IOP TTY 3 */
#define DECI2_PROTO_I4TTYP	0x0114	/* IOP TTY 4 */
#define DECI2_PROTO_I5TTYP	0x0115	/* IOP TTY 5 */
#define DECI2_PROTO_I6TTYP	0x0116	/* IOP TTY 6 */
#define DECI2_PROTO_I7TTYP	0x0117	/* IOP TTY 7 */
#define DECI2_PROTO_I8TTYP	0x0118	/* IOP TTY 8 */
#define DECI2_PROTO_I9TTYP	0x0119	/* IOP TTY 9 */
#define DECI2_PROTO_IKTTYP	0x011f	/* IOP Kernel TTY */
#define DECI2_PROTO_DRFP0	0x0120	/* IOP File (host0:) for EE */
#define DECI2_PROTO_DRFP1	0x0121	/* IOP File (host1:) for IOP */
#define DECI2_PROTO_DRFP2	0x0122	/* IOP File (host2:) for CONF */
#define DECI2_PROTO_ISDBGP	0x0130	/* IOP System Debugging */
#define DECI2_PROTO_ITDBGP	0x0140	/* IOP Thread Debugging */
#define DECI2_PROTO_ILOADP	0x0150	/* IOP Load */
#define DECI2_PROTO_E0TTYP	0x0210	/* EE  TTY 0 */
#define DECI2_PROTO_E1TTYP	0x0211	/* EE  TTY 1 */
#define DECI2_PROTO_E2TTYP	0x0212	/* EE  TTY 2 */
#define DECI2_PROTO_E3TTYP	0x0213	/* EE  TTY 3 */
#define DECI2_PROTO_E4TTYP	0x0214	/* EE  TTY 4 */
#define DECI2_PROTO_E5TTYP	0x0215	/* EE  TTY 5 */
#define DECI2_PROTO_E6TTYP	0x0216	/* EE  TTY 6 */
#define DECI2_PROTO_E7TTYP	0x0217	/* EE  TTY 7 */
#define DECI2_PROTO_E8TTYP	0x0218	/* EE  TTY 8 */
#define DECI2_PROTO_E9TTYP	0x0219	/* EE  TTY 9 */
#define DECI2_PROTO_EKTTYP	0x021f	/* EE  Kernel TTY */
#define DECI2_PROTO_ESDBGP	0x0230	/* EE  System Debugging */
#define DECI2_PROTO_ETDBGP	0x0240	/* EE  Thread Debugging */
#define DECI2_PROTO_NETMP	0x0400	/* Net Manager */
#define DECI2_PROTO_NTTYP	0x0410	/* Net TTY */

#define DECI2_PROTO_ITTYP	DECI2_PROTO_I0TTYP
#define DECI2_PROTO_ETTYP	DECI2_PROTO_E0TTYP

/* 0x0000-0x0fff SCE use */
/* 0x1000-0xdfff Tool licensee use */
/* 0xe000-0xefff Local use for licensee */
/* 0xf000-0xffff reserved */

#endif	/* !_DECI2_H */
