#!/bin/sh
MY_PATH="`dirname \"$0\"`"
. $MY_PATH/tool-path.sh
TOOL_PATH=$MY_PATH/$COIA_PATH
echo coia batch: Read CANcrypt parameters
[ -z "$1" ] && echo "Use CcRd SerialPort" && exit 1
echo Use: coia Read CANcrypt parameters
$TOOL_PATH/coia -p $1 -r 0x5EF0,1 -r 0x5EF0,2 -r 0x5EF0,3 -r 0x5EF0,4 -r 0x5EF0,5 -r 0x5EF0,6
echo coia batch CcRd: end
