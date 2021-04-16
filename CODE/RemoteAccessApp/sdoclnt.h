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
           $LastChangedDate: 2014-05-01 10:22:11 +0100 (Thu, 01 May 2014) $
           $LastChangedRevision: 3108 $
***************************************************************************/ 

#ifndef _SDOCLNT_H
#define _SDOCLNT_H

#include "global.h"
#include "Timer.h"


// Number of SDO channels implemented
#define NR_OF_SDO_CLIENTS 32
// define to 1 to enable block transfers
#define USE_BLOCKED_SDO_CLIENT 1


/**************************************************************************
GLOBAL DEFINITIONS
**************************************************************************/

// SDO Client Status Flags (in SDOCLIENT)
#define SDOCL_FREE      0x00 // Channel unused
#define SDOCL_READY     0x01 // Channel initialized
#define SDOCL_WAIT_RES  0x02 // Waiting for expedited SDO response 
#define SDOCL_NEXT_UPL  0x04 // Next SDO Upload Request queued
#define SDOCL_NEXT_DWN  0x08 // Next SDO Download Request due
#define SDOCL_LAST_SEG  0x10 // Last segment of a segmented transfer
#define SDOCL_SEGMENTED 0x80 // Segmented transfer allowed

// For block transfer
#define SDOCL_BLOCK_INITWR 0x20 // Block Write Init
#define SDOCL_BLOCK_WRITE  0x30 // Block Write in progress
#define SDOCL_BLOCK_WRCONF 0x40 // Block Write over
#define SDOCL_BLOCK_WRFINA 0x41 // Block Write over, final & call-back
#define SDOCL_BLOCK_INITRD 0x50 // Block Read Init
#define SDOCL_BLOCK_READ   0x60 // Block Read in progress
#define SDOCL_BLOCK_RDCONF 0x70 // Block Read over
#define SDOCL_BLOCK_RDFINA 0x71 // Block Read over, final & call-back

// SDO Response Status (in SDOCLNTCB_SDOComplete, SDOCLNT_IsTransferComplete)
#define SDOERR_FATAL    0x00 // Illegal pointer passed
#define SDOERR_OK       0x01 // Confirmation, last access was sucess
#define SDOERR_ABORT    0x80 // Abort received
#define SDOERR_TIMEOUT  0x81 // Request timed out
#define SDOERR_TOGGLE   0x82 // Toggle error
#define SDOERR_BUFSIZE  0x84 // Out of Memory
#define SDOERR_PARAM    0x85 // Wrong parameter
#define SDOERR_UNKNOWN  0x88 // No transfer possible
#define SDOERR_RUNNING  0xFF // SDO Transfer still running, not complete

// Scanning of nodes: states
#define SCAN_NONE       0x00 // node is not present 
#define SCAN_DELAY      0x02 // started a delay
#define SCAN_RUN        0x80 // scanning in progress
#define SCAN_WAITREPLY  0x81 // waiting for reply
#define SCAN_ABORT      0xC0 // scan aborted
#define SCAN_DONE       0xFE // scan complete
#define SCAN_OVER       0xFF // scan is over


/**************************************************************************
GLOBAL TYPES AND STRUCTURES
**************************************************************************/

typedef struct
{
  CAN_MSG sdomsg;                 // Message buffer for send and receive
  UNSIGNED32 canid_request;       // CAN ID used for SDO requests
  UNSIGNED32 canid_response;      // CAN ID used for SDO responses
  UNSIGNED32 bufmax;              // Maximum length of buffer
  UNSIGNED32 buflen;              // Length of expected transfer
  UNSIGNED32 curlen;              // Current length of buffer
  UNSIGNED32 last_abort;          // last abort code if any
  UNSIGNED8 *pBuf;                // Pointer to buffer for transfer
  UNSIGNED16 timeout;             // SDO Timeout current tmestamp
  UNSIGNED16 timeout_reload;      // SDO Timeout re-load value
  UNSIGNED16 b2btimeout;          // Back-to-Back Timestamp
  UNSIGNED16 index;               // Index of current request
  UNSIGNED8 subindex;             // Subindex of current request
#if USE_BLOCKED_SDO_CLIENT
  UNSIGNED8 blksize;              // Size of block (in messages)
  UNSIGNED8 n;                    // in last segment, number of unused bytes
#endif
  UNSIGNED8 toggle;               // Last toggle value used, or block counter
  UNSIGNED8 status;               // Channel status info
  UNSIGNED8 channel;              // Channel number from 1 to NR_OF_SDO_CLIENTS
} SDOCLIENT;


// callbacks
typedef void (*SDOCLNTSENDCALLBACK)(UNSIGNED8 nodeid, UNSIGNED8 *pData, void *param);


