#!/bin/sh
MY_PATH="`dirname \"$0\"`"
. $MY_PATH/tool-path.sh
TOOL_PATH=$MY_PATH/$COIA_PATH
echo coia batch: Write PDO Data digital out node 5, disable auto PDO
[ -z "$1" ] && echo "Use COWr SerialPort" && exit 1
echo Use: coia write node 5 disable auto PDO, digital out
$TOOL_PATH/coia -p $1 --node-write 5,0x6200,1,1,0x15 --node-write 5,0x6200,2,1,0x16 --node-write 5,0x6200,3,1,0x17 --node-write 5,0x6200,4,1,0x18
$TOOL_PATH/coia -p $1 -w 0x5F01,4,4,0x0000005F
$TOOL_PATH/coia -p $1 --node-write 5,0x6200,1,1,0x25 --node-write 5,0x6200,2,1,0x26 --node-write 5,0x6200,3,1,0x27 --node-write 5,0x6200,4,1,0x28
$TOOL_PATH/coia -p $1 --node-write 5,0x6200,1,1,0x35 --node-write 5,0x6200,2,1,0x36 --node-write 5,0x6200,3,1,0x37 --node-write 5,0x6200,4,1,0x38
$TOOL_PATH/coia -p $1 --delay 1000 -w 0x5F01,0x0C,2,0x0501 -w 0x5F01,4,4,0x0000001F
$TOOL_PATH/coia -p $1 --node-write 5,0x6200,1,1,0x45 --node-write 5,0x6200,2,1,0x46 --node-write 5,0x6200,3,1,0x47 --node-write 5,0x6200,4,1,0x48
$TOOL_PATH/coia -p $1 --delay 1000 -w 0x5F01,0x0C,2,0x0501
echo coia batch COWr: end
