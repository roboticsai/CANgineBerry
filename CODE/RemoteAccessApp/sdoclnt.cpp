/**************************************************************************
MODULE:    SDOCLNT
CONTAINS:  CANopenIA extended SDO client handling
COPYRIGHT: Embedded Systems Academy, Inc. 2008-2017.
DISCLAIM:  Read and understand our disclaimer before using this code!
           www.esacademy.com/disclaim.htm
           This software was written in accordance to the guidelines at
           www.esacademy.com/software/softwarestyleguide.pdf
LICENSE:   Free to use with licensed CANopenIA chips, modules or devices
           like CANgineBerry, CANgineXXX and 447izer
VERSION:   1.10, EmSA 10-NOV-17
           $LastChangedDate: 2014-05-08 16:31:23 +0100 (Thu, 08 May 2014) $
           $LastChangedRevision: 3122 $
***************************************************************************/ 

#include "sdoclnt.h"
#include <stdio.h>
#include <string.h>

// SDO client back-to-back transmit timeout in milliseconds
#define SDO_BACK2BACK_TIMEOUT 3
// Default SDO client timeout in milliseconds
#define SDO_REQUEST_TIMEOUT 150

// SDO block transfer max number of blocks (4 to 127)
#define SDO_BLK_MAX_SIZE 127

/**************************************************************************
DOES:    Constructor - resets all SDO Client channels
RETURNS: nothing
***************************************************************************/ 
SDOCLNT::SDOCLNT (
  Timer *Tim,                                              // timer instance
  SDOCLNTSENDCALLBACK *SendCallback,                       // callback function for sending
  void *SenderInstance                                     // instance of sender
  )
{
  for (mCurrentChannel = 0; mCurrentChannel < NR_OF_SDO_CLIENTS; mCurrentChannel++)
  {
    mSDOClientList[mCurrentChannel].status     = SDOCL_FREE;
    mSDOClientList[mCurrentChannel].channel    = mCurrentChannel+1;
    mSDOClientList[mCurrentChannel].last_abort = 0;
  }

  // no callbacks
  SdoRequestCompleteCallback = NULL;

  // store pointer to timer
  this->Tim = Tim;

  // store details of how to send
  this->SendCallback   = SendCallback;
  this->SenderInstance = SenderInstance;
}


/**************************************************************************
DOES:    Sends a message
RETURNS: TRUE if successful, FALSE if error
**************************************************************************/
UNSIGNED8 SDOCLNT::MCOHW_PushMessage (
  UNSIGNED8 node_id,
  CAN_MSG *pTx
  )
{
  if (SenderInstance && SendCallback)
  {
    ((SDOCLNTSENDCALLBACK)SendCallback)(node_id, &pTx->BUF[0], SenderInstance);
    return TRUE;
  }

  return FALSE;
}


/**************************************************************************
DOES:    Generates an SDO abort message for a specific SDO channel
GLOBALS: Assumes that the sdomsg is already initalized properly from last
         request.
**************************************************************************/
UNSIGNED8 SDOCLNT::SDOCLNT_SendSDOAbort (
  SDOCLIENT *p_client, // Pointer to initialized SDO client structure
  UNSIGNED32 ErrorCode // 4 byte SDO abort error code
  )
{
UNSIGNED8 *pDest;
UNSIGNED8 i;
UNSIGNED8 retval = FALSE;;

  if (p_client != 0)
  {

    // Prepare SDO abort message
    pDest = &(p_client->sdomsg.BUF[0]);
    *pDest = 0x80; // BUF[0], Command Specifier for Abort
    pDest += 4;
    for (i=0;i<4;i++)
    {
      *pDest = (UNSIGNED8) ErrorCode;
      ErrorCode >>= 8;
      pDest++;
    }

    // Transmit SDO Abort message
    retval = MCOHW_PushMessage(p_client->channel, &(p_client->sdomsg));
  }

  return retval;
}


/**************************************************************************
Description in comgr.h
***************************************************************************/ 
void SDOCLNT::SDOCLNT_SDOComplete (
  UNSIGNED8 channel, // SDO channel number in range of 1 to NR_OF_SDO_CLIENTS
  UNSIGNED32 abort_code // status, error, abort code
  )
{
  if (abort_code == SDOERR_OK)
  {
    mSDOClientList[channel-1].last_abort = 0;
  }
  else
  {
    mSDOClientList[channel-1].last_abort = abort_code;
  }

  // Execute call-back
  if (SdoRequestCompleteCallback)
  {
    ((SDOREQUESTCOMPLETECALLBACK)SdoRequestCompleteCallback)(channel, abort_code);
  }
}


