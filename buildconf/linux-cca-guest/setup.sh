#!/bin/bash
set -euo pipefail
source $(git rev-parse --show-toplevel)/env.sh

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

BUILDROOT_CONFIG_DIR=$SCRIPT_DIR/buildroot
BUILDROOT_OUTPUT_DIR=$OUTPUT_LINUX_CCA_GUEST_DIR
BUILDROOT_DIR=$EXT_BUILDROOT_DIR

SRC_LINUX=$SRC_DIR/linux-cca-guest

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
    echo "building root image with linux kernel"
    set -x
    env -u LD_LIBRARY_PATH \
        time make BR2_JLEVEL=$BR2_JLEVEL O=$BUILDROOT_OUTPUT_DIR \
        gdev_guest-rebuild all

    # linux-rebuild

    ls -al $BUILDROOT_OUTPUT_DIR/images/
    cp -rf $BUILDROOT_OUTPUT_DIR/images/Image $ASSETS_DIR/snapshots/Image-cca
    cp -rf $BUILDROOT_OUTPUT_DIR/images/rootfs.ext2 $ASSETS_DIR/snapshots/rootfs-ns.ext2
}

function do_linux {
    cd $BUILDROOT_DIR
    echo "building root image with linux kernel"
    set -x
    env -u LD_LIBRARY_PATH \
        time make BR2_JLEVEL=$BR2_JLEVEL O=$BUILDROOT_OUTPUT_DIR \
        linux-rebuild all

    ls -al $BUILDROOT_OUTPUT_DIR/images/
    cp -rf $BUILDROOT_OUTPUT_DIR/images/Image $ASSETS_DIR/snapshots/Image-cca
    cp -rf $BUILDROOT_OUTPUT_DIR/images/rootfs.ext2 $ASSETS_DIR/snapshots/rootfs-ns.ext2
}

function do_libdrm {
    cd $BUILDROOT_DIR
    echo "=== building libdrm only"
    set -x
    env -u LD_LIBRARY_PATH \
        time make BR2_JLEVEL=$BR2_JLEVEL O=$BUILDROOT_OUTPUT_DIR \
        libdrm-rebuild


    ls -al $BUILDROOT_OUTPUT_DIR/build/libdrm-custom/build
    cp -rf $BUILDROOT_OUTPUT_DIR/build/libdrm-custom/build/lib*.so* $ASSETS_DIR/snapshots/lib64/
    ls -al $ASSETS_DIR/snapshots/lib64/
}

# XXX: this does not work with qemu without cca patches
function do_run {
    cd $BUILDROOT_OUTPUT_DIR
    exec qemu-system-aarch64 \
        -s -M virt -m 2G -cpu cortex-a53 -nographic -smp 2 \
        -kernel ./images/Image -append "rootwait root=/dev/vda console=ttyAMA0 nokaslr" \
        -netdev user,id=eth0 -device virtio-net-device,netdev=eth0 \
        -drive file=./images/rootfs.ext4,if=none,format=raw,id=hd0 -device virtio-blk-device,drive=hd0 \
        -virtfs local,path=$SHARE_DIR,mount_tag=host0,security_model=none,id=host0
    #-s  \
}

function do_run_fvp {
    cd $BUILDROOT_OUTPUT_DIR
    local preload=$ASSETS_DIR/fvp/bin/libhook-libc-2.31.so

    # XXX: For shrinkwrap output when compiling locally
    # local shrinkwrap=$HOME/.shrinkwrap/package/cca-3world
    # local bl1=$shrinkwrap/bl1.bin
    # local fip=$shrinkwrap/fip.bin

    # XXX: For tfa prebuilt off assets
    local bl1=$ASSETS_DIR/snapshots/bl1.bin
    local fip=$ASSETS_DIR/snapshots/fip.bin

    local image=./images/Image
    local rootfs=./images/rootfs.ext4
    local p9_folder=$ROOT_DIR

    $SCRIPTS_DIR/run_fvp.sh \
        --bl1=$bl1 \
        --fip=$fip \
        --kernel=$image \
        --rootfs=$rootfs \
        --p9=$p9_folder \
        --hook=$preload
}

function do_compilationdb {
    local buildroot_kernel=$BUILDROOT_OUTPUT_DIR/build/linux-custom
    local src_kernel=$SRC_LINUX
    local out=$src_kernel/compile_commands.json

    cd $BUILDROOT_OUTPUT_DIR/build/linux-custom/
    ./scripts/clang-tools/gen_compile_commands.py -o $out
    cd $src_kernel
    sed -i "s#$buildroot_kernel#$src_kernel#g" compile_commands.json
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
    libdrm)
        do_libdrm
        ;;
    linux)
        do_linux
        ;;
    build)
        do_compile
        do_run_fvp
        ;;
    run)
        do_run
        ;;
    run_fvp)
        do_run_fvp
        ;;
    compilationdb)
        do_compilationdb
        ;;
    *)
      echo "unknown"
      exit 1
       ;;
esac
