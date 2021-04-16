/**************************************************************************
MODULE:    SerialProtocol
CONTAINS:  Protocol handler for CANopenIA serial communication
COPYRIGHT: Embedded Systems Academy, Inc. 2008-2017.
DISCLAIM:  Read and understand our disclaimer before using this code!
           www.esacademy.com/disclaim.htm
           This software was written in accordance to the guidelines at
           www.esacademy.com/software/softwarestyleguide.pdf
LICENSE:   Free to use with licensed CANopenIA chips, modules or devices
           like CANgineBerry, CANgineXXX and 447izer
VERSION:   1.10, EmSA 10-NOV-17
           $LastChangedDate: 2014-01-11 17:30:59 +0100 (Sa, 11 Jan 2014) $
           $LastChangedRevision: 1702 $
***************************************************************************/ 
#pragma once

#ifndef _SERIALPROTOCOL_H
#define _SERIALPROTOCOL_H

#include <stdint.h>
#include <time.h>
#include "global.h"
#include "xsdo.h"
#include "sdoclnt.h"
#include "Timer.h"
#include "SerialPort.h"

/**************************************************************************
GLOBAL DEFINES
***************************************************************************/ 

// error codes
#define ERROR_NOERROR              0
#define ERROR_ODENTRYNOTFOUND      (1UL << 0)
#define ERROR_INVALIDCOMMANDLENGTH (1UL << 1)
#define ERROR_INVALIDCOMMAND       (1UL << 2)
#define ERROR_COIABUSY             (1UL << 3)
#define ERROR_NORESOURCES          (1UL << 4)
#define ERROR_TXBUFFERFULL         (1UL << 5)
#define ERROR_TRANSFERABORTED      (1UL << 6)
#define ERROR_RXBUFFERTOOSMALL     (1UL << 7)
#define ERROR_SDOTOGGLEERROR       (1UL << 8)
#define ERROR_SDOTIMEOUT           (1UL << 9)
#define ERROR_UNKNOWN              (1UL << 10)
#define ERROR_NOTSUPPORTED         (1UL << 11)
#define ERROR_NODEERROR            (1UL << 12)
#define ERROR_TX                   (1UL << 29)
#define ERROR_NORESPONSE           (1UL << 30)
#define ERROR_WRONGRESPONSE        (1UL << 31)

// node status values
#define NODESTATUS_BOOT         0x00
#define NODESTATUS_STOPPED      0x04
#define NODESTATUS_OPERATIONAL  0x05
#define NODESTATUS_PREOP        0x7F
#define NODESTATUS_EMCY_NEW     0x80
#define NODESTATUS_EMCY_OVER    0x81
#define NODESTATUS_HBACTIVE     0x90
#define NODESTATUS_HBLOST       0x91
#define NODESTATUS_SCANSTARTED  0x9F
#define NODESTATUS_SCANCOMPLETE 0xA0
#define NODESTATUS_SCANABORTED  0xA8
#define NODESTATUS_RESETAPP     0xB0
#define NODESTATUS_RESETCOM     0xB1
#define NODESTATUS_SLEEP        0xF0
#define NODESTATUS_BOOTLOADER   0xF1

// hardware status values
#define HWSTATUS_NONE           0
#define HWSTATUS_INITALIZING    (1UL << 0)
#define HWSTATUS_CANERROR       (1UL << 1)
#define HWSTATUS_ERRORPASSIVE   (1UL << 2)
#define HWSTATUS_RXQUEUEOVERRUN (1UL << 3)
#define HWSTATUS_TXQUEUEOVERRUN (1UL << 4)
#define HWSTATUS_SUPPORTSCANFD  (1UL << 5)
#define HWSTATUS_TXBUSY         (1UL << 6)
#define HWSTATUS_BUSOFF         (1UL << 7)

// host control values [5F09,00]
#define HOSTCONTROL_NONE          0
#define HOSTCONTROL_OBJECTTOSLEEP (1UL << 0)

