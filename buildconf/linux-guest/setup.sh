#!/bin/bash
set -euo pipefail
source $(git rev-parse --show-toplevel)/env.sh

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

BUILDROOT_CONFIG_DIR=$SCRIPT_DIR/buildroot
BUILDROOT_OUTPUT_DIR=$OUTPUT_DIR/buildroot-linux-guest2
BUILDROOT_DIR=$EXT_BUILDROOT_DIR

BR2_EXTERNAL=$SCRIPT_DIR/
BR2_JLEVEL=$(nproc)
SHARE_DIR="$SCRIPT_DIR/.."


function do_init {
    cd $BUILDROOT_DIR
    rm -f .config
    rm -f $BUILDROOT_OUTPUT_DIR/.config

    make V=1 qemu_aarch64_virt_defconfig O=$BUILDROOT_OUTPUT_DIR
    cat $BUILDROOT_CONFIG_DIR/buildroot_config_fragment_aarch64 >> $BUILDROOT_OUTPUT_DIR/.config
    make olddefconfig O=$BUILDROOT_OUTPUT_DIR
}

function do_clean {
    cd $BUILDROOT_DIR
    make clean O=$BUILDROOT_OUTPUT_DIR
    do_init
}

function do_compile {
    cd $BUILDROOT_DIR
    env -u LD_LIBRARY_PATH \
        time make BR2_JLEVEL=$BR2_JLEVEL O=$BUILDROOT_OUTPUT_DIR \
        linux-rebuild all
}

function do_run {
    cd $BUILDROOT_OUTPUT_DIR
    source $SCRIPTS_DIR/env-aarch64.sh
    exec qemu-system-aarch64 \
        -M virt -cpu cortex-a53 -nographic -smp 1 \
        -kernel ./images/Image -append "rootwait root=/dev/vda console=ttyAMA0 nokaslr" -netdev user,id=eth0 -device virtio-net-device,netdev=eth0 \
        -drive file=./images/rootfs.ext4,if=none,format=raw,id=hd0 -device virtio-blk-device,drive=hd0
    -virtfs local,path=$SHARE_DIR,mount_tag=host0,security_model=none,id=host0
    #-s  \
        }

function do_run_fvp {
    cd $BUILDROOT_OUTPUT_DIR

    local pre=$ASSETS_DIR/tfa
    local bl1=$pre/bl1-fvp
    local fip=$pre/fip-fvp
	local image=./images/Image
	local rootfs=./images/rootfs.ext4
	local p9_folder=$ROOT_DIR

    $SCRIPTS_DIR/run_fvp.sh $bl1 $fip $image $rootfs $p9_folder


}

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
    run_fvp)
        do_run_fvp
        ;;
    *)
        echo "unknown"
        exit 1
esac
