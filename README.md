CANgineBerry: Firmware, Software & Manuals Archive
==================================================
THIS ARCHIVE CONTAINS THE WINDOWS(R) VERSION

COPYRIGHT: Embedded Systems Academy, Inc. 2017-2019
           www.cangineberry.com

VERSION:   1.06, ESA 29-APR-19

NOTES:     This kit does not include a CAN or CANopen tutorial.
           See www.canopenbook.com to get started with CANopen

Scope of File Delivery
======================

Directory "APPS"
Contains the various applications that can be loaded into the 
CANgineBerry using the COIAUpdater utility.
 + CANopenIA-BEDS: CANopen Device (slave)
 + CANopenIA-MGR:  CANopen Manager (master)

Directory "CODE":
Contains code examples

Directory "DOC"
Contains the main CANgineBerry documentation of the hardware, the 
bootloader and the COIAUpdater utility.

Directory "UTIL"
Contains the utilities provided.
 + COIA:          generic command line utility for communicating
                  with the CANopenIA-MGR and CANopenIA-BEDS apps
 + COIAUpdater:   load applications and configuration files to
                  the CANgineBerry, configure hardware settings
 + TST (dir):     directory with shell scripts as example and
                  for testing


Step-by-step First Operation
============================

1) Connect CANgineBerry to the host computer system, like a 
Raspberry Pi or Windows PC using a USB-to-UART module
(when using such a module, ensure voltage levels are as required: 
5V supply, but only 3.3V signals)

2) On host computer system start the COIAUpdater utility to connect 
to the CANgineBerry. Use option "-h" for help.
You must know the UART/COM port (e.g. 'COM5') used. Example:

$ COIAUpdater -p 5 -i

3) If the CANgineBerry does not have the desired firmware and/or
configuration stored, use the COIAUpdater utility to load
the firmware and configuration wanted.

4) Depending on firmware loaded, use COIA, RemoteAccess or
other utilities on the host system to operate the
CANgineBerry.


----- END - OF - FILE -----
