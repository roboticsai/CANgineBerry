/**************************************************************************
MODULE:    CRC
CONTAINS:  CRC calculation for CANopenIA serial communication
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

#include "CRC.h"

// compatible with SDO block transfers
#define CRCPOLYNOMIAL 0x1021          // x^16 + x^12 + x^5 + 1

CRC::CRC(void)
{
  crc = 0;
}


CRC::~CRC(void)
{
}


/**************************************************************************
DOES:    Adds a byte to the CRC value calculation
RETURNS: Nothing
**************************************************************************/
void CRC::Add(
  unsigned char data                                       // byte to add
  )
{
  unsigned short i, v, xor_flag;

  v = 0x80;

  for (i = 0; i < 8; i++)
  {
    if (crc & 0x8000)
    {
      xor_flag= 1;
    }
    else
    {
      xor_flag= 0;
    }
    crc = crc << 1;

    if (data & v)
    {
      crc = crc + 1;
    }

    if (xor_flag)
    {
      crc = crc ^ CRCPOLYNOMIAL;
    }

    v = v >> 1;
  }
}

/**************************************************************************
DOES:    Finalizes the CRC calculation
RETURNS: CRC value
**************************************************************************/
unsigned short CRC::Finalize(
  void
  )
{
  unsigned short i, xor_flag;

  for (i = 0; i < 16; i++)
  {
    if (crc & 0x8000)
    {
      xor_flag= 1;
    }
    else
    {
      xor_flag= 0;
    }
    crc = crc << 1;

    if (xor_flag)
    {
      crc = crc ^ CRCPOLYNOMIAL;
    }
  }

  return crc;
}

/*----------------------- END OF FILE ----------------------------------*/
