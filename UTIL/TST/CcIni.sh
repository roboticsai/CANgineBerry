#!/bin/sh
MY_PATH="`dirname \"$0\"`"
. $MY_PATH/tool-path.sh
TOOL_PATH=$MY_PATH/$COIAUPDATER_PATH
echo coiaupdater batch: Configure node ID and 125 kbps and LSS disabled for CANgineBerry
[ -z "$2" ] && echo "Use CcIni SerialPort NodeID" && exit 1
$TOOL_PATH/coiaupdater -p $1 -n $2 -r 125 -l 0
$TOOL_PATH/coiaupdater -p $1 -i
echo coiaupdater batch CcIni: end
