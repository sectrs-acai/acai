#!/bin/bash
set -euo pipefail
source $(git rev-parse --show-toplevel)/env.sh

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

function do_init {
    echo "init"
}

function do_clean {
    make -f ./Makefile clean
}

function do_build_tests {
    make -f ./Makefile tests DEBUG=1
}

function do_run_tests {
    make -f ./Makefile run-tests
}

function do_build_linux {
    make -f ./Makefile linux
}

function do_run_linux {
    local p9_folder=$ROOT_DIR
    local preload=$ASSETS_DIR/fvp/bin/libhook-libc-2.31.so


    local asset_pre=$ASSETS_DIR/snapshots
    local bl1=$asset_pre/bl1.bin
    local fip=$asset_pre/fip.bin

    # asset output
    local image=$ASSETS_DIR/tfa/tfa-unmod-realm-ready/Image
    local rootfs=$ASSETS_DIR/busybox-buildroot-lkvm-rootfs.ext2

    # buildroot output
    #local image=$OUTPUT_LINUX_CCA_GUEST_DIR/images/Image
    #local rootfs=$OUTPUT_LINUX_CCA_GUEST_DIR/images/rootfs.ext4

    $SCRIPTS_DIR/run_fvp.sh $bl1 $fip $image $rootfs $p9_folder $preload
}

# "${@:2}"
case "$1" in
    clean)
        do_clean
        ;;
    init)
        do_init
        ;;
    tests)
        do_build_tests
        ;;
    linux)
        do_build_linux
        ;;
    run_tests)
        do_run_tests
        ;;
    run_linux)
        do_run_linux
        ;;
    *)
        echo "unknown"
        exit 1
esac