class SDOCLNT
{
  public:
    /**************************************************************************
    DOES:    Constructor - resets all SDO Client channels
    RETURNS: nothing
    **************************************************************************/
    SDOCLNT(
      Timer *Tim,                                          // timer instance
      SDOCLNTSENDCALLBACK *SendCallback,                   // callback function for sending
      void *SenderInstance                                 // instance of sender
      );
    /**************************************************************************
    DOES:    (Re-)initializes an SDO client channel.
    RETURNS: NULL-Pointer, if channel initialization failed.
             Pointer to SDOCLIENT structure used, if init success
    **************************************************************************/ 
    SDOCLIENT *SDOCLNT_Init (
      UNSIGNED8 channel, // SDO channel number in range of 1 to NR_OF_SDO_CLIENTS
      UNSIGNED32 canid_request, // CAN message ID used for the SDO request
      UNSIGNED32 canid_response, // CAN message ID used for the SDO response
      UNSIGNED8 *p_buf, // data buffer pointer for data exchanged
      UNSIGNED32 buf_size // max length of data buffer
      );
    /**************************************************************************
    DOES:    Transmits an SDO Write (download) request.
    RETURNS: TRUE, if request was queued
             FALSE, if transmit queue full
    GLOBALS: Uses the last data buffer assigned from last call to SDOCLNT_Init,
             SDOCLNT_ReadXtd or SDOCLNT_WriteXtd
    NOTE:    Non blocking transfer, use functions SDOCLNT_GetStatus, 
             SDOCLNT_BlockUntilCompleted or SDOCLNTCB_SDOComplete to determine
             when transfer completed
    **************************************************************************/ 
    UNSIGNED8 SDOCLNT_Write (
      SDOCLIENT *p_client, // Pointer to initialized SDO client structure
      UNSIGNED16 index, // Object Dictionary Index to write
      UNSIGNED8 subindex // Object Dictionary Subindex to write
      );
    /**************************************************************************
    DOES:    Transmits an SDO Write (download) request with extended parameters
    RETURNS: TRUE, if request was queued
             FALSE, if transmit queue full
    NOTE:    Non blocking transfer, use functions SDOCLNT_GetStatus, 
             SDOCLNT_BlockUntilCompleted or SDOCLNTCB_SDOComplete to determine
             when transfer completed
    **************************************************************************/ 
    UNSIGNED8 SDOCLNT_WriteXtd (
      SDOCLIENT *p_client, // Pointer to initialized SDO client structure
      UNSIGNED16 index, // Object Dictionary Index to write
      UNSIGNED8 subindex, // Object Dictionary Subindex to write
      UNSIGNED8 *pSrc, // Pointer to data source
      UNSIGNED32 len, // Length of data
      UNSIGNED16 timeout // Timeout for this transfer in milliseconds
      );
    /**************************************************************************
    DOES:    Transmits an SDO Read (upload) request.
    RETURNS: TRUE, if request was queued
             FALSE, if transmit queue full
    GLOBALS: Uses the last data buffer assigned from last call to SDOCLNT_Init,
             SDOCLNT_ReadXtd or SDOCLNT_WriteXtd
    NOTE:    Non blocking transfer, use functions SDOCLNT_GetStatus, 
             SDOCLNT_BlockUntilCompleted or SDOCLNTCB_SDOComplete to determine
             when transfer completed
    **************************************************************************/ 
    UNSIGNED8 SDOCLNT_Read (
      SDOCLIENT *p_client, // Pointer to initialized SDO client structure
      UNSIGNED16 index, // Object Dictionary Index to read
      UNSIGNED8 subindex // Object Dictionary Subindex to read
      );
    /**************************************************************************
    DOES:    Transmits an SDO Read (upload) request with extended parameters
    RETURNS: TRUE, if request was queued
             FALSE, if transmit queue full
    NOTE:    Non blocking transfer, use functions SDOCLNT_GetStatus, 
             SDOCLNT_BlockUntilCompleted or SDOCLNTCB_SDOComplete to determine
             when transfer completed
    **************************************************************************/ 
    UNSIGNED8 SDOCLNT_ReadXtd (
      SDOCLIENT *p_client, // Pointer to initialized SDO client structure
      UNSIGNED16 index, // Object Dictionary Index to read
      UNSIGNED8 subindex, // Object Dictionary Subindex to read
      UNSIGNED8 *pDest, // Pointer to data destination
      UNSIGNED32 len, // Maximum length of data destination
      UNSIGNED16 timeout // Timeout for this transfer in milliseconds
      );
    /**************************************************************************
    DOES:    Checks the current status of the SDO client
    RETURNS: SDOERR_xxx codes
    **************************************************************************/ 
    UNSIGNED8 SDOCLNT_GetStatus (
      SDOCLIENT *p_client // Pointer to initialized SDO client structure
      );
    /**************************************************************************
    DOES:    Checks if an SDO client is busy or not
    RETURNS: TRUE if busy
    **************************************************************************/ 
    UNSIGNED8 SDOCLNT_IsBusy (
      SDOCLIENT *p_client // Pointer to initialized SDO client structure
      );
    /**************************************************************************
    DOES:    Returns the last abort code of the SDO client, if any
    RETURNS: Last SDO client Abort code or 0
    **************************************************************************/ 
    UNSIGNED32 SDOCLNT_GetLastAbort (
      SDOCLIENT *p_client // Pointer to initialized SDO client structure
      );
    /**************************************************************************
    DOES:    Call-Back to application when an SDO client transfer is completed
    RETURNS: nothing
    **************************************************************************/
    void SDOCLNTCB_SDOComplete (
      UNSIGNED8 channel, // SDO channel number in range of 1 to NR_OF_SDO_CLIENTS
      UNSIGNED32 abort_code // SDOERR_xxx codes or SDO abort code
      );
    /**************************************************************************
    DOES:    Aborts a transfer currently in progress.
    RETURNS: nothing
    **************************************************************************/
    void SDOCLNT_AbortTransfer (
      SDOCLIENT *p_client, // Pointer to initialized SDO client structure
      UNSIGNED32 abort_code // Abort code to transmit
      );
    /**************************************************************************
    DOES:    Initiates multiple SDO read requests to be sent, results read
             go directly into an array of UNSIGNED32. Limited to expedited
             transfers of up to 4 byte.
             Format of the ScanList is 4 bytes per entry
             0,1: Index (set to 0xFFFF to mark end of list)
             2: Subindex
             3: Length
             When the scan completed, MGRCB_NodeStatusChanged() is called
    RETURNS: nothing
    **************************************************************************/
    void MGRSCAN_Init (
      UNSIGNED8 sdo_clnt,   // SDO client number from 1 to NR_OF_SDO_CLIENTS
      UNSIGNED8 node_id,    // Node ID of node to read data from
      UNSIGNED8 *pScanList, // Pointer to list with OD entries to be read
      UNSIGNED32 *pScanData, // Pointer to array, must be as long as number
                            // of entries in list above
      UNSIGNED16 Delay      // Delay in ms between read requests
      );
    /**************************************************************************
    DOES:    Can be used to check if a previously started scan is still running
    RETURNS: TRUE if scan is still running, else FALSE
    **************************************************************************/
    UNSIGNED8 MGRSCAN_GetStatus (
      UNSIGNED8 node_id     // Node ID of node whic is auto-scanned
      );
    /**************************************************************************
    DOES:    Working on clients
    RETURNS: TRUE for success, FALSE for error
    ***************************************************************************/ 
    UNSIGNED8 MGR_SDOHandleClient (
      void
      );
    /**************************************************************************
    DOES:    Gets executed if a CAN message was received that is the response
             to a SDO request
    RETURNS: Nothing
    ***************************************************************************/ 
    void MGR_HandleSDOClientResponse (
       UNSIGNED8 node_id, // node id that sent the response
       UNSIGNED8 *pDat // Pointer to 8 bytes of SDO data received
      );

