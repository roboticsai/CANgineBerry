#!/bin/sh
MY_PATH="`dirname \"$0\"`"
. $MY_PATH/tool-path.sh
TOOL_PATH=$MY_PATH/$COIAUPDATER_PATH
echo Calling $TOOL_PATH/coiaupdater "$@"
$TOOL_PATH/coiaupdater "$@"