/**************************************************************************
Description in sdoclnt.h
***************************************************************************/ 
SDOCLIENT *SDOCLNT::SDOCLNT_Init (
  UNSIGNED8 channel, // SDO channel number in range of 1 to NR_OF_SDO_CLIENTS
  UNSIGNED32 canid_request, // CAN message ID used for the SDO request
  UNSIGNED32 canid_response, // CAN message ID used for the SDO response
  UNSIGNED8 *p_buf, // data buffer pointer for data exchanged
  UNSIGNED32 buf_size // max length of data buffer
  )
{
SDOCLIENT *pClient = 0;

  channel--;
  if (channel < NR_OF_SDO_CLIENTS)
  { 
    // Init pointer to this client's structure
    pClient = &(mSDOClientList[channel]);

    // Copy configuration into structure
    pClient->canid_request = canid_request;
    pClient->canid_response = canid_response;
    pClient->bufmax = buf_size;
    pClient->buflen = buf_size;
    pClient->curlen = 0;
    pClient->pBuf = p_buf;
    pClient->channel = channel+1;
    pClient->status = SDOCL_READY;
    pClient->last_abort = 0xFFFFFFFF;
    pClient->timeout_reload = SDO_REQUEST_TIMEOUT;

  }

  return pClient;
}


/**************************************************************************
Description in sdoclnt.h
***************************************************************************/ 
UNSIGNED8 SDOCLNT::SDOCLNT_Write (
  SDOCLIENT *p_client, // Pointer to initialized SDO client structure
  UNSIGNED16 index, // Object Dictionary Index to write
  UNSIGNED8 subindex // Object Dictionary Subindex to write
  )
{
UNSIGNED8 *pDest; // Destination Pointer
UNSIGNED8 *pSrc; // Source Pointer
UNSIGNED16 loop; // Loop counter

  if ( (p_client == 0) ||
       ((p_client->status & SDOCL_READY) != SDOCL_READY)
     )
  {
    return FALSE;
  }

  // Erase last abort
  p_client->last_abort = 0xFFFFFFFF;

  // Prepare SDO request message
  p_client->sdomsg.ID = p_client->canid_request;
  p_client->sdomsg.LEN = 8;
  // copy multiplexor
  pDest = &(p_client->sdomsg.BUF[1]);
  *pDest = (UNSIGNED8) index; // BUF[1], Lo-Byte Index
  pDest++;
  *pDest = (UNSIGNED8) (index >> 8); // BUF[2], Hi-Byte Index
  pDest++;
  *pDest = subindex; // BUF[3], Subindex
  // Save index and subindex
  p_client->index = index;
  p_client->subindex = subindex;

  pDest = &(p_client->sdomsg.BUF[0]);
  if (p_client->buflen <= 4)
  { // this can be done with one expedited transfer
    // BUF[0], Command Specifier for Write
    *pDest = 0x23 + ((4-p_client->buflen) << 2); 

    pDest = &(p_client->sdomsg.BUF[4]);
    pSrc = p_client->pBuf;
    for (loop = 0; loop < 4; loop++)
    {
      if (loop < p_client->buflen)
      {
        *pDest = *pSrc; 
      }
      else
      {
        *pDest = 0; // fill unused bytes with zero
      }
      pDest++;
      pSrc++;
    }
    p_client->status = SDOCL_READY;
  }
#if USE_BLOCKED_SDO_CLIENT
  else if ((p_client->buflen > 28))
  { // block transfer only makes sense when at least 4 segments are used
    *pDest = (6 << 5) | 0x02; // BUF[0], Command Specifier for Write Block
    pDest++;
    *pDest = (UNSIGNED8) index; // BUF[1], Lo-Byte Index
    pDest++;
    *pDest = (UNSIGNED8) (index >> 8); // BUF[2], Hi-Byte Index
    pDest++;
    *pDest = subindex; // BUF[3], Subindex
    pDest++;
    *pDest = p_client->buflen;
    pDest++;
    *pDest = (p_client->buflen >> 8);
    pDest++;
    *pDest = (p_client->buflen >> 16);
    pDest++;
    *pDest = (p_client->buflen >> 24);

    p_client->curlen = 0;
    p_client->toggle = 0;
    p_client->status = SDOCL_BLOCK_INITWR;

    if (MCOHW_PushMessage(p_client->channel, &(p_client->sdomsg)))
    { // Transmission initiated
      // set timeout for SDO request
      p_client->timeout = Tim->GetTime() + p_client->timeout_reload;
      p_client->status = p_client->status + SDOCL_WAIT_RES;
      return TRUE;
    }
  }
#endif
  else
  { // more than 4 bytes, segmented transfer
    *pDest = 0x21; // BUF[0], Command Specifier for Write

    pDest = &(p_client->sdomsg.BUF[4]);
    *pDest = (UNSIGNED8) p_client->buflen;
    pDest++;
    *pDest = (UNSIGNED8) (p_client->buflen >> 8);
    pDest++;
    *pDest = (UNSIGNED8) (p_client->buflen >> 16);
    pDest++;
    *pDest = (UNSIGNED8) (p_client->buflen >> 24);

    p_client->curlen = 0;
    p_client->toggle = 0;
    p_client->status = SDOCL_READY + SDOCL_SEGMENTED;
  }

  if (MCOHW_PushMessage(p_client->channel, &(p_client->sdomsg)))
  { // Transmission initiated
    // set timeout for SDO request
    p_client->timeout = Tim->GetTime() + p_client->timeout_reload;
    p_client->status = p_client->status + SDOCL_WAIT_RES; 
    return TRUE;
  }

  p_client->status = SDOCL_READY;
  return FALSE;
}


