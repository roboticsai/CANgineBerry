#!/bin/sh
MY_PATH="`dirname \"$0\"`"
. $MY_PATH/tool-path.sh
TOOL_PATH=$MY_PATH/$COIAUPDATER_PATH
echo coiaupdater batch: Flash devices
[ -z "$3" ] && echo "Use CcFlashAll SerialPort_1 SerialPort_2 SerialPort_3" && exit 1
$TOOL_PATH/coiaupdater -p $1 -f CgB_coia_BEDS1.4_sec.bin
$TOOL_PATH/coiaupdater -p $2 -f CgB_coia_BEDS1.4_sec.bin
$TOOL_PATH/coiaupdater -p $3 -f CgB_coia_BEDS1.4_sec.bin
echo coiaupdater batch CcFlashEnd: end