// NMT reset comands [5F01,01]
#define RESET_APPLICATION   129
#define RESET_COMMUNICATION 130

// constants, must match implementation of device
#define MAX_PACKET_LENGTH (28 + 7)
// max data that can be written to local OD in one go
#define MAX_WRITE_LENGTH (MAX_PACKET_LENGTH - 4)

/**************************************************************************
GLOBAL TYPES AND FUNCTIONS
***************************************************************************/ 

// data types
typedef struct _Packet
{
  unsigned long Length;
  unsigned char Data[MAX_PACKET_LENGTH];
} PACKET;

class SerialProtocol
{
  // Parsing states for packet protocol reception
  typedef enum {
    STATE_START,    // Start-of-header
    STATE_LENGTH,   // Length of data
    STATE_DATA,     // Data bytes
    STATE_CHECKL,   // Checksum low
    STATE_CHECKH    // Checksum high
  } STATE;

  public:
    /**************************************************************************
    DOES:    Constructor - performs initialization, connects to COM port
    GLOBALS: Reset call back, port, receive state machine
    **************************************************************************/
    SerialProtocol(void);
    /**************************************************************************
    DOES:    Destructor - performs cleanup, disconnects from serial port
    GLOBALS: Disconnect serial port
    **************************************************************************/
    ~SerialProtocol(void);
    /**************************************************************************
    DOES:    Checks for receives packets and handles them
    RETURNS: Nothing
    **************************************************************************/
    void Process(void);
    /**************************************************************************
    DOES:    Register a callback for process data written to COIA device
    RETURNS: Nothing
    **************************************************************************/
    void RegisterDataCallback(DATACALLBACK *Callback, void *Param);
    /**************************************************************************
    DOES:    Registers callbacks for segmented SDO requests
    RETURNS: Nothing
    **************************************************************************/
    void RegisterSDORequestCallbacks(SDOREQUESTCOMPLETECALLBACK *CompleteCallback);
    /**************************************************************************
    DOES:    Connects to the serial port
    RETURNS: TRUE for success, FALSE for error
    **************************************************************************/
    bool Connect(char *PortName, unsigned long Baudrate);
    /**************************************************************************
    DOES:    Disconnects from the serial port
    RETURNS: Nothing
    **************************************************************************/
    void Disconnect(void);
    /**************************************************************************
    DOES:    Reads from the device's object dictionary
             Blocks until response received. Note that callback functions will
             continue to be called while waiting for the response.
    RETURNS: ERROR_NOERROR for success or error code for failure
    **************************************************************************/
    unsigned long ReadLocalOD(unsigned short Index, unsigned char Subindex, unsigned long *DataLength, unsigned char *Data);
    /**************************************************************************
    DOES:    Reads from a remote device's object dictionary
             Blocks until response received. Note that callback functions will
             continue to be called while waiting for the response.
    RETURNS: ERROR_NOERROR for success or error code for failure
    **************************************************************************/
    unsigned long ReadRemoteOD(unsigned char NodeID, unsigned short Index, unsigned char Subindex, unsigned long *DataLength, unsigned char *Data);
    /**************************************************************************
    DOES:    Reads from a remote device's object dictionary
             Can read any size of data.
             Does not block. SdoRequestCompleteCallback will be called on
             completion
    RETURNS: ERROR_NOERROR for success or error code for failure
    **************************************************************************/
    unsigned long ReadRemoteODExtended(
      unsigned char NodeID,
      unsigned short Index,         // index of od entry to read
      unsigned char Subindex,       // subindex of od entry to read
      unsigned long *DataLength,    // location to store length of data read, filled with size of buffer
      unsigned char *Data           // location to store data read
      );
    /**************************************************************************
    DOES:    Writes to the device's object dictionary
             Blocks until response received. Note that callback functions will
             continue to be called while waiting for the response.
    RETURNS: ERROR_NOERROR for success or error code for failure
    **************************************************************************/
    unsigned long WriteLocalOD(unsigned short Index, unsigned char Subindex, unsigned long DataLength, unsigned char *Data);
    /**************************************************************************
    DOES:    Writes to the object dictionary of a remote node
             Blocks until response received. Note that callback functions will
             continue to be called while waiting for the response.
    RETURNS: ERROR_NOERROR for success or error code for failure
    **************************************************************************/
    unsigned long WriteRemoteOD(unsigned char NodeID, unsigned short Index, unsigned char Subindex, unsigned long DataLength, unsigned char *Data);
    /**************************************************************************
    DOES:    Writes to a remote device's object dictionary
             Can write any size of data.
             Does not block. SdoRequestCompleteCallback will be called on
             completion
    RETURNS: ERROR_NOERROR for success or error code for failure
    **************************************************************************/
    unsigned long WriteRemoteODExtended(
      unsigned char NodeID,
      unsigned short Index,         // index of od entry to write
      unsigned char Subindex,       // subindex of od entry to write
      unsigned long DataLength,     // length of data to write
      unsigned char *Data           // location of data to write
      );
    /**************************************************************************
    DOES:    If the node is sleeping, transmit something to wake node up
    RETURNS: Nothing
    **************************************************************************/
    void WakeUp(void);
    /**************************************************************************
    DOES:    Sets the sleep objection state, device will actively object
             to sleep requests
    RETURNS: ERROR_NOERROR for success or error code for failure
    **************************************************************************/
    unsigned long SetSleepObjection(unsigned char SleepObjectionOn);
    /**************************************************************************
    DOES:    Resets the node
    RETURNS: ERROR_NOERROR for success or error code for failure
    **************************************************************************/
    unsigned long Reset(void);
    /**************************************************************************
    DOES:    Resets the communication layer in the node
    RETURNS: ERROR_NOERROR for success or error code for failure
    **************************************************************************/
    unsigned long ResetCommunicationLayer(void);
    /**************************************************************************
    DOES:    Gets the last node error
    RETURNS: Returns error code from node
    **************************************************************************/
    unsigned short GetLastNodeError(void);

