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

#include "global.h"
#include <time.h>
#include <stdio.h>
#include <string.h>
#include "SerialProtocol.h"
#include "CRC.h"


/**************************************************************************
LOCAL DEFINES
***************************************************************************/ 

// com port timeout in milliseconds
#define COM_TIMEOUT 200
// time to wait for command responses in seconds
#define RESPONSE_TIMEOUT 6
// max delay between bytes inside a packet in seconds
#define INTRAPACKET_TIMEOUT 2
// start of packet header byte, must match implementation on device
#define SOH 0x11

#ifdef WIN32
// pause execuction for a specific number of seconds
void System_Sleep
(
  unsigned long milliseconds                               // milliseconds to sleep
)
{
  Sleep(milliseconds);
}
#else
// pause execuction for a specific number of seconds
void System_Sleep
(
  unsigned long milliseconds                               // milliseconds to sleep
)
{
  struct timespec timeOut, remains;

  timeOut.tv_sec = milliseconds / 1000;
  timeOut.tv_nsec = (milliseconds - (timeOut.tv_sec * 1000)) * 1000000;

  nanosleep(&timeOut, &remains);
}
#endif

/**************************************************************************
DOES:    Constructor - performs initialization, connects to COM port
GLOBALS: Reset call back, port, receive state machine
**************************************************************************/
SerialProtocol::SerialProtocol(
  void
  )
{
  // no callbacks
  DataCallback = NULL;
  // serial port not open
  PortHandle = INVALID_HANDLE_VALUE;
  // reset state machine
  ReceiveState = STATE_START;

  // create a timer
  Tim = new Timer();

  // create new XSDO handler
  XSdo = new XSDO();

  // create serial port access
  Port = new SerialPort();

  // create new sdo client handler, register 'send' function to be called on this class instance
  SdoClient = new SDOCLNT(Tim, (SDOCLNTSENDCALLBACK *)&SerialProtocol::SdoClientSendCallback, this);
}


/**************************************************************************
DOES:    Destructor - performs cleanup, disconnects from serial port
GLOBALS: Disconnect serial port
**************************************************************************/
SerialProtocol::~SerialProtocol(
  void
  )
{
  // make sure we disconnect from the serial port
  Disconnect();
}


/**************************************************************************
DOES:    Called when sdo client wants to send an SDO to a node
RETURNS: nothing
**************************************************************************/
void SerialProtocol::SdoClientSend (
  UNSIGNED8 node_id,
  UNSIGNED8 *pData
  )
{
  PACKET Packet;

  // construct and send request packet
  Packet.Data[0] = 'C';
  Packet.Data[1] = node_id;
  memcpy(&(Packet.Data[2]), &(pData[0]), 8);
  Packet.Length = 10;
  SendPacket(&Packet);
}


/**************************************************************************
DOES:    Connects to the serial port
RETURNS: TRUE for success, FALSE for error
**************************************************************************/
bool SerialProtocol::Connect(
  char *PortName,           // serial port to use
  unsigned long Baudrate    // serial baud rate to use (bps)
  )
{
  PortHandle = Port->Connect(PortName, Baudrate);
  if (PortHandle == INVALID_HANDLE_VALUE)
  {
    return FALSE;
  }

  return TRUE;
}


/**************************************************************************
DOES:    Disconnects from the serial port
RETURNS: Nothing
**************************************************************************/
void SerialProtocol::Disconnect(
  void
  )
{
  Port->Disconnect(PortHandle);
  PortHandle = INVALID_HANDLE_VALUE;
}


/**************************************************************************
DOES:    Register a callback for process data written to COIA device
RETURNS: Nothing
**************************************************************************/
void SerialProtocol::RegisterDataCallback(
  DATACALLBACK *Callback,                                  // new callback function or NULL to disable
  void *Param                                              // arbitrary callback parameter
  )
{
  DataCallback = Callback;
  DataCallbackParam = Param;
}


