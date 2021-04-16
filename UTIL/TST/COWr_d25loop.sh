#!/bin/sh
MY_PATH="`dirname \"$0\"`"
. $MY_PATH/tool-path.sh
TOOL_PATH=$MY_PATH/$COIA_PATH
echo coia batch: Write PDO Data digital out node 3
[ -z "$1" ] && echo "Use COWr SerialPort" && exit 1
echo Use: coia write node 3 digital out
$TOOL_PATH/coia -p $1 --node-write 3,0x6200,1,1,0x50 --delay 25 --node-write 3,0x6200,2,1,0x66 --delay 25 --node-write 3,0x6200,3,1,0x77 --delay 25 --node-write 3,0x6200,4,1,0x8F
$TOOL_PATH/coia -p $1 --node-write 3,0x6200,1,1,0x51 --delay 25 --node-write 3,0x6200,2,1,0x66 --delay 25 --node-write 3,0x6200,3,1,0x77 --delay 25 --node-write 3,0x6200,4,1,0x8E
$TOOL_PATH/coia -p $1 --node-write 3,0x6200,1,1,0x52 --delay 25 --node-write 3,0x6200,2,1,0x66 --delay 25 --node-write 3,0x6200,3,1,0x77 --delay 25 --node-write 3,0x6200,4,1,0x8D
$TOOL_PATH/coia -p $1 --node-write 3,0x6200,1,1,0x53 --delay 25 --node-write 3,0x6200,2,1,0x66 --delay 25 --node-write 3,0x6200,3,1,0x77 --delay 25 --node-write 3,0x6200,4,1,0x8C
$TOOL_PATH/coia -p $1 --node-write 3,0x6200,1,1,0x54 --delay 25 --node-write 3,0x6200,2,1,0x66 --delay 25 --node-write 3,0x6200,3,1,0x77 --delay 25 --node-write 3,0x6200,4,1,0x8B
$TOOL_PATH/coia -p $1 --node-write 3,0x6200,1,1,0x55 --delay 25 --node-write 3,0x6200,2,1,0x66 --delay 25 --node-write 3,0x6200,3,1,0x77 --delay 25 --node-write 3,0x6200,4,1,0x8A
$TOOL_PATH/coia -p $1 --node-write 3,0x6200,1,1,0x56 --delay 25 --node-write 3,0x6200,2,1,0x66 --delay 25 --node-write 3,0x6200,3,1,0x77 --delay 25 --node-write 3,0x6200,4,1,0x89
$TOOL_PATH/coia -p $1 --node-write 3,0x6200,1,1,0x57 --delay 25 --node-write 3,0x6200,2,1,0x66 --delay 25 --node-write 3,0x6200,3,1,0x77 --delay 25 --node-write 3,0x6200,4,1,0x88
$TOOL_PATH/coia -p $1 --node-write 3,0x6200,1,1,0x58 --delay 25 --node-write 3,0x6200,2,1,0x66 --delay 25 --node-write 3,0x6200,3,1,0x77 --delay 25 --node-write 3,0x6200,4,1,0x87
$TOOL_PATH/coia -p $1 --node-write 3,0x6200,1,1,0x59 --delay 25 --node-write 3,0x6200,2,1,0x66 --delay 25 --node-write 3,0x6200,3,1,0x77 --delay 25 --node-write 3,0x6200,4,1,0x86
$TOOL_PATH/coia -p $1 --node-write 3,0x6200,1,1,0x5A --delay 25 --node-write 3,0x6200,2,1,0x66 --delay 25 --node-write 3,0x6200,3,1,0x77 --delay 25 --node-write 3,0x6200,4,1,0x85
$TOOL_PATH/coia -p $1 --node-write 3,0x6200,1,1,0x5B --delay 25 --node-write 3,0x6200,2,1,0x66 --delay 25 --node-write 3,0x6200,3,1,0x77 --delay 25 --node-write 3,0x6200,4,1,0x84
$TOOL_PATH/coia -p $1 --node-write 3,0x6200,1,1,0x5C --delay 25 --node-write 3,0x6200,2,1,0x66 --delay 25 --node-write 3,0x6200,3,1,0x77 --delay 25 --node-write 3,0x6200,4,1,0x83
$TOOL_PATH/coia -p $1 --node-write 3,0x6200,1,1,0x5D --delay 25 --node-write 3,0x6200,2,1,0x66 --delay 25 --node-write 3,0x6200,3,1,0x77 --delay 25 --node-write 3,0x6200,4,1,0x82
$TOOL_PATH/coia -p $1 --node-write 3,0x6200,1,1,0x5E --delay 25 --node-write 3,0x6200,2,1,0x66 --delay 25 --node-write 3,0x6200,3,1,0x77 --delay 25 --node-write 3,0x6200,4,1,0x81
$TOOL_PATH/coia -p $1 --node-write 3,0x6200,1,1,0x5F --delay 25 --node-write 3,0x6200,2,1,0x66 --delay 25 --node-write 3,0x6200,3,1,0x77 --delay 25 --node-write 3,0x6200,4,1,0x80
echo coia batch COWr: end
