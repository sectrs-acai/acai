#!/bin/bash
set -euo pipefail
source $(git rev-parse --show-toplevel)/env.sh

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

BUILDROOT_CONFIG_DIR=$SCRIPT_DIR/buildroot
BUILDROOT_OUTPUT_DIR=$OUTPUT_LINUX_CCA_REALM_DIR
BUILDROOT_DIR=$EXT_BUILDROOT_DIR
BR2_EXTERNAL=$SCRIPT_DIR/../buildroot_packages
BR2_JLEVEL=$(nproc)

SHARE_DIR=$ROOT_DIR

function do_init {
    set -x
    cd $BUILDROOT_DIR

    rm -f .config
    rm -f .config.old
    rm -f $BUILDROOT_OUTPUT_DIR/.config
    rm -f $BUILDROOT_OUTPUT_DIR/.config.old

    make V=1 BR2_EXTERNAL=$BR2_EXTERNAL O=$BUILDROOT_OUTPUT_DIR  qemu_aarch64_virt_defconfig
    cat $BUILDROOT_CONFIG_DIR/buildroot_config_fragment_aarch64 >> $BUILDROOT_OUTPUT_DIR/.config
    make V=1 BR2_EXTERNAL=$BR2_EXTERNAL O=$BUILDROOT_OUTPUT_DIR olddefconfig

    ./utils/diffconfig $BUILDROOT_OUTPUT_DIR/.config.old $BUILDROOT_OUTPUT_DIR/.config
    ./utils/diffconfig -m $BUILDROOT_OUTPUT_DIR/.config.old $BUILDROOT_OUTPUT_DIR/.config
}

function do_clean {
    cd $BUILDROOT_DIR
    make V=1 O=$BUILDROOT_OUTPUT_DIR clean
    do_init
}

function do_compile {
    cd $BUILDROOT_DIR
    set -x
    env -u LD_LIBRARY_PATH \
        time make BR2_JLEVEL=$BR2_JLEVEL O=$BUILDROOT_OUTPUT_DIR  all
    ls -al $BUILDROOT_OUTPUT_DIR/images

    cp -rf $BUILDROOT_OUTPUT_DIR/images/rootfs.cpio $ASSETS_DIR/snapshots/rootfs.realm.cpio
}

# "${@:2}"
case "$1" in
    xconfig)
        cd $BUILDROOT_DIR
        set -x
        make xconfig O=$BUILDROOT_OUTPUT_DIR
        ./utils/diffconfig $BUILDROOT_OUTPUT_DIR/.config.old $BUILDROOT_OUTPUT_DIR/.config
        ./utils/diffconfig -m $BUILDROOT_OUTPUT_DIR/.config.old $BUILDROOT_OUTPUT_DIR/.config
        ;;
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
        ;;
    *)
        echo "unknown"
        exit 1
esac