/**************************************************************************
Description in sdoclnt.h
***************************************************************************/ 
UNSIGNED8 SDOCLNT::SDOCLNT_Read (
  SDOCLIENT *p_client, // Pointer to initialized SDO client structure
  UNSIGNED16 index, // Object Dictionary Index to read
  UNSIGNED8 subindex // Object Dictionary Subindex to read
  )
{
UNSIGNED8 *pDest;

  if ( (p_client == 0) ||
       ((p_client->status & SDOCL_READY) != SDOCL_READY)
     )
  {
    return FALSE;
  }

  // Erase last abort
  p_client->last_abort = 0xFFFFFFFF;
  // Save index and subindex
  p_client->index = index;
  p_client->subindex = subindex;
  // Prepare SDO request message
  p_client->sdomsg.ID = p_client->canid_request;
  p_client->sdomsg.LEN = 8;
  pDest = ( UNSIGNED8 *) &(p_client->sdomsg.BUF[0]);
#if USE_BLOCKED_SDO_CLIENT
  if (p_client->bufmax > 28)
  {
    pDest[0] = (5 << 5); // BUF[0], Command Specifier for Read Block
    pDest[4] = SDO_BLK_MAX_SIZE; // BUF[4], max blocks
    pDest[5] = 1; // BUF[5], allowfall back
  }
  else
#endif
  {
    pDest[0] = 0x40; // BUF[0], Command Specifier for Read
    pDest[4] = 0;
    pDest[5] = 0;
  }
  pDest[1] = (UNSIGNED8) index; // BUF[1], Lo-Byte Index
  pDest[2] = (UNSIGNED8) (index >> 8); // BUF[2], Hi-Byte Index
  pDest[3] = subindex; // BUF[3], Subindex
  pDest[6] = 0;
  pDest[7] = 0;

  if (MCOHW_PushMessage(p_client->channel, &(p_client->sdomsg)))
  { // Transmission initiated
    // set timeout for SDO request
    p_client->timeout = Tim->GetTime() + p_client->timeout_reload;
    p_client->toggle = 0;
#if USE_BLOCKED_SDO_CLIENT
    p_client->status = SDOCL_BLOCK_INITRD + SDOCL_WAIT_RES;
#else
    p_client->status = SDOCL_READY + SDOCL_WAIT_RES;
#endif
    p_client->curlen = 0;
    return TRUE;
  }
  p_client->status = SDOCL_READY;
  return FALSE;
}


/**************************************************************************
Description in sdoclnt.h
***************************************************************************/ 
UNSIGNED8 SDOCLNT::SDOCLNT_WriteXtd (
  SDOCLIENT *p_client, // Pointer to initialized SDO client structure
  UNSIGNED16 index, // Object Dictionary Index to read
  UNSIGNED8 subindex, // Object Dictionary Subindex to read
  UNSIGNED8 *pSrc, // Pointer to data source
  UNSIGNED32 len, // Maximum length of data destination
  UNSIGNED16 timeout // Timeout for this transfer in milliseconds
  )
{
UNSIGNED8 ret_val = 0;

  if ( (p_client != 0) && ((p_client->status & SDOCL_READY) == SDOCL_READY) )
  {
    // change buffer data
    p_client->bufmax = len;
    p_client->buflen = len;
    p_client->curlen = 0;
    p_client->pBuf = pSrc;
    if (timeout == 0)
    { // use default
      p_client->timeout_reload = SDO_REQUEST_TIMEOUT;
    }
    else
    { // adjust timeout for this transfer
      p_client->timeout_reload = timeout;
    }

    ret_val = SDOCLNT_Write(p_client,index,subindex);
 
  }
  
   return ret_val;
}


/**************************************************************************
Description in sdoclnt.h
***************************************************************************/ 
UNSIGNED8 SDOCLNT::SDOCLNT_ReadXtd (
  SDOCLIENT *p_client, // Pointer to initialized SDO client structure
  UNSIGNED16 index, // Object Dictionary Index to read
  UNSIGNED8 subindex, // Object Dictionary Subindex to read
  UNSIGNED8 *pDest, // Pointer to data destination
  UNSIGNED32 len, // Maximum length of data destination
  UNSIGNED16 timeout // Timeout for this transfer in milliseconds
  )
{
UNSIGNED8 ret_val = 0;

  if ( (p_client != 0) && ((p_client->status & SDOCL_READY) == SDOCL_READY) )
  {
    // change buffer data
    p_client->bufmax = len;
    p_client->buflen = len;
    p_client->curlen = 0;
    p_client->pBuf = pDest;
    if (timeout == 0)
    { // use default
      p_client->timeout_reload = SDO_REQUEST_TIMEOUT;
    }
    else
    { // adjust timeout for this transfer
      p_client->timeout_reload = timeout;
    }

    ret_val = SDOCLNT_Read(p_client,index,subindex);
 
  }
  
   return ret_val;
}


