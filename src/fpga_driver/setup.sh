#!/bin/bash

#
# builds all major components for FPGA device interactions
# - requires buildroot toolchain installed
#

set -euo pipefail

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
PROJ_ROOT=$SCRIPT_DIR/../..

source ${PROJ_ROOT}/env.sh

set +x

function do_init {
    echo "do init..."
}

function do_clean {
    echo "do clean..."
}

function do_fh_host
{
    cd $PROJ_ROOT/scripts
    source env-x86.sh

    # build helpers
    cd $PROJ_ROOT/src/fpga_driver/fh_host
    make
}

function do_libhook
{
    cd $PROJ_ROOT/scripts
    source env-x86.sh

    # build libhook
    cd $PROJ_ROOT/src/fpga_driver/libhook
    make
}

function do_fpga_usr_manager
{
    cd $PROJ_ROOT/scripts
    source env-x86.sh

    # build userspace manager
    cd $PROJ_ROOT/src/fpga_driver/fpga_escape_libhook
    make
}


function do_intercept
{
    cd $PROJ_ROOT/scripts
    source env-aarch64.sh

    cd $PROJ_ROOT/src/fpga_driver/devmem_intercept
    make
}


function do_xdma_stub
{
    cd $PROJ_ROOT/scripts
    source env-aarch64.sh


    cd $PROJ_ROOT/src/fpga_driver/xdma_stub/linux-kernel/xdma/
    make

    cd $PROJ_ROOT/src/fpga_driver/xdma_stub/linux-kernel/tools
    make
}


function do_xdma
{
    cd $PROJ_ROOT/scripts
    source env-x86.sh

    cd $PROJ_ROOT/src/fpga_driver/xdma/linux-kernel/xdma
    make

    cd $PROJ_ROOT/src/fpga_driver/xdma_stub/linux-kernel/tools
    make
}


function do_build_aarch64 {
    do_intercept
    do_xdma_stub
}

function do_build_x86 {
    do_fh_host
    do_libhook
    do_fpga_usr_manager
    do_xdma
    do_pteditor
}

function do_pteditor {
    echo $PROJ_ROOT
    set -x
    cd $PROJ_ROOT/scripts
    source ./env-x86.sh
    cd $PROJ_ROOT/ext/pteditor/module/
    make
}

# "${@:2}"
case "$1" in
    clean)
        do_clean
        ;;
    init)
        do_init
        ;;
      build)
         do_build_aarch64
         do_build_x86
         ;;
    fh_host)
        do_fh_host
        ;;
    libhook)
        do_libhook
        ;;
    fpga_usr_manager)
        do_fpga_usr_manager
        ;;
    build_aarch64)
        do_build_aarch64
        ;;
    build_x86)
        do_build_x86
        ;;
    xdma)
        do_xdma
        ;;
    xdma_stub)
        do_xdma_stub
        ;;
    intercept)
        do_intercept
        ;;
    pteditor)
        do_pteditor
        ;;
    *)
        echo "unknown command given"
        exit 1
        ;;
esac


echo "-- script executed successfully"
