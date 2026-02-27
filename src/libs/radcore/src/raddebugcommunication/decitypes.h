#ifndef	DECITYPES_H
#define DECITYPES_H

typedef unsigned char byte;				// 8bit
typedef unsigned short half;			// 16bit
typedef unsigned int word;				// 32bit
typedef struct { word xa[2]; } twin;    // 64bit
typedef struct { word xa[4]; } quad;    // 128bit

#endif // DECITYPES_H