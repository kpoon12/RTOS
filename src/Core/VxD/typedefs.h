#ifndef __TYPEDEFS_H__
#define __TYPEDEFS_H__

/* Includes ------------------------------------------------------------------*/
#include "global_includes.h"

#define NO_TIMEOUT 0xFFFF

#ifndef BYTE
#define BYTE		uint8_t
#define LPBYTE 		BYTE *
#define LPCBYTE 	BYTE const *
#endif

#ifndef VBYTE
#define VBYTE		volatile BYTE
#define LPVBYTE 	VBYTE *
#define LPCVBYTE 	VBYTE const *
#endif

#ifndef WORD
#define WORD		uint16_t
#define LPWORD  	WORD *
#define LPCWORD  	WORD const *
#endif

#ifndef VWORD
#define VWORD		volatile WORD
#define LPVWORD  	VWORD *
#define LPCVWORD  	VWORD const *
#endif

#ifndef DWORD
#define DWORD		uint32_t
#define LPDWORD   	DWORD *
#define LPCDWORD  	DWORD const *
#endif

#ifndef VDWORD
#define VDWORD		volatile DWORD
#define LPVDWORD   	VDWORD *
#define LPCVDWORD  	VDWORD const *
#endif

#define BIT_NONE	(WORD)0
#define BIT_0		(WORD)(1<<0)
#define BIT_1		(WORD)(1<<1)
#define BIT_2		(WORD)(1<<2)
#define BIT_3		(WORD)(1<<3)
#define BIT_4		(WORD)(1<<4)
#define BIT_5		(WORD)(1<<5)
#define BIT_6		(WORD)(1<<6)
#define BIT_7		(WORD)(1<<7)
#define BIT_8		(WORD)(1<<8)
#define BIT_9		(WORD)(1<<9)
#define BIT_A		(WORD)(1<<10)
#define BIT_B		(WORD)(1<<11)
#define BIT_C		(WORD)(1<<12)
#define BIT_D		(WORD)(1<<13)
#define BIT_E		(WORD)(1<<14)
#define BIT_F		(WORD)(1<<15)
#define BIT_ALL		(WORD)0xFF

typedef unsigned int    BOOL;

#ifndef __TRUE
 #define __TRUE         1
#endif
#ifndef __FALSE
 #define __FALSE        0
#endif

#endif

