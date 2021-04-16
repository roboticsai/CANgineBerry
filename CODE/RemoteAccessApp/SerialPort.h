/**************************************************************************
MODULE:    SerialPort
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

#include "global.h"

class SerialPort
{
public:
  SerialPort();
  ~SerialPort();

  // Connects to a serial port
  // returns handle for success, INVALID_HANDLE_VALUE for error
  HANDLE Connect
  (
    char *PortName,                                          // name of port
    unsigned long Baudrate                                   // baudrate in bps
  );

  // disconnects from the serial port
  void Disconnect
  (
    HANDLE PortHandle                                        // handle of serial port to disconnect from
  );

  // writes a set of bytes to the port
  // returns true for success, false for error
  bool WriteBytes
  (
    HANDLE PortHandle,                                       // handle of port to write to
    UNSIGNED8 *Bytes,                                        // bytes to write
    unsigned long Length,                                    // number of bytes to write
    unsigned long *BytesWritten                              // on return filled with number of bytes written
  );

  // reads a set of bytes from the port
  // returns true for success, false for error
  bool ReadBytes
  (
    HANDLE PortHandle,                                       // handle of port to read from
    UNSIGNED8 *Bytes,                                        // location to store read bytes
    unsigned long Length,                                    // max number of bytes to read
    unsigned long *BytesRead                                 // on return filled with number of bytes read
  );
};
