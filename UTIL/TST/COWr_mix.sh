#!/bin/sh
MY_PATH="`dirname \"$0\"`"
. $MY_PATH/tool-path.sh
TOOL_PATH=$MY_PATH/$COIA_PATH
echo coia batch: Write PDO Data digital out node 3
[ -z "$1" ] && echo "Use COWr SerialPort" && exit 1
echo Use: coia write node 3 digital out
$TOOL_PATH/coia -p $1 --node-write 3,0x6200,1,1,0x5A --delay 10 --node-write 3,0x6200,2,1,0xA5 --delay 10 --node-write 3,0x6200,4,1,0x0B --delay 10 --node-write 3,0x6200,3,1,0xAD
echo coia batch COWr: end
