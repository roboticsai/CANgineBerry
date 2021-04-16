/**************************************************************************
MODULE:    XSDO
CONTAINS:  CANopenIA extended SDO functionality
COPYRIGHT: Embedded Systems Academy, Inc. 2008-2017.
DISCLAIM:  Read and understand our disclaimer before using this code!
           www.esacademy.com/disclaim.htm
           This software was written in accordance to the guidelines at
           www.esacademy.com/software/softwarestyleguide.pdf
LICENSE:   Free to use with licensed CANopenIA chips, modules or devices
           like CANgineBerry, CANgineXXX and 447izer
VERSION:   1.10, EmSA 10-NOV-17
           $LastChangedDate: 2013-07-05 16:46:50 +0200 (Fr, 05 Jul 2013) $
           $LastChangedRevision: 1573 $
***************************************************************************/ 

#ifndef _XSDO_H
#define _XSDO_H

#include "global.h"

#define USE_EXTENDED_SDO 1
#define NR_OF_SDOSERVER 17


/**************************************************************************
DEFINES FOR ACCESS TYPE TO OD ENRIES
Readable, Writable, Read-Mapable, Write-Mapable, Call-Back
**************************************************************************/
#define ODRD 0x10
#define ODWR 0x20
#define RMAP 0x40
#define WMAP 0x80
#define CALB 0x08

class XSDO
{
  public:

    /**************************************************************************
    DOES:    Constructor - performs initialization
    **************************************************************************/
    XSDO(
      void
      );
    /**************************************************************************
    DOES:    Process SDO Segmented Requests to generic OD entries
    RETURNS: 0x00 Nothing was done
             0x01 OK, handled, response generated
             0x02 Abort, SDO Abort was generated
    **************************************************************************/
    UNSIGNED8 XSDO_HandleExtended (
      UNSIGNED8 *pReqBUF,                                  // Pointer to 8 data bytes with SDO data from request
      CAN_MSG *pResCAN,                                    // Pointer to SDO response
      UNSIGNED8 SDOServer                                  // Number of SDO Server (1 to NR_OF_SDOSERVER)
      );
    /**************************************************************************
    DOES:    Called from ProcessStackTick
             Checks if we are in middle of Block Read transfer
    RETURNS: FALSE, nothing done
             TRUE, transfer in progress, message generated
    **************************************************************************/
    UNSIGNED8 XSDO_BlkRdProgress (
      void
      );

  private:
    /**************************************************************************
    DOES:    Marks message as transmitted
    RETURNS: nothing
    **************************************************************************/
    void MCOHW_PushMessage (
      CAN_MSG *pTx
      );
    /**************************************************************************
    DOES:    Generates an SDO Abort Response
    RETURNS: nothing
    **************************************************************************/
    void MCO_SendSDOAbort (
      CAN_MSG *pTxCAN,
      UNSIGNED32 ErrorCode                                 // 4 byte SDO abort error code
      );
    /**************************************************************************  
    DOES: Common exit routine for SDO_Handler. 
          Send SDO response with write confirmation.
          Assumes that ID, LEN and BUF[1-3] are already set
    **************************************************************************/
    void XSDO_WriteConfirm (
      CAN_MSG *pTxCAN
      );
    /**************************************************************************
    DOES: Common exit routine for SDO_Handler, segmented write
          Send SDO response with write confirmation for segmented transfer
    **************************************************************************/
    void XSDO_WriteSegConfirm (
      CAN_MSG *pTxCAN,
      UNSIGNED8 sdoserv
      );
    /*******************************************************************************
    DOES:    Writes the next segment of segmented SDO transfer to the 
             destination buffer.
    RETURNS: TRUE if no error occured during the segment
             FALSE if a major error occured and the transfer needs to be aborted
    *******************************************************************************/
    UNSIGNED8 XSDO_WriteNextSegment (
      UNSIGNED8 last,                                      // Set to 1 if this is the last segment
      UNSIGNED8 len,                                       // length of segment (0-7)
      UNSIGNED8 *pDat,                                     // pointer to 'len' data bytes
      UNSIGNED8 sdoserv                                    // SDO server from 0 to NR_OF_SDOSERVERS-1
      );
    /*******************************************************************************
    DOES:    Reads the next segment of segmented SDO transfer from the
             source buffer.
    *******************************************************************************/
    void XSDO_ReadNextSegment (
      UNSIGNED8 *pDat,                                     // pointer to SDO data bytes
      UNSIGNED8 sdoserv                                    // sdo server from 0 to NR_OF_SDOSERVERS-1
      );
    /**************************************************************************
    DOES:    Process segmented SDO Requests to generic OD entries
    RETURNS: 0x01 all OK
             0x02 toggle error
             0x03 received data too big
             0x04 do a SDO_ABORT_READONLY
             0x05 do a SDO_ABORT_WRITEONLY
             0x06 do a SDO_ABORT_NOT_EXISTS
             0x07 do a SDO_ABORT_DATATOBIG
             0x20 pTxData contains SDO response to send
             0xF0 command specifier error
    **************************************************************************/
    UNSIGNED8 XSDO_HandleSegmented (
      UNSIGNED16 index,                                    // Current index (if known)
      UNSIGNED8 *pRxData,                                  // SDO Request
      UNSIGNED8 *pTxData,                                  // SDO Response
      UNSIGNED8 SDOsrv                                     // SDO server number from 0 to NR_OF_SDOSERVERS-1
      );
    // SDO status info
    struct
    {
      UNSIGNED32 size;                                     // Current data length
      UNSIGNED32 bufsize;                                  // Max data length of buffer
      UNSIGNED32 bufcnt;                                   // Count in buffer
      UNSIGNED16 idx;                                      // Index
      UNSIGNED8 *pBuf;                                     // Buffer base pointer
      UNSIGNED8 *pDat;                                     // Running data pointer
      UNSIGNED8 sub;                                       // Subindex
      UNSIGNED8 state;                                     // State machine
      UNSIGNED8 tog;                                       // Toggle bit
    } mXSDO[NR_OF_SDOSERVER];
};


#endif // _XSDO_H
/**************************************************************************
END OF FILE
**************************************************************************/
