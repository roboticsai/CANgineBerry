
#ifndef WIN32

#include <errno.h>
#include <fcntl.h> 
#include <string.h>
#include <termios.h>
#include <unistd.h>
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
  HANDLE fd = open(PortName, O_RDWR | O_NOCTTY | O_SYNC);
  speed_t speed;
  
  if (fd < 0)
  {
    char *errormsg = strerror(errno);
    fprintf(stderr, "ERROR: %d opening %s: %s\n", errno, PortName, errormsg);
    return INVALID_HANDLE_VALUE;
  }

  struct termios tty;
  memset(&tty, 0, sizeof tty);
  if (tcgetattr(fd, &tty) != 0)
  {
    fprintf(stderr, "ERROR: %d from tcgetattr\n", errno);
    return INVALID_HANDLE_VALUE;
  }

  switch (Baudrate)
  {
    case 1200:
      speed = B1200;
      break;

    case 2400:
      speed = B2400;
      break;

    case 4800:
      speed = B4800;
      break;

    case 9600:
      speed = B9600;
      break;

    case 19200:
      speed = B19200;
      break;

    case 38400:
      speed = B38400;
      break;

#ifdef B57600
    case 57600:
      speed = B57600;
      break;
#endif
#ifdef B115200
    case 115200:
      speed = B115200;
      break;
#endif
#ifdef B230400
    case 230400:
      speed = B230400;
      break;
#endif
#ifdef B460800
    case 460800:
      speed = B460800;
      break;
#endif
#ifdef B500000
    case 500000:
      speed = B500000;
      break;
#endif
#ifdef B576000
    case 576000:
      speed = B576000;
      break;
#endif
#ifdef B921600
    case 921600:
      speed = B921600;
      break;
#endif
#ifdef B100000
    case 100000:
      speed = B100000;
      break;
#endif
#ifdef B1152000
    case 1152000:
      speed = B1152000;
      break;
#endif
#ifdef B1500000
    case 1500000:
      speed = B1500000;
      break;
#endif
#ifdef B2000000
    case 2000000:
      speed = B2000000;
      break;
#endif
#ifdef B2500000
    case 2500000:
      speed = B2500000;
      break;
#endif
#ifdef B3000000
    case 3000000:
      speed = B3000000;
      break;
#endif
#ifdef B3500000
    case 3500000:
      speed = B3500000;
      break;
#endif
#ifdef B4000000
    case 4000000:
      speed = B4000000;
      break;
#endif
    default:
      fprintf(stderr, "ERROR: Baudrate %ld is not supported\n", Baudrate);
      return INVALID_HANDLE_VALUE;
  }
  
  cfsetospeed(&tty, speed);
  cfsetispeed(&tty, speed);

  tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
                                                  // disable IGNBRK for mismatched speed tests; otherwise receive break
                                                  // as \000 chars

  // Input flags - Turn off input processing
  //
  // convert break to null byte, no CR to NL translation,
  // no NL to CR translation, don't mark parity errors or breaks
  // no input parity check, don't strip high bit off,
  // no XON/XOFF software flow control
  tty.c_iflag &= ~(IGNBRK | BRKINT | ICRNL | INLCR |
                   PARMRK | INPCK | ISTRIP | IXON | IXOFF | IXANY);

  tty.c_lflag = 0;                // no signaling chars, no echo,
                                  // no canonical processing
  tty.c_oflag = 0;                // no remapping, no delays
  tty.c_cc[VMIN] = 0;            // read doesn't block
  tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

  tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
                                  // enable reading
  tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
  tty.c_cflag |= 0; // parity
  tty.c_cflag &= ~CSTOPB;
  tty.c_cflag &= ~CRTSCTS;

  if (tcsetattr(fd, TCSANOW, &tty) != 0)
  {
    fprintf(stderr, "ERROR: %d from tcsetattr\n", errno);
    return INVALID_HANDLE_VALUE;
  }

  return fd;
}

// disconnects from the serial port
void SerialPort::Disconnect
  (
  HANDLE PortHandle                                        // handle of serial port to disconnect from
  )
{
  // if not a valid handle then nothing we can do
  if (PortHandle == INVALID_HANDLE_VALUE) return;

  close(PortHandle);
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
  ssize_t Result = write(PortHandle, Bytes, Length);
  if (Result == -1)
  {
    return FALSE;
  }
  if ((unsigned long)Result < Length)
  {
    return FALSE;
  }

  *BytesWritten = (unsigned long)Result;
  return TRUE;
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
  ssize_t Result = read(PortHandle, Bytes, Length);
  if (Result == -1)
  {
    return FALSE;
  }

  *BytesRead = (unsigned long)Result;
  return TRUE;
}

#endif // !WIN32
