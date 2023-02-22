#!/bin/bash
set -euo pipefail
source $(git rev-parse --show-toplevel)/env.sh

#
# 22-02-22: abertschi: I am currently having linux CAP issues
# using the yocto build of tfa when booting a realm,
# so this script uses shrinkwrap to build tfa
# as this was much easier to troubleshoot
#
# - build tfa, rmm, kernel, with projroot/src/* repositores
# - use precompiled buildroot rootfile system
#
# we overwrite default shrinkwrap default configs with overlay
# you find the orig configs in ./projroot/ext/shrinkwrap/config
# dt: fvp-base-psci-common.dtsi


SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
SHRINKWRAP_DIR=$EXT_DIR/shrinkwrap
export PATH=$SHRINKWRAP_DIR/shrinkwrap:$PATH
SHRINKWRAP_EXE=$SHRINKWRAP_DIR/shrinkwrap/shrinkwrap

OVERLAY_ORIG=./3world-overlay.yaml
OVERLAY=${OVERLAY_ORIG}.tmp

function update_overlay {
    cd $SCRIPT_DIR
    cp -f $OVERLAY_ORIG $OVERLAY
    local root=$ROOT_DIR
    sed -i "s#ROOT_DIR#${ROOT_DIR}#g" $OVERLAY
}

function do_init {
    update_overlay

    # dependencies required for shrinkwrap
    # https://shrinkwrap.docs.arm.com/en/latest/overview.html
    pip3 show pyyaml termcolor tuxmake graphlib-backport
}

function do_local {
    do_init
    cd $SCRIPT_DIR
    make -f ./Makefile_local.mk
}

function do_clean {
    do_init

    cd $SCRIPT_DIR
    $SHRINKWRAP_EXE clean cca-3world.yaml --overlay $OVERLAY
}

function do_compile {
    update_overlay
    cd $SCRIPT_DIR
    $SHRINKWRAP_EXE build cca-3world.yaml --overlay $OVERLAY --dry-run
}

function do_run {
    local p9_folder=$ROOT_DIR
    local preload=$ASSETS_DIR/fvp/bin/libhook-libc-2.31.so

    local shrinkwrap=$HOME/.shrinkwrap/package/cca-3world/
    local pre=$ASSETS_DIR/tfa

    if [[ true ]]; then
        # prebuild
        local asset_pre=$ASSETS_DIR/tfa/tfa-unmod-realm-ready
        local bl1=$asset_pre/bl1.bin
        local fip=$asset_pre/fip.bin
        local image=$asset_pre/Image
        local rootfs=$ASSETS_DIR/busybox-buildroot-lkvm-rootfs.ext2
    else
        # compiled
        local bl1=$shrinkwrap/bl1.bin
        local fip=$shrinkwrap/fip.bin

        local image_shrinkwrap=$shrinkwrap/Image
        local image_buildroot=$OUTPUT_LINUX_GUEST_DIR/images/Image
        local image=$image_shrinkwrap

        local rootfs_buildroot=$OUTPUT_LINUX_GUEST_DIR/images/rootfs.ext4
        local rootfs_prebuild=$ASSETS_DIR/busybox-buildroot-lkvm-rootfs.ext2
        local rootfs=$rootfs_prebuild
    fi


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
    compile)
        do_compile
        ;;
    build)
        do_compile
        do_run
        ;;
    run)
        do_run
        ;;
    local)
        do_local
        ;;
    *)
        echo "unknown"
        exit 1
esac
