#!/bin/sh
MY_PATH="`dirname \"$0\"`"
. $MY_PATH/tool-path.sh
TOOL_PATH=$MY_PATH/$COIA_PATH
echo coia batch: Write PDO Data digital out node 5, enforce SDO. no PDO
[ -z "$1" ] && echo "Use COWr SerialPort" && exit 1
echo Use: coia write node 5 enforce SDO, digital out
$TOOL_PATH/coia -p $1 --node-write 5,0x6200,1,1,0x11 --node-write 5,0x6200,2,1,0x22 --node-write 5,0x6200,3,1,0x33 --node-write 5,0x6200,4,1,0x44
$TOOL_PATH/coia -p $1 -w 0x5F01,4,4,0x0000003F
$TOOL_PATH/coia -p $1 --node-write 5,0x6200,1,1,0x55 --node-write 5,0x6200,2,1,0x66 --node-write 5,0x6200,3,1,0x77 --node-write 5,0x6200,4,1,0x88
$TOOL_PATH/coia -p $1 -w 0x5F01,4,4,0x0000001F
$TOOL_PATH/coia -p $1 --node-write 5,0x6200,1,1,0xCC --node-write 5,0x6200,2,1,0xDD --node-write 5,0x6200,3,1,0xEE --node-write 5,0x6200,4,1,0xFF
echo coia batch COWr: end
