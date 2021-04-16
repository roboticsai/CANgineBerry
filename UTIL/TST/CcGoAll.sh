#!/bin/sh
MY_PATH="`dirname \"$0\"`"
. $MY_PATH/tool-path.sh
TOOL_PATH=$MY_PATH/$COIA_PATH
echo coia batch: Start CANcrypt, here group 2,3,7
[ -z "$3" ] && echo "Use CcGoAll SerialPort_Device_2 SerialPort_Device_3 SerialPort_Device_7" && exit 1
echo coia Activate CANcrypt, ignore sec list, device ID 2, 3 and 7
$TOOL_PATH/coia -p $1 -w 0x5EF0,5,2,0x0006 -w 0x5EF0,6,2,0x0082 -w 0x5EF0,3,1,0x20 -w 0x5eF0,2,1,2
$TOOL_PATH/coia -p $2 -w 0x5EF0,5,2,0x0006 -w 0x5EF0,6,2,0x0082 -w 0x5EF0,3,1,0x20 -w 0x5eF0,2,1,3
$TOOL_PATH/coia -p $3 -w 0x5EF0,5,2,0x0006 -w 0x5EF0,6,2,0x0082 -w 0x5EF0,3,1,0x20 -w 0x5eF0,2,1,7
echo coia batch CcGoAll: end