/**************************************************************************
DOES:    Registers callbacks for segmented SDO requests
RETURNS: Nothing
**************************************************************************/
void SerialProtocol::RegisterSDORequestCallbacks
  (
  SDOREQUESTCOMPLETECALLBACK *CompleteCallback             // new callback function or NULL to disable
  )
{
  SdoClient->SdoRequestCompleteCallback = CompleteCallback;
}


/**************************************************************************
DOES:    Checks for receives packets and handles them
RETURNS: Nothing
**************************************************************************/
void SerialProtocol::Process(
  void
  )
{
  PACKET Packet;
  unsigned long DataLength;
  CAN_MSG tx_sdo;

  if (GetPacket(&Packet))
  {
    if (Packet.Length > 0)
    {
      switch (Packet.Data[0])
      {
        // new process data
        case 'D':
          if (DataCallback)
          {
            DataLength = Packet.Length - 5;
            ((DATACALLBACK)DataCallback)(Packet.Data[1], GET_U16(Packet.Data + 2), Packet.Data[4], DataLength, &Packet.Data[5], DataCallbackParam);
          }
          break;

        // SDO segment
        case 'F':
          tx_sdo.LEN = 0;
          XSdo->XSDO_HandleExtended((UNSIGNED8 *)(&Packet.Data[2]), &tx_sdo, Packet.Data[1]);
          if (tx_sdo.LEN == 8)
          { // construct and send response packet
            Packet.Data[0] = 'G';
            // Packet.Data[1], leave at received value
            // set value last to 1, if this was last segment
            if ((tx_sdo.BUF[0] == 0x80) || (tx_sdo.BUF[0] & 1))
              Packet.Data[2] = 1; // last segment
            else
              Packet.Data[2] = 0; // more segments to come
            memcpy(&(Packet.Data[3]), &(tx_sdo.BUF[0]), 8);
            Packet.Length = 11;
            SendPacket(&Packet);
          }
          break;

        // custom sdo request response
        case 'V':
          SdoClient->MGR_HandleSDOClientResponse(Packet.Data[1],(UNSIGNED8 *)(&Packet.Data[2]));
          break;

        // all other packets
        default:
          // must be a command response
          // store and signal
          ResponsePacket   = Packet;
          ResponseReceived = TRUE;
          break;
      }
    }
    else
    {
      printf("Packet with no data! ");
    }
  }

  // work on sdo clients
  SdoClient->MGR_SDOHandleClient();
}


/**************************************************************************
DOES:    If the node is sleeping, transmit something to wake node up
RETURNS: Nothing
**************************************************************************/
void SerialProtocol::WakeUp(
  void
  )
{
  unsigned char Buf[4];
  unsigned long Length = 4;

  // perform read - node will wake up to process serial command 
  // if sleeping, this wakes processor up
  ReadLocalOD(0x1000, 0x00, &Length, Buf);
}


/**************************************************************************
DOES:    Sets the sleep objection state, device will actively object
         to sleep requests
RETURNS: ERROR_NOERROR for success or error code for failure
**************************************************************************/
unsigned long SerialProtocol::SetSleepObjection(
  unsigned char SleepObjectionOn   // TRUE to enable sleep objection, FALSE to disable
  )
{
  return WriteLocalOD(0x5F01, 0x02, 1, &SleepObjectionOn);
}


/**************************************************************************
DOES:    Resets the node
RETURNS: ERROR_NOERROR for success or error code for failure
**************************************************************************/
unsigned long SerialProtocol::Reset
  (
  void
  )
{
  unsigned char Command = RESET_APPLICATION;

  return WriteLocalOD(0x5F01, 0x01, 1, &Command);
}


/**************************************************************************
DOES:    Resets the communication layer in the node
RETURNS: ERROR_NOERROR for success or error code for failure
**************************************************************************/
unsigned long SerialProtocol::ResetCommunicationLayer
  (
  void
  )
{
  unsigned char Command = RESET_COMMUNICATION;

  return WriteLocalOD(0x5F01, 0x01, 1, &Command);
}


