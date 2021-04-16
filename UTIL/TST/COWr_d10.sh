#!/bin/sh
MY_PATH="`dirname \"$0\"`"
. $MY_PATH/tool-path.sh
TOOL_PATH=$MY_PATH/$COIA_PATH
echo coia batch: Write PDO Data digital out node 3
[ -z "$1" ] && echo "Use COWr SerialPort" && exit 1
echo Use: coia write node 3 digital out
$TOOL_PATH/coia -p $1 --node-write 3,0x6000,1,1,0x55 --delay 10 --node-write 3,0x6000,2,1,0x66 --delay 10 --node-write 3,0x6000,3,1,0x77 --delay 10 --node-write 3,0x6000,4,1,0x88
echo coia batch COWr: end
