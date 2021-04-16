#!/bin/sh
MY_PATH="`dirname \"$0\"`"
. $MY_PATH/tool-path.sh
TOOL_PATH=$MY_PATH/$COIA_PATH
echo coia batch: Write drive command out node 8
[ -z "$1" ] && echo "Use COWr SerialPort" && exit 1
echo Use: coia write node 8 drive command
$TOOL_PATH/coia -p $1 --node-write 8,0x6040,0,2,0x1234 --node-write 8,0x6060,0,1,0xAA --node-write 8,0x607A,0,4,0x87654321
$TOOL_PATH/coia -p $1 --delay 1000 -w 0x5F01,0x0C,2,0x0802
$TOOL_PATH/coia -p $1 --delay 1000 -w 0x5F01,0x0C,2,0x0803
echo coia batch COWr: end
