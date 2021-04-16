#!/bin/sh
MY_PATH="`dirname \"$0\"`"
. $MY_PATH/tool-path.sh
TOOL_PATH=$MY_PATH/$COIA_PATH
echo coia batch: Start CANcrypt, here group 2,3,7
[ -z "$2" ] && echo "Use CcGo SerialPort CANcryprtDeviceID" && exit 1
echo coia Activate CANcrypt, ignore sec list, device ID $2
$TOOL_PATH/coia -p $1 -w 0x5EF0,5,2,0x0006 -w 0x5EF0,6,2,0x0082 -w 0x5EF0,3,1,0x20 -w 0x5eF0,2,1,$2
echo coia batch CcGo: end