/**************************************************************************
Description in sdoclnt.h
***************************************************************************/ 
void SDOCLNT::SDOCLNT_AbortTransfer (
  SDOCLIENT *p_client, // Pointer to initialized SDO client structure
  UNSIGNED32 abort_code // Abort code to transmit
  )
{
  if ((p_client != 0) && (p_client->status != SDOCL_READY) && (p_client->status != SDOCL_FREE))
  { // client pointer not null and client currently in use
    p_client->last_abort = abort_code;
    p_client->curlen = 0;
    p_client->status = SDOCL_READY;
    SDOCLNT_SendSDOAbort(p_client,abort_code);
    SDOCLNT_SDOComplete(p_client->channel,abort_code);
  }
}


/**************************************************************************
DOES:    Checks if an SDO client is busy or not
RETURNS: TRUE if busy
**************************************************************************/ 
UNSIGNED8 SDOCLNT::SDOCLNT_IsBusy (
  SDOCLIENT *p_client // Pointer to initialized SDO client structure
  )
{
  if ((p_client != 0) && ((p_client->status & 0x1F) != SDOCL_READY)) return TRUE;

  return FALSE;
}


/**************************************************************************
Description in sdoclnt.h
***************************************************************************/ 
UNSIGNED8 SDOCLNT::SDOCLNT_GetStatus (
  SDOCLIENT *p_client // Pointer to initialized SDO client structure
  )
{
UNSIGNED8 ret_val = SDOERR_FATAL;

  if (p_client != 0)
  {
    if (p_client->last_abort < 0xFFFFFFFFl)
    { // client not in use, check if there was an abort
      if (p_client->last_abort == 0)
      {
        ret_val = SDOERR_OK;
      }
      else if (p_client->last_abort == SDO_ABORT_TIMEOUT)
      {
        ret_val = SDOERR_TIMEOUT;
      }
      else if (p_client->last_abort == SDO_ABORT_NOTRANSFER)
      {
        ret_val = SDOERR_UNKNOWN;
      }
      else if (p_client->last_abort == SDO_ABORT_TOGGLE)
      {
        ret_val = SDOERR_TOGGLE;
      }
      else if (p_client->last_abort == SDO_ABORT_NOMEM)
      {
        ret_val = SDOERR_BUFSIZE;
      }
      else
      {
        ret_val = SDOERR_ABORT;
      }
    }
    else
    { // client in use
      ret_val = SDOERR_RUNNING;
    }
  }
  return ret_val;
}


/**************************************************************************
Description in sdoclnt.h
***************************************************************************/ 
UNSIGNED32 SDOCLNT::SDOCLNT_GetLastAbort (
  SDOCLIENT *p_client // Pointer to initialized SDO client structure
  )
{
UNSIGNED32 ret_val = 0xFFFFFFFFl;

  if (p_client != 0)
  {
    ret_val = p_client->last_abort;
  }
  return ret_val;
}


