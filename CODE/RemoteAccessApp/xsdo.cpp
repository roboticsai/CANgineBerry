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

//#include <windows.h>
#include "xsdo.h"

#if ((NR_OF_SDOSERVER == 0) || (NR_OF_SDOSERVER > 127))
#error Illegal value for NR_OF_SDOSERVER
#endif

/**************************************************************************
GLOBAL VARIABLES
***************************************************************************/ 

// Fatal Error return value
#define TXLOST_SDO 0x0770

// states for state machine
#define STAT_NONE    0
#define STAT_READ    1
#define STAT_WRITE   2
#define STAT_WRITECB 4

#define BLK_STAT_WRITE  8   // sdo block transfer in progress (download/write)
#define BLK_STAT_READ   16  // sdo block transfer in progress (upload/read)
#define BLK_STAT_WRCONF 32  // sdo block transfer completed
#define BLK_STAT_RDGO   64  // sdo block transfer in progress (upload/read)
#define BLK_STAT_RDCONF 128 // sdo block transfer completed


/**************************************************************************
DOES:    Constructor - performs initialization
**************************************************************************/
XSDO::XSDO(
  void
  )
{
}


/**************************************************************************
LOCAL FUNCTIONS
***************************************************************************/

/**************************************************************************
DOES:    Marks message as transmitted
RETURNS: nothing
**************************************************************************/
void XSDO::MCOHW_PushMessage (
  CAN_MSG *pTx
  )
{ 
  // set length field to 8 to indicate transmission
  pTx->LEN = 8;
}


/**************************************************************************
DOES:    Generates an SDO Abort Response
RETURNS: nothing
**************************************************************************/
void XSDO::MCO_SendSDOAbort (
  CAN_MSG *pTxCAN,
  UNSIGNED32 ErrorCode  // 4 byte SDO abort error code
  )
{
  UNSIGNED8 i;

  // construct message data
  pTxCAN->BUF[0] = 0x80;
  for (i=0;i<4;i++)
  {
    pTxCAN->BUF[4+i] = (UNSIGNED8)ErrorCode;
    ErrorCode >>= 8;
  }

  // transmit message
  MCOHW_PushMessage(pTxCAN);
}


/**************************************************************************
DOES: Common exit routine for SDO_Handler. 
      Send SDO response with write confirmation.
      Assumes that ID, LEN and BUF[1-3] are already set
**************************************************************************/
void XSDO::XSDO_WriteConfirm (
  CAN_MSG *pTxCAN
  )
{
UNSIGNED8 i;

  // Load SDO Response into transmit buffer
  pTxCAN->BUF[0] = 0x60; // Write response code
  // Clear unused bytes
  for (i = 4; i < 8; i++)
  {
    pTxCAN->BUF[i] = 0;
  }
    
  // Transmit SDO Response message
  MCOHW_PushMessage(pTxCAN);
}


/**************************************************************************
DOES: Common exit routine for SDO_Handler, segmented write
      Send SDO response with write confirmation for segmented transfer
**************************************************************************/
void XSDO::XSDO_WriteSegConfirm (
  CAN_MSG *pTxCAN,
  UNSIGNED8 sdoserv
  )
{
UNSIGNED8 i;

  // Load SDO Response into transmit buffer
  pTxCAN->BUF[0] = mXSDO[sdoserv].tog & 0x10; // Copy toggle bit
  pTxCAN->BUF[0] += 0x20;
  
  // Clear unused bytes
  for (i = 1; i < 8; i++)
  {
    pTxCAN->BUF[i] = 0;
  }
    
  // Transmit SDO Response message
  MCOHW_PushMessage(pTxCAN);

  mXSDO[sdoserv].tog = ~mXSDO[sdoserv].tog; // Toggle the toggle bit
}


