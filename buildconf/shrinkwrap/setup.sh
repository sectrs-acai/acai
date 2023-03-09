#!/bin/bash
set -euo pipefail
source $(git rev-parse --show-toplevel)/env.sh

# build tfa, rmm, kernel, with projroot/src/* repositores.
# use precompiled buildroot rootfile system.
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

OVERLAY_KVMTOOL_ORIG=./kvmtool.yaml
OVERLAY_KVMTOOL=${OVERLAY_KVMTOOL_ORIG}.tmp


function update_overlay {
    cd $SCRIPT_DIR
    cp -f $OVERLAY_ORIG $OVERLAY
    local root=$ROOT_DIR
    sed -i "s#ROOT_DIR#${ROOT_DIR}#g" $OVERLAY
}

function update_overlay_kvmtool {
    cd $SCRIPT_DIR
    cp -f ${OVERLAY_KVMTOOL_ORIG} ${OVERLAY_KVMTOOL}
    local root=$ROOT_DIR
    sed -i "s#ROOT_DIR#${ROOT_DIR}#g" ${OVERLAY_KVMTOOL}
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

function do_local_clean {
    make -f ./Makefile_local.mk clean
}

function do_clean {
    do_init

    cd $SCRIPT_DIR
    set -x
    $SHRINKWRAP_EXE clean cca-3world.yaml --overlay $OVERLAY
    rm -fr $HOME/.shrinkwrap
}

function do_compile {
    update_overlay

    set -x
    cd $SCRIPT_DIR
    $SHRINKWRAP_EXE build cca-3world.yaml --overlay $OVERLAY
    set +x
}

function do_kvmtool {
    update_overlay_kvmtool

    set -x
    cd $SCRIPT_DIR
    $SHRINKWRAP_EXE build ${OVERLAY_KVMTOOL}
    cp $HOME/.shrinkwrap/package/kvmtool.yaml/lkvm $ASSETS_DIR/snapshots
    set +x
}

function do_run {
    local p9_folder=$ROOT_DIR
    local preload=$ASSETS_DIR/fvp/bin/libhook-libc-2.31.so

    local shrinkwrap=$HOME/.shrinkwrap/package/cca-3world
    local pre=$ASSETS_DIR/tfa

    # prebuild from assets
    local asset_pre=$ASSETS_DIR/snapshots
    local bl1=$asset_pre/bl1.bin
    local fip=$asset_pre/fip.bin
    local image=$asset_pre/Image
    # local rootfs=$ASSETS_DIR/busybox-buildroot-lkvm-rootfs.ext2

    # compiled with shrinkwrap
    # XXX: When using local make file, change these to make output
    local bl1=$shrinkwrap/bl1.bin
    local fip=$shrinkwrap/fip.bin

    local image_shrinkwrap=$shrinkwrap/Image
    local image_buildroot=$OUTPUT_LINUX_GUEST_DIR/images/Image
    #local image=$image_shrinkwrap

    local rootfs_buildroot=$OUTPUT_LINUX_GUEST_DIR/images/rootfs.ext4
    local rootfs_prebuild=$ASSETS_DIR/busybox-buildroot-lkvm-rootfs.ext2
    local rootfs=$rootfs_prebuild

    $SCRIPTS_DIR/run_fvp.sh $bl1 $fip $image $rootfs $p9_folder $preload
}

# "${@:2}"
case "$1" in
    clean)
        do_clean
        do_local_clean
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
    kvmtool)
        do_kvmtool
        ;;
    *)
        echo "unknown"
        exit 1
esac
