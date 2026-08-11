#!/usr/bin/sh
# 参数说明：
# $1 - params path: echo_app_top
# $2 - user cmd: build / clean / ...

print_not_null()
{
    # $1 为空，返回错误
    if [ x"$1" = x"" ]; then
        return 1
    fi

    echo "$1"
}


set -e
cd `dirname $0`

BUILD_PARAM_DIR=$1
BUILD_PARAM_FILE=$BUILD_PARAM_DIR/build_param.config
. $BUILD_PARAM_FILE

EXAMPLE_NAME=$CONFIG_PROJECT_NAME
EXAMPLE_VER=$CONFIG_PROJECT_VERSION
HEADER_DIR=$OPEN_HEADER_DIR
LIBS_DIR=$OPEN_LIBS_DIR
LIBS=$PLATFORM_NEED_LIBS
OUTPUT_DIR=$BIN_OUTPUT_DIR
USER_CMD=$2
TARGET=$PLATFORM_CHIP
BOARD_NAME=$PLATFORM_BOARD

# echo EXAMPLE_NAME=$EXAMPLE_NAME
# echo EXAMPLE_VER=$EXAMPLE_VER
# echo HEADER_DIR=$HEADER_DIR
# echo LIBS_DIR=$LIBS_DIR
# echo LIBS=$LIBS
# echo OUTPUT_DIR=$OUTPUT_DIR
# echo USER_CMD=$USER_CMD
# echo BOARD_NAME=$BOARD_NAME
# exit 0

if [ x"$USER_CMD" = x"build" ]; then
    USER_CMD=all
fi

mkdir -p $OUTPUT_DIR
make EXAMPLE_NAME=$EXAMPLE_NAME EXAMPLE_VER=$EXAMPLE_VER LIBS_DIR=$LIBS_DIR LIBS="$LIBS" OUTPUT_DIR=$OUTPUT_DIR HEADER_DIR="$HEADER_DIR" $USER_CMD

if [ x$USER_CMD = "xclean" ];then
	echo "*************************************************************************"
	echo "************************CLEAN SUCCESS************************************"
	echo "*************************************************************************"
	exit 0
fi

