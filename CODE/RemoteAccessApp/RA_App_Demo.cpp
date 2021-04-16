/**************************************************************************
MODULE:    RA_App
CONTAINS:  Main module for CANopenIA Remote Access implementation
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

#ifdef WIN32
#include <windows.h>
#include <conio.h>
#endif
#include <stdio.h>
#include <time.h>
#include <signal.h>
#include "SerialProtocol.h"

#define MAX_NUMBER_OF_NODES 32

// define to 1 to enable display of new data on the network
#define SHOW_NEW_DATA 0

/**************************************************************************
MODULE VARIABLES
***************************************************************************/ 

// node state flags
#define NODE_STATE_NONE         0
#define NODE_STATE_HBACTIVE     (1 << 0)
#define NODE_STATE_HBLOSS       (1 << 1)
#define NODE_STATE_SCANNING     (1 << 2)
#define NODE_STATE_SCANFINISHED (1 << 3)
#define NODE_STATE_PRODUCEDATA  (1 << 4)
#define NODE_STATE_BOOTED       (1 << 7)

// NMT commands
#define NMT_OPERATIONAL    1
#define NMT_STOP           2
#define NMT_PREOPERATIONAL 128
#define NMT_RESETAPP       129
#define NMT_RESETCOM       130

// baudrate to connect at
#define BAUDRATE 921600

static SerialProtocol *COIADevice = new SerialProtocol();

static unsigned char MyNodeID = 0;       // Our own node ID
static unsigned char MyNMTState;         // Our own state
static int TerminationRequested = FALSE; // termination flag

// States of nodes
// bit 0: HB active 
// bit 1: HB loss
// bit 2: scanner active, do not use SDO client
// bit 3: inital scan completed
// bit 4: produce data
// bit 7: not available, booted
static unsigned char NodeStates[MAX_NUMBER_OF_NODES] = { NODE_STATE_NONE };

// Data to read or write
static unsigned long Length;
static unsigned char DataBuf[MAX_PACKET_LENGTH - 7];


/**************************************************************************
DOES:    This function is called from the new data recived call-back,
         if data received indicates a change in this devices node status:
         Change of own node ID or own NMT state or own HW status
GLOBALS: Updates MyNodeID and MyNMTState
**************************************************************************/
void OwnStatusChanged (
  unsigned char SubIdx, // 1 to 3 for node id, NMT status or HW status
  unsigned char data
  )
{
  if (SubIdx == 1)
  {
    printf("\n{Own node ID changed to %d} ", data);
    MyNodeID = data;
  }
  else if (SubIdx == 2)
  {
    printf("\n{Own status changed to %d} ", data);
    MyNMTState = data;

    if (data == NODESTATUS_OPERATIONAL)
    { // we are now set to operational
    }
    // if node was reset then re-initialize it
    if (data == NODESTATUS_RESETAPP)
    {
      // wait for node to complete reset
      Timer::Sleep(500);
    }
  }
  else if  (SubIdx == 3)
  {
    printf("\n{Hardware status changed to 0x%2.2X - ", data);
  
    if (data == HWSTATUS_NONE)
    {
      printf("NONE} ");
      return;
    }

    if (data & HWSTATUS_INITALIZING)    printf("INIT ");
    if (data & HWSTATUS_CANERROR)       printf("CAN-ERROR ");
    if (data & HWSTATUS_ERRORPASSIVE)   printf("ERROR-PASSIVE ");
    if (data & HWSTATUS_RXQUEUEOVERRUN) printf("RX-OVERRUN ");
    if (data & HWSTATUS_TXQUEUEOVERRUN) printf("TX-OVERRUN ");
    if (data & HWSTATUS_TXBUSY)         printf("TX-BUSY ");
    if (data & HWSTATUS_BUSOFF)         printf("BUS-OFF ");
    printf("} ");
  }
  else
  {
    printf("\n{Unknown Subindex: %d} ", SubIdx);
  }
}


