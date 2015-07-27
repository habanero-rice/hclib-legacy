#!/bin/bash

function checkDefine() {
    NAME=$1
    FILE=$2
    RES1=`grep "$NAME" ${FILE}`
    RES2=`echo "$RES1" | grep -e "//"`
    # Very very basic check
    if [ -z "$RES1" -o -n "$RES2" ]; then
        NAME=`echo ${NAME} | cut -d' ' -f2-2`
        echo "error ${NAME} is not defined in ${FILE}"
        exit 1
    fi
}

function printSetup() {
    echo ""
    echo "**"
    echo ""
    echo "OCR x86 successfully installed under ${OCR_SRC_ROOT}/install/x86"
    echo "Update your environment with the following variable:"
    echo ""
    echo "export OCR_INSTALL=\${OCR_SRC_ROOT}/install/x86"
    echo "export PATH=\${OCR_INSTALL}/bin:\${PATH}"
    echo "export LD_LIBRARY_PATH=\${OCR_INSTALL}/lib:\${LD_LIBRARY_PATH}"
    echo "export OCR_CONFIG=\${OCR_INSTALL}/config/default.cfg"
    echo ""
    echo "**"
}

#
# Check OCR common.mk
#
echo "Tweaking OCR common.mk"
OCR_COMMON_MK=${OCR_SRC_ROOT}/build/common.mk

# Set OCR ELS size
sed -i s/"# CFLAGS += -DELS_USER_SIZE=0"/"CFLAGS += -DELS_USER_SIZE=1"/g ${OCR_COMMON_MK}

#
# Check OCR x86 ocr-config.h
#
echo "Tweaking OCR x86/ocr-config.h"
OCR_BUILD_CFGFILE=${OCR_SRC_ROOT}/build/x86/ocr-config.h

# Check for legacy support
DEF_EXT_LEGACY="#define ENABLE_EXTENSION_LEGACY"
checkDefine "${DEF_EXT_LEGACY}" "${OCR_BUILD_CFGFILE}"

# Check for runtime extension support
DEF_EXT_RTITF="#define ENABLE_EXTENSION_RTITF"
checkDefine "${DEF_EXT_RTITF}" "${OCR_BUILD_CFGFILE}"

# Check for blocking calls support
DEF_EXT_BLOCKING="#define ENABLE_SCHEDULER_BLOCKING_SUPPORT"
checkDefine "${DEF_EXT_BLOCKING}" "${OCR_BUILD_CFGFILE}"

#
# Build OCR x86
#
echo "Build OCR x86"
cd ${OCR_SRC_ROOT}/build/x86
make clean && make && make install && printSetup
cd -



