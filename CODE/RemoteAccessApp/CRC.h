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

#pragma once

//#include <windows.h>

class CRC
{
  public:
    CRC(void);
    ~CRC(void);
    void Add(unsigned char data);
    unsigned short Finalize(void);

  private:
    unsigned short crc;
};

/*----------------------- END OF FILE ----------------------------------*/

