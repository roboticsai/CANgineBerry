#!/bin/sh
MY_PATH="`dirname \"$0\"`"
. $MY_PATH/tool-path.sh
TOOL_PATH=$MY_PATH/$COIA_PATH
echo Calling $TOOL_PATH/coia "$@"
$TOOL_PATH/coia "$@"