/**************************************************************************
DOES:    This function is called from the new data recived call-back,
         if data received indicates a change in the node status of any of
         the nodes connected to the network
GLOBALS: Updates NodeScanned
**************************************************************************/
void NodeStatusChanged (
  unsigned char NodeID, // node ID for which a change of state was detected
                        // highest bit set if it is for own node ID
  unsigned char State // current state of that node
  )
{
  if (NodeID >= MAX_NUMBER_OF_NODES) return;

  printf("\n{Node %d ", NodeID & 0x7F);

  if (NodeID & 0x80)
  {
    printf("(self) ");
  }

  printf("status changed to 0x%2.2X - ",State);
  switch(State)
  {
    case NODESTATUS_BOOT: 
      printf("BOOT} "); 
      NodeStates[NodeID - 1] |= NODE_STATE_BOOTED; // signal node booted
      break;
    case NODESTATUS_STOPPED: printf("STOP} "); break;
    case NODESTATUS_OPERATIONAL: printf("OPERATIONAL} "); break;
    case NODESTATUS_PREOP: printf("PREOP} "); break;
    case NODESTATUS_EMCY_OVER: printf("EMCY CLEAR} "); break;  
    case NODESTATUS_EMCY_NEW: printf("NEW EMCY} "); break;   
    case NODESTATUS_HBACTIVE: 
      printf("HB ACTIVE} "); 
      NodeStates[NodeID - 1] |= NODE_STATE_HBACTIVE; // signal HB active
      NodeStates[NodeID - 1] &= ~NODE_STATE_HBLOSS;
      break;   
    case NODESTATUS_HBLOST: 
      printf("HB LOST} "); 
      NodeStates[NodeID - 1] = NODE_STATE_HBACTIVE | NODE_STATE_HBLOSS; // signal HB active & loss, reset all other bits
      break;     
    case NODESTATUS_SCANSTARTED: 
      printf("SCAN INIT} "); 
      NodeStates[NodeID - 1] |= NODE_STATE_SCANNING; // signal scan init
      NodeStates[NodeID - 1] &= ~NODE_STATE_BOOTED;
      break;
    case NODESTATUS_SCANCOMPLETE: // App can now access this node
      printf("SCANNED} "); 
      NodeStates[NodeID - 1] |= NODE_STATE_SCANFINISHED; // signal scan complete
      NodeStates[NodeID - 1] &= ~NODE_STATE_SCANNING;
      break;
    case NODESTATUS_SCANABORTED: 
      printf("SCAN ABORT} "); 
      NodeStates[NodeID - 1] &= ~NODE_STATE_SCANFINISHED; // signal scan abort
      NodeStates[NodeID - 1] &= ~NODE_STATE_SCANNING;
      break;
    case NODESTATUS_RESETAPP: printf("RESET APP} "); break;    
    case NODESTATUS_RESETCOM: printf("RESET COM} "); break;   
    case NODESTATUS_SLEEP: printf("SLEEP} "); break;   
    default: printf("UNKNOWN} "); break;
  }
}


/**************************************************************************
DOES:    Call-back function, data indication, new data arrived in device
**************************************************************************/
void NewData (
  unsigned char NodeID,     // node id from which this data arrived
  int Index,                // Index of Object Dictionary entry written to
  unsigned char Subindex,   // Subindex of Object Dictionary entry written to
  unsigned long DataLength, // data length of data written
  unsigned char *Data       // pointer to the data written
  )
{
  // is this our own status that changed?
  if (Index == 0x5F00)
  { // device status
    OwnStatusChanged(Subindex,*Data);
  }
  // is this a generic node status change?
  else if (Index == 0x5F04)
  { // node status
    NodeStatusChanged(Subindex,*Data);
  }
#if SHOW_NEW_DATA == 1
  else
  { // display raw data received
    printf("{%d:%4.4X,%2.2X;", NodeID, Index, Subindex);
    while(DataLength > 0)
    {
      printf(" %2.2X", *Data);
      Data++;
      DataLength--;
    }
    printf("} ");
  }
#endif // SHOW_NEW_DATA
}


/*******************************************************************************
DOES:    Call Back function indicates when an SDO request has finished
RETURNS: Nothing
*******************************************************************************/
void SDORequestComplete (
  UNSIGNED8 nodeid,                                        // node id that request was sent to
  UNSIGNED32 abortcode                                     // result of transfer
  )
{
  printf("[SDO-Complete 0x%2.2X:0x%8.8X] ", nodeid, abortcode);
}


/*******************************************************************************
DOES:    Called when user presses Ctrl-C. Sets a flag
RETURNS: Nothing
*******************************************************************************/
static void Terminate
  (
  int SignalNumber
  )
{
  TerminationRequested = TRUE;
}