/*******************************************************************************
DOES:    Writes the next segment of segmented SDO transfer to the 
         destination buffer.
RETURNS: TRUE if no error occured during the segment
         FALSE if a major error occured and the transfer needs to be aborted
*******************************************************************************/
UNSIGNED8 XSDO::XSDO_WriteNextSegment (
  UNSIGNED8 last, // Set to 1 if this is the last segment
  UNSIGNED8 len, // length of segment (0-7)
  UNSIGNED8 *pDat, // pointer to 'len' data bytes
  UNSIGNED8 sdoserv // SDO server from 0 to NR_OF_SDOSERVERS-1
  )
{
  while (len > 0)
  { // Process byte-by-byte of segment
    if (mXSDO[sdoserv].size == 0)
    { // end of buffer reached
      return FALSE;
    }
    // Copy data
    *mXSDO[sdoserv].pDat = *pDat;
    // Increment pointers
    pDat++;
    mXSDO[sdoserv].pDat++;
    // Decrement local and overall length counter
    len--;
    mXSDO[sdoserv].size--;

    if (mXSDO[sdoserv].pBuf != 0)
    { // Entry handled by application
      // Check for destiation buffer overrun
      mXSDO[sdoserv].bufcnt++;
      if ((mXSDO[sdoserv].bufcnt >= mXSDO[sdoserv].bufsize) || (mXSDO[sdoserv].size == 0))
      { // reached end of destination buffer or end of transfer
        // Restore buffer pointer
        mXSDO[sdoserv].pDat = mXSDO[sdoserv].pBuf;
        // Reset counter
        mXSDO[sdoserv].bufcnt = 0;
      }
    }
  }
  return TRUE;
}


/*******************************************************************************
DOES:    Reads the next segment of segmented SDO transfer from the
         source buffer.
*******************************************************************************/
void XSDO::XSDO_ReadNextSegment (
  UNSIGNED8 *pDat, // pointer to SDO data bytes
  UNSIGNED8 sdoserv // sdo server from 0 to NR_OF_SDOSERVERS-1
  )
{
UNSIGNED8 *p1st;
UNSIGNED8 len;

  // Init variables
  p1st = pDat; // remember pointer to first byte
  len = 7;
  pDat++; // start at byte 1 not zero

  while ((len > 0) && (mXSDO[sdoserv].size > 0))
  {
    // Copy data
    *pDat = *mXSDO[sdoserv].pDat;
    // Increment pointers
    pDat++;
    mXSDO[sdoserv].pDat++;
    // Decrement local and overall length counter

    len--;
    mXSDO[sdoserv].size--;
  }

  if (mXSDO[sdoserv].size == 0)
  {
    // Reset data buffer pointer
    if (mXSDO[sdoserv].size > 0)
    {
      mXSDO[sdoserv].pDat = mXSDO[sdoserv].pBuf;
    }
    while ((len > 0) && (mXSDO[sdoserv].size > 0))
    {
      // Copy data
      *pDat = *mXSDO[sdoserv].pDat;
      // Increment pointers
      pDat++;
      mXSDO[sdoserv].pDat++;
      // Decrement local and overall length counter

      len--;
      mXSDO[sdoserv].size--;
    }
  }

  // Now calculate contents of 1st byte
  *p1st = ((mXSDO[sdoserv].tog & 1) << 4) + (len << 1);
  if (mXSDO[sdoserv].size == 0)
  { // end of all segmented data reached
    *p1st |= 0x01; // set "last segment" bit
    mXSDO[sdoserv].state = STAT_NONE; // transfer completed
  }
}


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
UNSIGNED8 XSDO::XSDO_HandleSegmented (
  UNSIGNED16 index, // Current index (if known)
  UNSIGNED8 *pRxData, // SDO Request
  UNSIGNED8 *pTxData, // SDO Response
  UNSIGNED8 SDOsrv // SDO server number from 0 to NR_OF_SDOSERVERS-1
  )
{
UNSIGNED8 ret_val = 0; 
UNSIGNED32 totalsize = 0;

  // Check if SDO Segmented Download (Write) Transfer in progress 
  if ((mXSDO[SDOsrv].state == STAT_WRITE) || (mXSDO[SDOsrv].state == STAT_WRITECB))
  { 
    // Check if command specifier is right
    if (pRxData[0] & 0xE0)
    {
      mXSDO[SDOsrv].state = STAT_NONE;
      return 0xF0; // Report general error
    }
    if (((*pRxData & 0x10) >> 4) != (mXSDO[SDOsrv].tog & 0x01))
    {
      mXSDO[SDOsrv].state = STAT_NONE;
      return 0x02; // Report toggle error
    }

    if ( XSDO_WriteNextSegment( *pRxData & 0x01, 
                                 7 - ((*pRxData & 0x0E) >> 1), 
                                 &(pRxData[1]),
                                 SDOsrv ) != TRUE
       )
    {
      // received data too big
      mXSDO[SDOsrv].state = STAT_NONE;
      return 0x03; // Report access error
    }
  
    if (*pRxData & 0x01)
    { // last segment
      mXSDO[SDOsrv].state = STAT_NONE; // transfer completed
    }

    return 1; // all OK
  }
  
  // Check if SDO Segmented Upload (Read) Transfer in progress 
  if (mXSDO[SDOsrv].state == STAT_READ)
  { 
    // Check if command specifier is right
    if ((pRxData[0] & 0xE0) != 0x60)
    {
      mXSDO[SDOsrv].state = STAT_NONE;
      return 0xF0; // Report general error
    }
    if (((*pRxData & 0x10) >> 4) != (mXSDO[SDOsrv].tog & 0x01))
    {
      mXSDO[SDOsrv].state = STAT_NONE;
      return 0x02; // Report toggle error
    }
    XSDO_ReadNextSegment(pTxData,SDOsrv);

    mXSDO[SDOsrv].tog = ~mXSDO[SDOsrv].tog; // Toggle the toggle bit
    return 0x20; // Transmit pTXData as response
  }

  // Check if this is a new download (write) request
  if ((pRxData[0] & 0xF0) == 0x20) 
  { // This is SDO download, segmented or expedited transfer
  }

  // Check if this is an new upload (read) request
  if (pRxData[0] == 0x40)
  { // This is SDO upload, init segmented transfer
  }

  // Nothing was done here
  return 0;
}  


