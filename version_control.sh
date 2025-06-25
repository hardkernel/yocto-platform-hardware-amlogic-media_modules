#!/bin/sh
GIT_BIN="/usr/bin/git"

IS_DDK=$1
if [ -z "$IS_DDK" ]; then
    MEDIA_MODULE_PATH=$(cd "$(dirname "$0")";pwd)
else
    #MEDIA_MODULE_PATH="$(pwd)/../../../../../../../../../../driver_modules/media_modules/"
    PWD_PATH="$(pwd)"
    MEDIA_MODULE_PATH="${PWD_PATH%%/common*}/common/driver_modules/media_modules"
fi
CHECK_MSG=$MEDIA_MODULE_PATH/firmware/checkmsg
UCODE_BIN=$MEDIA_MODULE_PATH/firmware/video_ucode.bin

Major_V=$(cd ${MEDIA_MODULE_PATH}; grep "Major_V" VERSION  | awk -F [=] '{print $2}')
Minor_V=$(cd ${MEDIA_MODULE_PATH}; grep "Minor_V" VERSION  | awk -F [=] '{print $2}')
BASE_CHANGEID=$(cd ${MEDIA_MODULE_PATH}; grep "^DevelopingChangeId" VERSION | awk -F [=] '{print $2}' | cut -c1-6)
#MEDIAMODULE_CHANGEID=$(cd ${MEDIA_MODULE_PATH}; $GIT_BIN log -1 ${MEDIA_MODULE_PATH} | grep "Change-Id: " | awk '{ print $2}' | cut -c1-6 | tail -1)
COMMIT_COUNT=$(cd ${MEDIA_MODULE_PATH}; $GIT_BIN log | grep "Change-Id: " | grep -n ${BASE_CHANGEID} | awk -F ":" '{printf "%d", $1-1}' )
MEDIAMODULE_COMMITID=$(cd ${MEDIA_MODULE_PATH}; $GIT_BIN rev-parse --short HEAD)
UCODE_VERSION_DETAIL=$(cd ${MEDIA_MODULE_PATH}; $CHECK_MSG $UCODE_BIN | grep "ver" | awk '{print $3}' | sed 's/v//g')
UCODE_VERSION=$(cd ${MEDIA_MODULE_PATH}; $CHECK_MSG $UCODE_BIN | grep "ver    :"  | awk -F '[v-]' '{print $3}' | awk -F [\.] '{printf "%d%02d%03d", $1,$2,$3}')
RELEASED_VERSION=$(cd ${MEDIA_MODULE_PATH}; grep "^#V" VERSION | head -1 | awk  '{print $1}' | sed 's/#//g')
LTS_FLAG=$(cd ${MEDIA_MODULE_PATH}; grep "^LTS_FLAG=" VERSION | awk -F "=" '{printf $2}')
RELEASE_FLAG=$(cd ${MEDIA_MODULE_PATH}/; grep "^RELEASE_FLAG=" VERSION | awk -F "=" '{printf $2}')

if [ -n "${COMMIT_COUNT}" ]; then
    DEV_FLAG="-dev"
    [ "${RELEASE_FLAG}" = "1" ] && DEV_FLAG=""
    VERSION_CONTROL_CFLAGS="-DDECODER_VERSION=${Major_V}.${Minor_V}${DEV_FLAG:+$DEV_FLAG}.${COMMIT_COUNT}-g${MEDIAMODULE_COMMITID}.${UCODE_VERSION}${LTS_FLAG}"
else
    DEV_FLAG=""
    [ "${RELEASE_FLAG}" != "1" ] && DEV_FLAG="-dev"
    VERSION_CONTROL_CFLAGS="-DRELEASED_VERSION=${RELEASED_VERSION}${DEV_FLAG}"
fi

VERSION_CONTROL_CFLAGS="${VERSION_CONTROL_CFLAGS} -DUCODE_VERSION=${UCODE_VERSION_DETAIL}"

if [ -z "$IS_DDK" ]; then
    echo ${VERSION_CONTROL_CFLAGS}
else
    #chmod 777 $MEDIA_MODULE_PATH/drivers/frame_provider/decoder/utils/vdec_version.h
    #echo > $MEDIA_MODULE_PATH/drivers/frame_provider/decoder/utils/vdec_version.h
    for define in $VERSION_CONTROL_CFLAGS; do
        name="${define#-D}"
        value="${name#*=}"
        name="${name%=*}"

        echo "#define $name $value"
    done
    #chmod 644 $MEDIA_MODULE_PATH/drivers/frame_provider/decoder/utils/vdec_version.h
fi