/**************************************************************************
DOES:    Reads from the device's object dictionary
         Blocks until response received. Note that callback functions will
         continue to be called while waiting for the response.
RETURNS: ERROR_NOERROR for success or error code for failure
**************************************************************************/
unsigned long SerialProtocol::ReadLocalOD(
  unsigned short Index,         // index of od entry to read
  unsigned char Subindex,       // subindex of od entry to read
  unsigned long *DataLength,    // location to store length of data read
  unsigned char *Data           // location to store data read (must hold at least MAX_PACKET_LENGTH bytes)
  )
{
  PACKET Packet;
  time_t endtime;
  unsigned long b;
  unsigned short errorcode;

  // construct packet
  Packet.Data[0] = 'R';
  STORE_U16(Index, Packet.Data + 1);
  Packet.Data[3] = Subindex;
  Packet.Length = 4;

  // clear received flag
  ResponseReceived = FALSE;
  // send
  if (!SendPacket(&Packet))
  {
    return ERROR_TX;
  }

  endtime = time(NULL) + RESPONSE_TIMEOUT;
  // wait for response
  do
  {
    if (time(NULL) >= endtime)
    {
      return ERROR_NORESPONSE;
    }
    // process packet receive
    Process();
  } while (!ResponseReceived);

  // if wrong response received then something went wrong
  if (ResponsePacket.Data[0] != Packet.Data[0])
  {
    return ERROR_WRONGRESPONSE;
  }

  // check for an error
  errorcode = ResponsePacket.Data[4] | ((unsigned short)ResponsePacket.Data[5] << 8);
  if (errorcode)
  {
    LastNodeError = errorcode;
    return ERROR_NODEERROR;
  }

  // get data length
  *DataLength = ResponsePacket.Length - 6;
  // copy data
  for (b = 0; b < *DataLength; b++) Data[b] = ResponsePacket.Data[6 + b];

  return ResponsePacket.Data[4];
}


/**************************************************************************
DOES:    Reads from a remote device's object dictionary
         Can read any size of data.
         Does not block. SdoRequestCompleteCallback will be called on
         completion
RETURNS: ERROR_NOERROR for success or error code for failure
**************************************************************************/
unsigned long SerialProtocol::ReadRemoteODExtended(
  unsigned char NodeID,
  unsigned short Index,         // index of od entry to read
  unsigned char Subindex,       // subindex of od entry to read
  unsigned long *DataLength,    // location to store length of data read, filled with size of buffer
  unsigned char *Data           // location to store data read
  )
{
  SDOCLIENT *Client = SdoClient->SDOCLNT_Init(NodeID, 0, 0, Data, *DataLength);
  if (SdoClient->SDOCLNT_Read(Client, Index, Subindex) == TRUE)
  {
    return ERROR_NOERROR;
  }

  return ERROR_NORESOURCES;
}


/**************************************************************************
DOES:    Reads from a remote device's object dictionary
         Blocks until response received. Note that callback functions will
         continue to be called while waiting for the response.
RETURNS: ERROR_NOERROR for success or error code for failure
**************************************************************************/
unsigned long SerialProtocol::ReadRemoteOD(
  unsigned char NodeID,         // node id of remote node
  unsigned short Index,         // index of od entry to read
  unsigned char Subindex,       // subindex of od entry to read
  unsigned long *DataLength,    // location to store length of data read
  unsigned char *Data           // location to store data read (must hold at least MAX_PACKET_LENGTH bytes)
  )
{
  PACKET Packet;
  time_t endtime;
  unsigned long b;
  unsigned short errorcode;

  // construct packet
  Packet.Data[0] = 'U';
  Packet.Data[1] = NodeID;
  STORE_U16(Index, Packet.Data + 2);
  Packet.Data[4] = Subindex;
  Packet.Length = 5;

  // clear received flag
  ResponseReceived = FALSE;
  // send
  if (!SendPacket(&Packet))
  {
    return ERROR_TX;
  }

  endtime = time(NULL) + RESPONSE_TIMEOUT;
  // wait for response
  do
  {
    if (time(NULL) >= endtime)
    {
      return ERROR_NORESPONSE;
    }
    // process packet receive
    Process();
  } while (!ResponseReceived);

  // if wrong response received then something went wrong
  if (ResponsePacket.Data[0] != Packet.Data[0])
  {
    return ERROR_WRONGRESPONSE;
  }

  // check for an error
  errorcode = ResponsePacket.Data[5] | ((unsigned short)ResponsePacket.Data[6] << 8);
  if (errorcode)
  {
    LastNodeError = errorcode;
    return ERROR_NODEERROR;
  }

  // get data length
  *DataLength = ResponsePacket.Length - 7;
  // copy data
  for (b = 0; b < *DataLength; b++) Data[b] = ResponsePacket.Data[7 + b];

  return ResponsePacket.Data[5];
}