/**************************************************************************
PUBLIC FUNCTIONS
***************************************************************************/


/**************************************************************************
DOES:    Process SDO Segmented Requests to generic OD entries
RETURNS: 0x00 Nothing was done
         0x01 OK, handled, response generated
         0x02 Abort, SDO Abort was generated
**************************************************************************/
UNSIGNED8 XSDO::XSDO_HandleExtended (
  UNSIGNED8 *pReqBUF, // Pointer to 8 data bytes with SDO data from request
  CAN_MSG *pResCAN, // Pointer to SDO response
  UNSIGNED8 SDOServer // Number of SDO Server (< NR_OF_SDOSERVER)
  )
{
UNSIGNED16 index;   // Index of SDO request
UNSIGNED8  ret_val; // Return value

#if (NR_OF_SDOSERVER > 1)
  if (SDOServer >= NR_OF_SDOSERVER)
  { // Fatal error from caller, parameter out of range
    return 0x00;
  }
#endif

  // Copy Multiplexor into response
  pResCAN->BUF[1] = pReqBUF[1]; // index lo
  pResCAN->BUF[2] = pReqBUF[2]; // index hi
  pResCAN->BUF[3] = pReqBUF[3]; // subindex

  // Check for abort
  if (*pReqBUF == 0x80)
  { // Abort code received
    // reset state machine for segmented transfers
    mXSDO[SDOServer].state = STAT_NONE;
    return 0; // simply ignore the abort received
  }

  // Conformance check on cmd 0xE0
  if (((*pReqBUF & 0xE0) == 0xE0) 
      || (*pReqBUF == 0xA0)
     )
  {
    // reset state machine for segmented transfers
    mXSDO[SDOServer].state = STAT_NONE;
    MCO_SendSDOAbort(pResCAN,SDO_ABORT_UNKNOWN_COMMAND);
    return 2;
  }

  // Get requested index
  index = pReqBUF[2]; 
  index = (index << 8) + pReqBUF[1]; 

  ret_val = XSDO_HandleSegmented(index,pReqBUF,(UNSIGNED8 *)&(pResCAN->BUF[0]),SDOServer);
  switch (ret_val)
  {
  case 0: // Nothing found
    break;
  case 1: 
    XSDO_WriteSegConfirm(pResCAN,SDOServer);
    return 1;
  case 2:
    MCO_SendSDOAbort(pResCAN,SDO_ABORT_TOGGLE);
    return 2;
  case 3:
    MCO_SendSDOAbort(pResCAN,SDO_ABORT_TRANSFER);
    return 2;
  case 4:
    MCO_SendSDOAbort(pResCAN,SDO_ABORT_READONLY);
    return 2;
  case 5:
    MCO_SendSDOAbort(pResCAN,SDO_ABORT_WRITEONLY);
    return 2;
  case 6:
    MCO_SendSDOAbort(pResCAN,SDO_ABORT_NOT_EXISTS);
  return 2;
  case 0x10: 
    XSDO_WriteConfirm(pResCAN);
    return 1;
  case 0x20: 
  case 0x21: 
    // Transmit SDO Response message
    MCOHW_PushMessage(pResCAN);
    return 1;
  default:
    MCO_SendSDOAbort(pResCAN,SDO_ABORT_GENERAL);
    return 2;
  }

  // Nothing done here
  return 0;
}


/*----------------------- END OF FILE ----------------------------------*/
