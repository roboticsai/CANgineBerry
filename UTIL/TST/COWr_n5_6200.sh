#!/bin/sh
MY_PATH="`dirname \"$0\"`"
. $MY_PATH/tool-path.sh
TOOL_PATH=$MY_PATH/$COIA_PATH
echo coia batch: Write PDO Data digital out node 5
[ -z "$1" ] && echo "Use COWr SerialPort" && exit 1
echo Use: coia write node 5 digital out
$TOOL_PATH/coia -p $1 --node-write 5,0x6200,1,1,0x55 --node-write 5,0x6200,2,1,0x66 --node-write 5,0x6200,3,1,0x77 --node-write 5,0x6200,4,1,0x88
$TOOL_PATH/coia -p $1 --node-write 5,0x6200,1,1,0x56 --node-write 5,0x6200,2,1,0x67 --node-write 5,0x6200,3,1,0x78 --node-write 5,0x6200,4,1,0x89
$TOOL_PATH/coia -p $1 --node-write 5,0x6200,1,1,0x57 --node-write 5,0x6200,2,1,0x68 --node-write 5,0x6200,3,1,0x79 --node-write 5,0x6200,4,1,0x8A
echo coia batch COWr: end