/**************************************************************************
DOES:    Gets executed if a CAN message was received that is the response
         to a SDO request
RETURNS: Nothing
***************************************************************************/ 
void SDOCLNT::MGR_HandleSDOClientResponse (
   UNSIGNED8 node_id, // node id that sent the response
   UNSIGNED8 *pDat // Pointer to 8 bytes of SDO data received
  )
{
UNSIGNED8 *pDest; // Destination pointer
UNSIGNED16 loop; // Loop counter
UNSIGNED32 len;
SDOCLIENT *p_client = &(mSDOClientList[node_id-1]);

  // Response received, so any existing timeout can be canceled / extended
  p_client->timeout = Tim->GetTime() + p_client->timeout_reload;
  p_client->b2btimeout = Tim->GetTime() + SDO_BACK2BACK_TIMEOUT;

#if USE_BLOCKED_SDO_CLIENT
  // are we receiving an SDO read block?
  if (p_client->status == SDOCL_BLOCK_READ + SDOCL_WAIT_RES)
  { // we are now receiving a read block
    p_client->toggle++; // count segments received
    if (p_client->toggle == (*pDat & 0x7F))
    { // only continue if sequence counter matches
      // is this last of block
      if ((p_client->toggle == SDO_BLK_MAX_SIZE) || (*pDat & 0x80))
      {
        p_client->status = SDOCL_BLOCK_RDCONF;
      }
      // retrieve data
      loop = 1;
      while ((p_client->curlen < p_client->buflen) && (loop <= 7))
      { // copy data received
        *(p_client->pBuf) = pDat[loop];
        p_client->curlen++;
        p_client->pBuf++;
        loop++;
      }
      // calculate number of bytes not used in segment
      p_client->n = 8 - loop;
    }
    else
    { // sequence error
      p_client->toggle--; // last segment not processed
    }
  }

  // are we receiving the SDO read block initialization confirmation?
  else if ((*pDat & 0xF9) == (6 << 5))
  { // SDO Block Upload (Read) init response confimration
    // Blocked transfer in process
    p_client->status = SDOCL_BLOCK_READ;
    // size buflen
    len = pDat[7];
    len <<= 8;
    len += pDat[6];
    len <<= 8;
    len += pDat[5];
    len <<= 8;
    len += pDat[4];
    // check with max buffer size available
    if (p_client->bufmax >= len)
    { // if available buffer is big enough, correct data size
      p_client->buflen = len;
    }
    else
    { // buffer is not big enough
      p_client->status = SDOCL_READY;
      // Generate Abort
      SDOCLNT_SendSDOAbort(p_client,SDO_ABORT_NOMEM);
      // Inform application of abort
      SDOCLNT_SDOComplete(p_client->channel,SDO_ABORT_NOMEM);
    }
  }

  // do we receive the confirmation for our block confirmation sent?
  else if ((*pDat & 0xE3) == ((6 << 5) + 0x01))
  { // SDO Block Upload (Read) block confimration
    // Blocked transfer in process
    p_client->status = SDOCL_BLOCK_RDFINA;
  }

  // is this the response to an SDO block write init request?
  else if (*pDat == 0xA0)
  { // SDO Block Download (Write) response confimration
    // Now switch to blocked transfer in process
    p_client->status = SDOCL_BLOCK_WRITE;
    // Maximum blksize
    p_client->blksize = pDat[4];
  }

  // is this the response to a block write completed confirmation?
  else if (*pDat == ((5 << 5) + 0x02))
  { // SDO Block Download (Write) block completed confirmation
    if (p_client->buflen == p_client->curlen)
    { // Blocked transfer completed, all transmitted
      p_client->status = SDOCL_BLOCK_WRCONF;
    }
    else
    {
      // Continue writing
      p_client->status = SDOCL_BLOCK_WRITE;
      // Maximum blksize for next block
      p_client->blksize = pDat[5];
      // restart counter
      p_client->toggle = 0;
    }
    // check number of confirmed blocks
    //if (pDat[4] < p_client->toggle)
  }

  // is this the final response to a block write?
  else if (*pDat == ((5 << 5) + 0x01))
  { // SDO Block Download (Write) final confirmation
    // free channel for new transfer
    p_client->status = SDOCL_BLOCK_WRFINA;
  }
  else
#endif // USE_BLOCKED_SDO_CLIENT
  if (*pDat == 0x80)
  { // SDO Abort received
    p_client->status = SDOCL_READY;
    SDOCLNT_SDOComplete(p_client->channel,SDOERR_ABORT);
  }
  // check if index and subindex information is in response
  else if ( ( (*pDat == 0x60) || ((*pDat & 0xF0) == 0x40) ) && 
       ( (pDat[1] != (UNSIGNED8)(p_client->index & 0x00FF)) || 
         (pDat[2] != (UNSIGNED8)((p_client->index & 0xFF00) >> 8)) || 
         (pDat[3] != p_client->subindex)
       )
     )
  { // response has index and subindex and they do not match
    p_client->status = SDOCL_READY;
    // Generate Abort
    SDOCLNT_SendSDOAbort(p_client,SDO_ABORT_PARAMETER);
    // Call-back application
    SDOCLNT_SDOComplete(p_client->channel,SDOERR_PARAM);
  }
  else if ((*pDat == 0x60) || (*pDat == 0x20) || (*pDat == 0x30))
  { // SDO Download (Write) response confimration
    // or next download segment confirmation
    p_client->status &= ~SDOCL_WAIT_RES;
    if ((p_client->status & SDOCL_SEGMENTED) == 0)
    { // only call the call-back if this is an expedited transfer
      SDOCLNT_SDOComplete(p_client->channel,SDOERR_OK);
    }
    // Verify toggle bit
    else if ((*pDat & 0x10) != p_client->toggle)
    {
      p_client->status = SDOCL_READY;
      // Generate Abort
      SDOCLNT_SendSDOAbort(p_client,SDO_ABORT_TOGGLE);
      // Inform application of abort
      SDOCLNT_SDOComplete(p_client->channel,SDOERR_TOGGLE);
    }
    else
    {
      // Check if this was the last segment
      if ((p_client->status & SDOCL_LAST_SEG) == SDOCL_LAST_SEG)
      {
        p_client->status = SDOCL_READY + SDOCL_SEGMENTED;
        // Call back application that transfer is completed
        SDOCLNT_SDOComplete(p_client->channel,SDOERR_OK);
      }
      else
      {
        p_client->status |= SDOCL_NEXT_DWN;

        // Set pointer to first data byte of next segment
        pDest = &(p_client->sdomsg.BUF[0]);

        // update toggle value for next usage
        if (p_client->curlen > 0)
        { // only start toggling after first segment was transferred
          if (p_client->toggle == 0)
          {
            p_client->toggle = 0x10;
          }
          else
          {
            p_client->toggle = 0x00;
          }
        }
        // Set command specifier for next request
        *pDest = p_client->toggle;

        // Copy next data segment
        if ((p_client->buflen > 7) && ((p_client->buflen-7) > p_client->curlen))
        { // more than 7 bytes remain for transfer
          memcpy(&(pDest[1]),&(p_client->pBuf[p_client->curlen]),7);
          p_client->curlen += 7;
        }
        else
        { // This is the last segment
          memcpy(&(pDest[1]),&(p_client->pBuf[p_client->curlen]),p_client->buflen-p_client->curlen);
          // update commad specifier: set bit for last segment
          *pDest |= 0x01;
          // and number of bytes that do not contain data
          len = p_client->buflen - p_client->curlen;
          *pDest += (7 - (UNSIGNED8) (len)) << 1;
          // update curlen counter
          p_client->curlen = p_client->buflen;
          // indicate last segment to SDO_HandleClient
          p_client->status |= SDOCL_LAST_SEG;
        }
      }
    }
  }
  else if ((*pDat & 0xF3) == 0x43)
  { // SDO Upload (Read) Response Confimration
    // Copy received data to destination
    len = 4 - ((*pDat >> 2) & 0x03);
    p_client->sdomsg.LEN = (UNSIGNED8) len;
    pDat += 4;
    pDest = p_client->pBuf;
    for (loop = 1; loop <= len; loop++)
    {
      *pDest = *pDat; 
      pDest++;
      pDat++;
    }
    p_client->curlen = len;
    p_client->status &= ~SDOCL_WAIT_RES;
    SDOCLNT_SDOComplete(p_client->channel,SDOERR_OK);
  } 
  else if ((*pDat == 0x41) || (*pDat == 0x40))
  { // Init segmented upload transfer,
    // data size indicated or not indicated
    p_client->status &= ~SDOCL_WAIT_RES;
    p_client->status |= SDOCL_NEXT_UPL;
    //gSDOClientList[channel].sdomsg.LEN = 8;
    p_client->sdomsg.BUF[0] = 0x60; 
    pDest = &(p_client->sdomsg.BUF[1]);
    for (loop = 1; loop <= 7; loop++)
    {
      *pDest = 0;
      pDest++;
    }
  }
  else if ((*pDat & 0xE0) == 0)
  { // SDO segment, upload (read)
    if ((*pDat & 0x10) != p_client->toggle)
    { // toggle error
      p_client->status = SDOCL_READY;
      // Generate Abort
      SDOCLNT_SendSDOAbort(p_client,SDO_ABORT_TOGGLE);
      // Call-back application
      SDOCLNT_SDOComplete(p_client->channel,SDOERR_TOGGLE);
    }
    else
    {
      len = 7 - ((*pDat & 0x0E) >> 1);
      if ((p_client->curlen + len) > p_client->buflen)
      { // destination buffer not big enough
        // Reset status info
        p_client->status = SDOCL_READY;
        // Generate Abort
        SDOCLNT_SendSDOAbort(p_client,SDO_ABORT_NOMEM);
        // Call-back application
        SDOCLNT_SDOComplete(p_client->channel,SDOERR_BUFSIZE);
      }
      else
      {
        memcpy(&(p_client->pBuf[p_client->curlen]),&(pDat[1]),len);
        p_client->curlen += len;
        if ((*pDat & 0x01) == 1)
        { // This was the last segment, transfer completed
          // Reset status info
          p_client->status = SDOCL_READY + SDOCL_SEGMENTED;
          // Call-back application
          SDOCLNT_SDOComplete(p_client->channel,SDOERR_OK);
        }
        else
        { // more segments to come
          p_client->status = SDOCL_NEXT_UPL;
          if (p_client->toggle == 0)
          {
            p_client->toggle = 0x10;
            p_client->sdomsg.BUF[0] |= 0x10;
          }
          else
          {
            p_client->toggle = 0x00;
            p_client->sdomsg.BUF[0] &= 0xEF;
          }
        }
      }
    }
  }
  else
  { // Unknown SDO response received
    p_client->status = SDOCL_READY;
    // Generate Abort
    SDOCLNT_SendSDOAbort(p_client,SDO_ABORT_NOTRANSFER);
    // Call-back application
    SDOCLNT_SDOComplete(p_client->channel,SDOERR_UNKNOWN);
  }
}