/**************************************************************************
DOES:    Main function, open com port, run for 10 minutes
**************************************************************************/
int main(int argc, char* argv[])
{
  unsigned long result;
  time_t PDOTime;
  time_t EndTime;
  unsigned int *p32;
  unsigned char nodes = 0;
  unsigned short NMTCmd;
  char *ComPort;

  printf("\nCANopenIA Remote Access by www.esacademy.com\nV1.20 of 15-NOV-2017\n\n");

  if (argc != 2)
  {
    printf("Usage: RA_App <comportnumber>\n");
    return 1;
  }

  ComPort = argv[1];

#ifdef WIN32
  printf("Connecting to COM%s port...\n", ComPort);
#else
  printf("Connecting to %s...\n", ComPort);
#endif // !WIN32
  if (!COIADevice->Connect(argv[1], BAUDRATE))
  {
#ifdef WIN32
    printf("Failed to connect to COM%s port\n", ComPort);
#else
    printf("Failed to connect to %s\n", ComPort);
#endif // !WIN32
    delete COIADevice;
    return 1;
  }
#ifdef WIN32
  printf("Connected to COM%s port\n", ComPort);
#else
  printf("Connected to %s\n", ComPort);
#endif // !WIN32

#if SHOW_NEW_DATA == 1 
  printf("\nData in {NodeID:Index,Subindex;Data}-brackets is received in call back functions.\n\n");
#endif // SHOW_NEW_DATA

  // register callback functions
  COIADevice->RegisterDataCallback((DATACALLBACK *)NewData, NULL);
  COIADevice->RegisterSDORequestCallbacks((SDOREQUESTCOMPLETECALLBACK *)SDORequestComplete);

  p32 = (unsigned int *) &(DataBuf[0]);

  // get NMT state of node
  printf("\nRequesting NMT state of COIA node...\n");
  if ((result = COIADevice->ReadLocalOD(0x5F00, 0x02, &Length, &MyNMTState)) != ERROR_NOERROR)
  {
    printf("\nFailed to get NMT state of COIA node. Error code = 0x%8.8lX\n", result);
    printf("Closing port...\n");
    // disconnect from COM port, finished with COIA device
    delete COIADevice;
    return 0;  
  }
  printf("\nNMT State = 0x%2.2X", MyNMTState);

  // wait for packets for 600 seconds
  EndTime = time(NULL) + 600;
  printf("\n\nWaiting for a CiA401 device to appear...");
  printf("\nRunning for 10min, or until CTRL-C: ");

  // look for Ctrl-C - note, this will affect all instances of this class at once
  TerminationRequested = FALSE;
  signal(SIGINT, Terminate);

  // PDO Time 1s
  PDOTime = time(NULL) + 1;

  // if we are operational then reset all nodes to make them boot up
  // so we find them
  if (MyNMTState == NODESTATUS_OPERATIONAL)
  {
    NMTCmd = NMT_RESETCOM;
    if ((result = COIADevice->WriteLocalOD(0x5F0A, 0x01, 2, (unsigned char *)&NMTCmd)) != ERROR_NOERROR)
    {
      printf("\nFailed to reset all nodes");
    }
  }

  // keep going until termination has been requested
  while(!TerminationRequested)
  {
    if (MyNMTState == NODESTATUS_OPERATIONAL)
    { // only if we have a node ID and are operational
      nodes++;
      if (nodes >= MAX_NUMBER_OF_NODES)
      {
        nodes = 0;
      }
      // process nodes from 1 to MAX_NUMBER_OF_NODES
      if ((nodes + 1) != MyNodeID) 
      { 
        if ((NodeStates[nodes] & NODE_STATE_SCANFINISHED) != 0)
        { // nodes scan complete
          // read device type
          if ((result = COIADevice->ReadRemoteOD(nodes + 1, 0x1000, 0x00, &Length, DataBuf)) == ERROR_NOERROR)
          {
            if ((*p32 & 0x0000FFFFul) == 401)
            { // This is a CiA401 generic I/O device
              printf("[CiA401 device: data producer enabled] ");
              NodeStates[nodes] |= NODE_STATE_PRODUCEDATA; // produce data for this device
            }
            else
            {
              printf("[CiA %lu device: no handler] ",(*p32 & 0x00000FFFul));
            }
          }
          else
          {
             printf("[Error on device type read for node %d - 0x%8.8lX] ", nodes + 1, result);
             if (result == ERROR_NODEERROR) printf("Node error: 0x%8.8X ", COIADevice->GetLastNodeError());
          }
          // reset marker
          NodeStates[nodes] &= ~NODE_STATE_SCANFINISHED;
        }
        else if ((NodeStates[nodes] & NODE_STATE_PRODUCEDATA) != 0)
        { // PDO data production enabled
          if (time(NULL) >= PDOTime) 
          {
            DataBuf[0] = (unsigned char) PDOTime;
            COIADevice->WriteRemoteOD(nodes, 0x6200, 0x01, 1, DataBuf);
            DataBuf[0] = (unsigned char) (PDOTime >> 8);
            COIADevice->WriteRemoteOD(nodes, 0x6200, 0x02, 1, DataBuf);
            PDOTime = time(NULL) + 1;
          }
        }
      }
    }

    // keep receiving packets
    COIADevice->Process();

    // EndTime reached?
    if (time(NULL) >= EndTime) break;
  }

  // de-register callback functions
  COIADevice->RegisterDataCallback(NULL, NULL);
  COIADevice->RegisterSDORequestCallbacks(NULL);

  COIADevice->Disconnect();
#ifdef WIN32
  printf("\nDisconnected from COM%s...\n", ComPort);
#else
  printf("\nDisconnected from %s...\n", ComPort);
#endif // !WIN32

  // disconnect from COM port, finished with COIA device
  delete COIADevice;

  return 0;
}

/*----------------------- END OF FILE ----------------------------------*/
