/**************************************************************************
MODULE:    SerialPort_Windows
CONTAINS:  Low-level serial handler for CANopenIA serial communication
COPYRIGHT: Embedded Systems Academy, Inc. 2017.
DISCLAIM:  Read and understand our disclaimer before using this code!
www.esacademy.com/disclaim.htm
This software was written in accordance to the guidelines at
www.esacademy.com/software/softwarestyleguide.pdf
LICENSE:   Free to use with licensed CANopenIA chips, modules or devices
like CANgineBerry, CANgineXXX and 447izer
VERSION:   1.10, EmSA 10-NOV-17
$LastChangedDate: 2017-11-15 11:48:10 +0000 (Wed, 15 Nov 2017) $
$LastChangedRevision: 4117 $
***************************************************************************/

#ifdef WIN32

#include <stdio.h>
#include "SerialPort.h"

SerialPort::SerialPort()
{
}

SerialPort::~SerialPort()
{
}

// Connects to a serial port
// returns handle for success, INVALID_HANDLE_VALUE for error
HANDLE SerialPort::Connect
  (
  char *PortName,                                          // name of port
  unsigned long Baudrate                                   // baudrate in bps
  )
{
  char resource[20];
  DCB mydcb;
  COMMTIMEOUTS mytimeouts;

  // create resource string
  sprintf(resource, "\\\\.\\COM%u", strtol((const char *)PortName, (char **)NULL, 10));

  // open com port
  HANDLE SerialPort = CreateFile(resource, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
    FILE_ATTRIBUTE_NORMAL, NULL);

  // if failed to open then report error
  if (SerialPort == INVALID_HANDLE_VALUE)
  {
    return SerialPort;
  }

  // it is necessary to call getcommstate and modify certain elements rather than
  // calling buildcommdcb, as the latter method fails on windows 2000
  if (!GetCommState(SerialPort, &mydcb))
  {
    CloseHandle(SerialPort);
    SerialPort = INVALID_HANDLE_VALUE;
    return SerialPort;
  }

  // specify new com port configuration
  mydcb.fBinary = TRUE;
  mydcb.fParity = FALSE;
  mydcb.fOutxCtsFlow = FALSE;
  mydcb.fOutxDsrFlow = FALSE;
  mydcb.fDsrSensitivity = FALSE;
  mydcb.fOutX = FALSE;
  mydcb.fInX = FALSE;
  mydcb.fNull = FALSE;
  mydcb.BaudRate = Baudrate;
  mydcb.Parity = NOPARITY;
  mydcb.StopBits = ONESTOPBIT;
  mydcb.ByteSize = 8;

  // configure com port
  if (!SetCommState(SerialPort, &mydcb))
  {
    CloseHandle(SerialPort);
    SerialPort = INVALID_HANDLE_VALUE;
    return SerialPort;
  }

  // set up timeout configuration
  // for reads we don't use a timeout so ReadFile will always return immediately
  mytimeouts.ReadIntervalTimeout = MAXDWORD;
  mytimeouts.ReadTotalTimeoutMultiplier = 0;
  mytimeouts.ReadTotalTimeoutConstant = 0;
  mytimeouts.WriteTotalTimeoutMultiplier = 0;
  mytimeouts.WriteTotalTimeoutConstant = 0;
  // configure timeouts
  if (!SetCommTimeouts(SerialPort, &mytimeouts))
  {
    CloseHandle(SerialPort);
    SerialPort = INVALID_HANDLE_VALUE;
    return SerialPort;
  }

  return SerialPort;
}

// disconnects from the serial port
void SerialPort::Disconnect
  (
  HANDLE PortHandle                                        // handle of serial port to disconnect from
  )
{
  // if not a valid handle then nothing we can do
  if (PortHandle == INVALID_HANDLE_VALUE) return;

  // close com port
  CloseHandle(PortHandle);
}

// writes a set of bytes to the port
// returns true for success, false for error
bool SerialPort::WriteBytes
  (
  HANDLE PortHandle,                                       // handle of port to write to
  UNSIGNED8 *Bytes,                                        // bytes to write
  unsigned long Length,                                    // number of bytes to write
  unsigned long *BytesWritten                              // on return filled with number of bytes written
  )
{
  return WriteFile(PortHandle, Bytes, Length, BytesWritten, NULL);
}

// reads a set of bytes from the port
// returns true for success, false for error
bool SerialPort::ReadBytes
  (
  HANDLE PortHandle,                                       // handle of port to read from
  UNSIGNED8 *Bytes,                                        // location to store read bytes
  unsigned long Length,                                    // max number of bytes to read
  unsigned long *BytesRead                                 // on return filled with number of bytes read
  )
{
  return ReadFile(PortHandle, Bytes, Length, BytesRead, NULL);
}

#endif // WIN32
