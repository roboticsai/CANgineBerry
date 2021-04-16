#!/bin/sh
MY_PATH="`dirname \"$0\"`"
. $MY_PATH/tool-path.sh
TOOL_PATH=$MY_PATH/$COIA_PATH
echo coia batch: Erase a CANcrypt key \(ID from 2 to 6\)
[ -z "$2" ] && echo "Use CcEraseKey SerialPort KeyID" && exit 1
$TOOL_PATH/coia -p $1 -w 0x5EF2,$2,4,0xFFFFFFFF
echo coia batch CcEraseKey $1 $2 completed