    // callback functions
    SDOREQUESTCOMPLETECALLBACK *SdoRequestCompleteCallback;

  private:
    /**************************************************************************
    Description in comgr.h
    ***************************************************************************/ 
    void SDOCLNT_SDOComplete (
      UNSIGNED8 channel, // SDO channel number in range of 1 to NR_OF_SDO_CLIENTS
      UNSIGNED32 abort_code // status, error, abort code
      );
    /**************************************************************************
    DOES:    Generates an SDO abort message for a specific SDO channel
    GLOBALS: Assumes that the sdomsg is already initalized properly from last
             request.
    **************************************************************************/
    UNSIGNED8 SDOCLNT_SendSDOAbort (
      SDOCLIENT *p_client, // Pointer to initialized SDO client structure
      UNSIGNED32 ErrorCode // 4 byte SDO abort error code
      );
    /**************************************************************************
    DOES:    Sends a message
    RETURNS: TRUE if successful, FALSE if error
    **************************************************************************/
    UNSIGNED8 MCOHW_PushMessage (
      UNSIGNED8 node_id,
      CAN_MSG *pTx
      );

    // data records for each client
    SDOCLIENT mSDOClientList[NR_OF_SDO_CLIENTS];
    // module loops through clients
    UNSIGNED8 mCurrentChannel;
    // timer
    Timer *Tim;
    // callback function to send an SDO
    SDOCLNTSENDCALLBACK *SendCallback;
    // instance of class that performs the SDO send
    void *SenderInstance;
};


#endif // _SDOCLNT_H


/**************************************************************************
END OF FILE
**************************************************************************/

