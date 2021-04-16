#!/bin/sh
MY_PATH="`dirname \"$0\"`"
. $MY_PATH/tool-path.sh
TOOL_PATH=$MY_PATH/$COIA_PATH
echo coia batch: Write PDO Data digital out node 3
[ -z "$1" ] && echo "Use COWr SerialPort" && exit 1
echo Use: coia write node 3 digital out
$TOOL_PATH/coia -p $1 --pdo-update-time 0 --pdo-event-time 100 --pdo-inhibit-time 0
echo coia batch COWr: end