/**************************************************************************
DOES:    Working on clients
RETURNS: TRUE for success, FALSE for error
***************************************************************************/ 
UNSIGNED8 SDOCLNT::MGR_SDOHandleClient (
  void
  )
{
#if USE_BLOCKED_SDO_CLIENT
UNSIGNED8 loop;
#endif

  mCurrentChannel++; // working on next channel
  if (mCurrentChannel >= NR_OF_SDO_CLIENTS)
  { // ensure values are in allowed range
    mCurrentChannel = 0;
  }
  if ((mSDOClientList[mCurrentChannel].status & SDOCL_WAIT_RES) == SDOCL_WAIT_RES)
  { // currently waiting for a response
    if (Tim->IsTimeExpired(mSDOClientList[mCurrentChannel].timeout))
    { // abort transfer
      mSDOClientList[mCurrentChannel].status = SDOCL_READY;
      SDOCLNT_SendSDOAbort(&(mSDOClientList[mCurrentChannel]),SDO_ABORT_TIMEOUT);
      SDOCLNT_SDOComplete(mCurrentChannel+1,SDOERR_TIMEOUT);
      return TRUE;
    }
  }
  else if (((mSDOClientList[mCurrentChannel].status & SDOCL_NEXT_UPL) == SDOCL_NEXT_UPL) ||
           ((mSDOClientList[mCurrentChannel].status & SDOCL_NEXT_DWN) == SDOCL_NEXT_DWN)
          )
  { // ready to transmit next request
    if (Tim->IsTimeExpired(mSDOClientList[mCurrentChannel].b2btimeout))
    { // wait for timeout to avoid back-to-back traffic
      if (!MCOHW_PushMessage(mCurrentChannel + 1, &(mSDOClientList[mCurrentChannel].sdomsg)))
      { // Error: transmit queue overrun
        return FALSE;
      }
      else
      { // message transmitted, set new timeout for response
        mSDOClientList[mCurrentChannel].timeout = Tim->GetTime() + mSDOClientList[mCurrentChannel].timeout_reload;
        // now wait for response
        mSDOClientList[mCurrentChannel].status |= SDOCL_WAIT_RES;
        return TRUE;
      }
    }
  }

#if USE_BLOCKED_SDO_CLIENT
  // are we in the middle of a write block?
  else if (mSDOClientList[mCurrentChannel].status == SDOCL_BLOCK_WRITE)
  { // ready to transmit next block part
    if (!Tim->IsTimeExpired(mSDOClientList[mCurrentChannel].b2btimeout))
    { // wait for timeout to avoid back-to-back traffic
      return FALSE;
    }
    if (mSDOClientList[mCurrentChannel].curlen < mSDOClientList[mCurrentChannel].buflen)
    { // Buffer not yet empty, more to transmit
      mSDOClientList[mCurrentChannel].toggle++;  
      mSDOClientList[mCurrentChannel].sdomsg.BUF[0] = mSDOClientList[mCurrentChannel].toggle;
      loop = 1;
      while ((mSDOClientList[mCurrentChannel].curlen < mSDOClientList[mCurrentChannel].buflen) && (loop <= 7))
      { // copy data
        mSDOClientList[mCurrentChannel].sdomsg.BUF[loop] = *(mSDOClientList[mCurrentChannel].pBuf);
        loop++;
        mSDOClientList[mCurrentChannel].pBuf++;
        mSDOClientList[mCurrentChannel].curlen++;
      }
      mSDOClientList[mCurrentChannel].blksize--;  
      mSDOClientList[mCurrentChannel].n = 8 - loop; // number of unused bytes
      if ((mSDOClientList[mCurrentChannel].curlen == mSDOClientList[mCurrentChannel].buflen) || (mSDOClientList[mCurrentChannel].blksize == 0))
      { // end of block reached
        if (mSDOClientList[mCurrentChannel].curlen == mSDOClientList[mCurrentChannel].buflen)
        { // This is the very last message
          mSDOClientList[mCurrentChannel].sdomsg.BUF[0] |= 0x80; // last segment bit indication
        }
        mSDOClientList[mCurrentChannel].status = SDOCL_BLOCK_WRCONF + SDOCL_WAIT_RES;
      }
      if (!MCOHW_PushMessage(mCurrentChannel + 1, &(mSDOClientList[mCurrentChannel].sdomsg)))
      { // Error: transmit queue overrun
        return FALSE;
      }
      // message transmitted, set new timeouts
      mSDOClientList[mCurrentChannel].b2btimeout = Tim->GetTime() + SDO_BACK2BACK_TIMEOUT;
      mSDOClientList[mCurrentChannel].timeout = Tim->GetTime() + mSDOClientList[mCurrentChannel].timeout_reload;
      return TRUE;
    }
  }

  // was a write block completed and needs to be confirmed?
  else if (mSDOClientList[mCurrentChannel].status == SDOCL_BLOCK_WRCONF)
  { // ready to send final block confirmation
    // set new state
    mSDOClientList[mCurrentChannel].status = SDOCL_BLOCK_WRFINA + SDOCL_WAIT_RES;
    // prepare confirmation message
    mSDOClientList[mCurrentChannel].sdomsg.BUF[0] = (6 << 5) + (mSDOClientList[mCurrentChannel].n << 2) + 0x01;
    mSDOClientList[mCurrentChannel].sdomsg.BUF[1] = 0;
    mSDOClientList[mCurrentChannel].sdomsg.BUF[2] = 0;
    mSDOClientList[mCurrentChannel].sdomsg.BUF[3] = 0;
    mSDOClientList[mCurrentChannel].sdomsg.BUF[4] = 0;
    mSDOClientList[mCurrentChannel].sdomsg.BUF[5] = 0;
    mSDOClientList[mCurrentChannel].sdomsg.BUF[6] = 0;
    mSDOClientList[mCurrentChannel].sdomsg.BUF[7] = 0;
    if (!MCOHW_PushMessage(mCurrentChannel + 1, &(mSDOClientList[mCurrentChannel].sdomsg)))
    { // Error: transmit queue overrun
      return FALSE;
    }
    // message transmitted, set new timeout for response
    mSDOClientList[mCurrentChannel].timeout = Tim->GetTime() + mSDOClientList[mCurrentChannel].timeout_reload;
    return TRUE;
  }

  // was a write block final confirmation sent?
  else if (mSDOClientList[mCurrentChannel].status == SDOCL_BLOCK_WRFINA)
  { // all completed, call back application 
    mSDOClientList[mCurrentChannel].status = SDOCL_READY; // available for next transfer
    SDOCLNT_SDOComplete(mCurrentChannel+1,SDOERR_OK);
    return TRUE;
  }

  // are we at end of a block read init transfer?
  else if (mSDOClientList[mCurrentChannel].status == SDOCL_BLOCK_READ)
  { // transmit one more request
    mSDOClientList[mCurrentChannel].sdomsg.BUF[0] = (5 << 5) + 0x03; // Command 5, subcommand 3
    mSDOClientList[mCurrentChannel].sdomsg.BUF[1] = 0;
    mSDOClientList[mCurrentChannel].sdomsg.BUF[2] = 0;
    mSDOClientList[mCurrentChannel].sdomsg.BUF[3] = 0;
    mSDOClientList[mCurrentChannel].sdomsg.BUF[4] = 0;
    mSDOClientList[mCurrentChannel].sdomsg.BUF[5] = 0;
    mSDOClientList[mCurrentChannel].sdomsg.BUF[6] = 0;
    mSDOClientList[mCurrentChannel].sdomsg.BUF[7] = 0;
    // Set flag: we wait for a response
    mSDOClientList[mCurrentChannel].status |= SDOCL_WAIT_RES;
    mSDOClientList[mCurrentChannel].timeout = Tim->GetTime() + mSDOClientList[mCurrentChannel].timeout_reload;
    if (!MCOHW_PushMessage(mCurrentChannel + 1, &(mSDOClientList[mCurrentChannel].sdomsg)))
    { // Error: transmit queue overrun
      return FALSE;
    }
    return TRUE;
  }
  
  // in middle of receiving a read block
  else if (mSDOClientList[mCurrentChannel].status == SDOCL_BLOCK_RDCONF)
  {
    mSDOClientList[mCurrentChannel].sdomsg.BUF[0] = (5 << 5) + 0x02; // Command 5, subcommand 2
    mSDOClientList[mCurrentChannel].sdomsg.BUF[1] = mSDOClientList[mCurrentChannel].toggle; // confirm the number of segments received
    mSDOClientList[mCurrentChannel].sdomsg.BUF[2] = SDO_BLK_MAX_SIZE; // blksize
    mSDOClientList[mCurrentChannel].sdomsg.BUF[3] = 0;
    mSDOClientList[mCurrentChannel].sdomsg.BUF[4] = 0;
    mSDOClientList[mCurrentChannel].sdomsg.BUF[5] = 0;
    mSDOClientList[mCurrentChannel].sdomsg.BUF[6] = 0;
    mSDOClientList[mCurrentChannel].sdomsg.BUF[7] = 0;
    if (mSDOClientList[mCurrentChannel].buflen != mSDOClientList[mCurrentChannel].curlen)
    { // More blocks to come
      mSDOClientList[mCurrentChannel].status = SDOCL_BLOCK_READ + SDOCL_WAIT_RES;
      mSDOClientList[mCurrentChannel].toggle = 0; // start new block counter
    }
    else
    { // last block
      // Set flag: we wait for a response
      mSDOClientList[mCurrentChannel].status |= SDOCL_WAIT_RES;
    }
    mSDOClientList[mCurrentChannel].timeout = Tim->GetTime() + mSDOClientList[mCurrentChannel].timeout_reload;
    if (!MCOHW_PushMessage(mCurrentChannel + 1, &(mSDOClientList[mCurrentChannel].sdomsg)))
    { // Error: transmit queue overrun
      return FALSE;
    }
    return TRUE;
  }

  // final confirmation of a read block
  else if (mSDOClientList[mCurrentChannel].status == SDOCL_BLOCK_RDFINA)
  { // final confirmation
    mSDOClientList[mCurrentChannel].sdomsg.BUF[0] = (5 << 5) + 0x01; // Command 5, subcommand 1
    mSDOClientList[mCurrentChannel].sdomsg.BUF[1] = 0;
    mSDOClientList[mCurrentChannel].sdomsg.BUF[2] = 0;
    mSDOClientList[mCurrentChannel].status = SDOCL_READY; // available for next transfer
    if (!MCOHW_PushMessage(mCurrentChannel + 1, &(mSDOClientList[mCurrentChannel].sdomsg)))
    { // Error: transmit queue overrun
      return FALSE;
    }
    SDOCLNT_SDOComplete(mCurrentChannel+1,SDOERR_OK);
    return TRUE;
  }
#endif // USE_BLOCKED_SDO_CLIENT

  return FALSE;
}


/*******************************************************************************
END OF FILE
*******************************************************************************/
