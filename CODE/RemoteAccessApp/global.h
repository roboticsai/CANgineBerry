// global definitions

#ifndef _GLOBAL_H
#define _GLOBAL_H

#ifndef WIN32
#include <linux/limits.h>
#endif

#ifdef WIN32
#include <Windows.h>
#else
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifdef WIN32
typedef void *HANDLE;
typedef HANDLE *PHANDLE;

#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE ((HANDLE)(LONG_PTR)-1)
#endif
#else
typedef int HANDLE;
typedef HANDLE *PHANDLE;

#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE ((HANDLE)-1)
#endif
#endif

#ifndef NULL
#ifdef __cplusplus
#define NULL 0
#else
#define NULL ((void *)0)
#endif
#endif

// data types
#define UNSIGNED8 unsigned char
#define UNSIGNED16 unsigned short
#define UNSIGNED32 unsigned int

#ifndef WIN32
#ifndef MAX_PATH
#define MAX_PATH PATH_MAX
#endif
#endif

// callback functions
typedef void(*DATACALLBACK)(unsigned char NodeID, int Index, unsigned char Subindex, unsigned long DataLength, unsigned char *Data, void *Param);
typedef void(*SDOREQUESTCOMPLETECALLBACK)(UNSIGNED8 nodeid, UNSIGNED32 abortcode);

// macros to get and store multi-byte values in little-endian format
#define STORE_U16(value, loc) (loc)[0] = (unsigned char)((value) & 0xFF); (loc)[1] = (unsigned char)(((value) >> 8) & 0xFF);
#define STORE_U32(value, loc) (loc)[0] = (unsigned char)((value) & 0xFF); (loc)[1] = (unsigned char)(((value) >> 8) & 0xFF); (loc)[2] = (unsigned char)(((value) >> 16) & 0xFF); (loc)[3] = (unsigned char)(((value) >> 24) & 0xFF);
#define GET_U16(loc) ((unsigned short)((loc)[1]) << 8) | ((loc)[0])
#define GET_U32(loc) ((unsigned long)((loc)[3]) << 24) | ((unsigned long)((loc)[2]) << 16) | ((unsigned long)((loc)[1]) << 8) | (unsigned long)((loc)[0])

typedef unsigned short COBID_TYPE;

// Data structure for a single CAN message 
typedef struct
{ // order optimized for allignment
  UNSIGNED8 BUF[8];              // Data buffer 
  COBID_TYPE ID;                 // Message Identifier 
  UNSIGNED8 LEN;                 // Data length (0-8)
} CAN_MSG;

// SDO Abort codes
#define SDO_ABORT_TOGGLE          0x05030000UL
#define SDO_ABORT_UNKNOWN_COMMAND 0x05040001UL
#define SDO_ABORT_INVALID_SEQ     0x05040003UL
#define SDO_ABORT_CRC             0x05040004UL
#define SDO_ABORT_UNSUPPORTED     0x06010000UL
#define SDO_ABORT_WRITEONLY       0x06010001UL
#define SDO_ABORT_READONLY        0x06010002UL
#define SDO_ABORT_NOT_EXISTS      0x06020000UL
#define SDO_ABORT_PARAMETER       0x06040043UL
#define SDO_ABORT_TYPEMISMATCH    0x06070010UL
#define SDO_ABORT_DATATOBIG       0x06070012UL
#define SDO_ABORT_UNKNOWNSUB      0x06090011UL
#define SDO_ABORT_VALUE_RANGE     0x06090030UL
#define SDO_ABORT_VALUE_HIGH      0x06090031UL
#define SDO_ABORT_VALUE_LOW       0x06090032UL
#define SDO_ABORT_GENERAL         0x08000000UL
#define SDO_ABORT_TRANSFER        0x08000020UL
#define SDO_ABORT_NOTRANSFERCTRL  0x08040021UL
#define SDO_ABORT_NOTMAPPED       0x06040041UL
#define SDO_ABORT_MAPLENGTH       0x06040042UL
#define SDO_ABORT_TIMEOUT         0x05040000UL
#define SDO_ABORT_NOTRANSFER      0x08040020UL
#define SDO_ABORT_NOMEM           0x05040005UL

#endif // _GLOBAL_H