  private:
    /**************************************************************************
    DOES:    Transmits a data packet to the device
    RETURNS: TRUE for success, FALSE for error
    **************************************************************************/
    bool SendPacket(PACKET *Packet);
    /**************************************************************************
    DOES:    Attempts to get the next message packet from the device
    RETURNS: TRUE if packet obtained, else FALSE
    **************************************************************************/
    bool GetPacket(PACKET *Packet);
    /**************************************************************************
    DOES:    Called when sdo client wants to send an SDO to a node
    RETURNS: nothing
    **************************************************************************/
    void SdoClientSend (
      UNSIGNED8 node_id,
      UNSIGNED8 *pData
    );

    // callback wrapper - redirects to instance of serial protocol class
    static void SdoClientSendCallback(UNSIGNED8 node_id, UNSIGNED8 *pData, void *param)
    { ((SerialProtocol *)param)->SdoClientSend(node_id, pData); }

    DATACALLBACK *DataCallback;
    void *DataCallbackParam;
    volatile bool ResponseReceived;
    PACKET ResponsePacket;
    volatile bool InitResponseReceived;
    PACKET InitResponsePacket;
    PACKET IncomingPacket;
    HANDLE PortHandle;
    STATE ReceiveState;
    unsigned long BytesRemaining;
    unsigned short IncomingCRC;
    time_t ReceiveTimeout;
    XSDO *XSdo;
    SDOCLNT *SdoClient;
    Timer *Tim;
    unsigned char TxPacketData[MAX_PACKET_LENGTH + 4];
    SerialPort *Port;
    unsigned short LastNodeError;
};


#endif // _SERIALPROTOCOL_H

/*----------------------- END OF FILE ----------------------------------*/