/**************************************************************************
DOES:    Writes to the device's object dictionary
         Blocks until response received. Note that callback functions will
         continue to be called while waiting for the response.
RETURNS: ERROR_NOERROR for success or error code for failure
**************************************************************************/
unsigned long SerialProtocol::WriteLocalOD(
  unsigned short Index,            // index of od entry to read
  unsigned char Subindex,          // subindex of od entry to read
  unsigned long DataLength,        // length of data to write
  unsigned char *Data              // location of data to write
  )
{
  PACKET Packet;
  time_t endtime;
  unsigned short errorcode;

  // don't allow write of too much data
  if (DataLength > MAX_WRITE_LENGTH)
  {
    return ERROR_NORESOURCES;
  }

  // construct packet
  Packet.Data[0] = 'W';
  STORE_U16(Index, Packet.Data + 1);
  Packet.Data[3] = Subindex;
  memcpy(&Packet.Data[4], Data, DataLength);
  Packet.Length = 4 + DataLength;

  // clear received flag
  ResponseReceived = FALSE;
  // send
  if (!SendPacket(&Packet))
  {
    return ERROR_TX;
  }

  endtime = time(NULL) + RESPONSE_TIMEOUT;
  // wait for response
  do
  {
    if (time(NULL) >= endtime)
    {
      return ERROR_NORESPONSE;
    }
    // process packet receive
    Process();
  } while (!ResponseReceived);

  // if wrong response received then something went wrong
  if (ResponsePacket.Data[0] != Packet.Data[0])
  {
    return ERROR_WRONGRESPONSE;
  }

  // check for an error
  errorcode = ResponsePacket.Data[4] | ((unsigned short)ResponsePacket.Data[5] << 8);
  if (errorcode)
  {
    LastNodeError = errorcode;
    return ERROR_NODEERROR;
  }

  return ResponsePacket.Data[4];
}


/**************************************************************************
DOES:    Writes to a remote device's object dictionary
          Can write any size of data.
          Does not block. SdoRequestCompleteCallback will be called on
          completion
RETURNS: ERROR_NOERROR for success or error code for failure
**************************************************************************/
unsigned long SerialProtocol::WriteRemoteODExtended(
  unsigned char NodeID,
  unsigned short Index,         // index of od entry to write
  unsigned char Subindex,       // subindex of od entry to write
  unsigned long DataLength,     // length of data to write
  unsigned char *Data           // location of data to write
  )
{
  SDOCLIENT *Client = SdoClient->SDOCLNT_Init(NodeID, 0, 0, Data, DataLength);
  if (SdoClient->SDOCLNT_Write(Client, Index, Subindex) == TRUE)
  {
    return ERROR_NOERROR;
  }

  return ERROR_NORESOURCES;
}


