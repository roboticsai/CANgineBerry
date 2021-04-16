#!/bin/sh
MY_PATH="`dirname \"$0\"`"
. $MY_PATH/tool-path.sh
TOOL_PATH=$MY_PATH/$COIA_PATH
echo coia batch: Write PDO Data digital out node 3
[ -z "$1" ] && echo "Use COWr SerialPort" && exit 1
echo Use: coia write node 3 digital out
$TOOL_PATH/coia -p $1 --node-write 3,0x6200,1,1,0xC0 --delay 75 --node-write 3,0x6200,2,1,0x75 --delay 75 --node-write 3,0x6200,3,1,0xDE --delay 75 --node-write 3,0x6200,4,1,0x1A
echo coia batch COWr: end