/**************************************************************************
DOES:    Writes to the object dictionary of a remote node
         Blocks until response received. Note that callback functions will
         continue to be called while waiting for the response.
RETURNS: ERROR_NOERROR for success or error code for failure
**************************************************************************/
unsigned long SerialProtocol::WriteRemoteOD(
  unsigned char NodeID,            // node id of node to write to
  unsigned short Index,            // index of od entry to read
  unsigned char Subindex,          // subindex of od entry to read
  unsigned long DataLength,        // length of data to write
  unsigned char *Data              // location of data to write
  )
{
  PACKET Packet;
  time_t endtime;
  unsigned short errorcode;

  // don't allow write of too much data
  if (DataLength > (MAX_PACKET_LENGTH - 4))
  {
    return ERROR_NORESOURCES;
  }

  // construct packet
  Packet.Data[0] = 'S';
  Packet.Data[1] = NodeID;
  STORE_U16(Index, Packet.Data + 2);
  Packet.Data[4] = Subindex;
  memcpy(&Packet.Data[5], Data, DataLength);
  Packet.Length = 5 + DataLength;

  // clear received flag
  ResponseReceived = FALSE;
  // send
  if (!SendPacket(&Packet))
  {
    return ERROR_TX;
  }

  endtime = time(NULL) + RESPONSE_TIMEOUT;
  // wait for response
  do
  {
    if (time(NULL) >= endtime)
    {
      return ERROR_NORESPONSE;
    }
    // process packet receive
    Process();
  } while (!ResponseReceived);

  // if wrong response received then something went wrong
  if (ResponsePacket.Data[0] != Packet.Data[0])
  {
    return ERROR_WRONGRESPONSE;
  }

  // check for an error
  errorcode = ResponsePacket.Data[5] | ((unsigned short)ResponsePacket.Data[6] << 8);
  if (errorcode)
  {
    LastNodeError = errorcode;
    return ERROR_NODEERROR;
  }

  return ResponsePacket.Data[5];
}

/**************************************************************************
DOES:    Transmits a data packet to the device
RETURNS: TRUE for success, FALSE for error
**************************************************************************/
bool SerialProtocol::SendPacket(
  PACKET *Packet                                           // location of packet to transmit
  )
{
  unsigned long byteswritten;
  unsigned short CRCValue;
  CRC *crc = new CRC();
  bool WriteResult;

  // if com port is not open, then nothing to do
  if (PortHandle == INVALID_HANDLE_VALUE)
  {
    return FALSE;
  }

  // assemble packet
  TxPacketData[0] = SOH;
  TxPacketData[1] = (unsigned char)Packet->Length;
  crc->Add(TxPacketData[1]);
  for (unsigned long b = 0; b < Packet->Length; b++)
  {
    TxPacketData[2 + b] = Packet->Data[b];
    crc->Add(TxPacketData[2 + b]);
  }
  CRCValue = crc->Finalize();
  TxPacketData[2 + Packet->Length]     = CRCValue & 0xFF;
  TxPacketData[2 + Packet->Length + 1] = (CRCValue >> 8) & 0xFF;

  // transmit packet
  WriteResult = Port->WriteBytes(PortHandle, TxPacketData, Packet->Length + 4, &byteswritten);
  if (!WriteResult)
  {
    return FALSE;
  }

  return TRUE;
}


/**************************************************************************
DOES:    Attempts to get the next message packet from the device
RETURNS: TRUE if packet obtained, else FALSE
**************************************************************************/
bool SerialProtocol::GetPacket(
  PACKET *Packet                                           // location to store packet
  )
{
  unsigned char rxbyte[1];
  unsigned long bytesread;
  unsigned long b;
  unsigned short CRCValue;
  bool ReadResult;

  // if com port is not open, then nothing to do
  if (PortHandle == INVALID_HANDLE_VALUE)
  {
     printf("\nERROR GetPacket: Com port closed\n");
     return FALSE;
  }

  // read next byte from com port
  ReadResult = Port->ReadBytes(PortHandle, rxbyte, 1, &bytesread);
  if (!ReadResult)
  {
    // failed to receive, check for timeout and reset state machine if needed
    if ((time(NULL) >= ReceiveTimeout) && (ReceiveState != STATE_LENGTH) && (ReceiveState != STATE_START))
    {
      printf("\nERROR GetPacket: Read in state %d\n",ReceiveState);
      printf("MSG: 0x%2X 0x%2X 0x%2X 0x%2X\n",IncomingPacket.Data[0],IncomingPacket.Data[1],IncomingPacket.Data[2],IncomingPacket.Data[3]);
      ReceiveState = STATE_START;
    }
    return FALSE;
  }
  
  // nothing read so allow other threads to run
  if (bytesread == 0)
  {
    System_Sleep(0);
    return FALSE;
  }

  if (bytesread != 1)
  {
    // failed to receive, check for timeout and reset state machine if needed
    if ((time(NULL) >= ReceiveTimeout) && (ReceiveState != STATE_LENGTH) && (ReceiveState != STATE_START))
    {
      printf("\nERROR GetPacket: bytesread \n");
      printf("MSG: 0x%2X 0x%2X 0x%2X 0x%2X\n",IncomingPacket.Data[0],IncomingPacket.Data[1],IncomingPacket.Data[2],IncomingPacket.Data[3]);
      ReceiveState = STATE_START;
    }
    return FALSE;
  }

  switch (ReceiveState)
  {
    case STATE_START:
      if (rxbyte[0] == SOH)
      {
        ReceiveState = STATE_LENGTH;
      }
      break;
    case STATE_LENGTH:
      IncomingPacket.Length = rxbyte[0];
      BytesRemaining = rxbyte[0];
      // if no data then skip to checksum
      if (IncomingPacket.Length == 0)
      {
        ReceiveState = STATE_CHECKL;
        IncomingCRC = 0x0000;
      }
      else if (IncomingPacket.Length > MAX_PACKET_LENGTH)
      { // packet is too large for us, start over, wait for next
        ReceiveState = STATE_START;
      }
      else
      {
        ReceiveState = STATE_DATA;
      }
      break;
    case STATE_DATA:
      if (IncomingPacket.Length == BytesRemaining)
      { // first byte, sanity check, is it a supported command byte
        if ( (rxbyte[0] != 'D') && (rxbyte[0] != 'R') && (rxbyte[0] != 'W') && (rxbyte[0] != 'U') && (rxbyte[0] != 'S') && (rxbyte[0] != 'F') && (rxbyte[0] != 'V'))
        { // unkown command, start over
          ReceiveState = STATE_START;
        }
      }
      IncomingPacket.Data[IncomingPacket.Length - BytesRemaining] = rxbyte[0];
      BytesRemaining--;
      if (!BytesRemaining)
      {
        ReceiveState = STATE_CHECKL;
        IncomingCRC = 0x0000;
      }
      break;
    case STATE_CHECKL:
      IncomingCRC |= rxbyte[0];
      ReceiveState = STATE_CHECKH;
      break;
    case STATE_CHECKH:
      IncomingCRC |= ((unsigned short)(rxbyte[0]) << 8);

      // calculate CRC
      CRC *crc = new CRC();
      crc->Add((unsigned char)IncomingPacket.Length);
      for (b = 0; b < IncomingPacket.Length; b++) crc->Add(IncomingPacket.Data[b]);
      CRCValue = crc->Finalize();
      // check CRC matches
      if (CRCValue == IncomingCRC)
      {
        // we have received a complete packet - copy and done
        *Packet = IncomingPacket;
        ReceiveState = STATE_START;
        return TRUE;
      }
      else
      {
        printf("\nERROR GetPacket: CRC fail \n");
        printf("MSG: 0x%2X 0x%2X 0x%2X 0x%2X\n",IncomingPacket.Data[0],IncomingPacket.Data[1],IncomingPacket.Data[2],IncomingPacket.Data[3]);
        ReceiveState = STATE_START;
        return FALSE;
      }
      break;
  }

  // reset timer for receiving bytes inside a packet
  ReceiveTimeout = time(NULL) + INTRAPACKET_TIMEOUT;

  return FALSE;
}

/**************************************************************************
DOES:    Gets the last node error
RETURNS: Returns error code from node
**************************************************************************/
unsigned short SerialProtocol::GetLastNodeError
  (
  void
  )
{
  return LastNodeError;
}

/*----------------------- END OF FILE ----------------------------------*/
