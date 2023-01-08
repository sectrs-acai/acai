#!/bin/bash
set -euo pipefail
source $(git rev-parse --show-toplevel)/env.sh

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

BUILDROOT_CONFIG_DIR=$SCRIPT_DIR/buildroot
BUILDROOT_OUTPUT_DIR=$OUTPUT_DIR/buildroot-linux-host
BUILDROOT_DIR=$EXT_BUILDROOT_DIR
SRC_LINUX_HOST=$SRC_DIR/linux-host

BR2_EXTERNAL=../buildroot_packages
BR2_JLEVEL=$(nproc)
SHARE_DIR=$ROOT_DIR


function do_init {
    cd $BUILDROOT_DIR
    rm -f .config
    rm -f $BUILDROOT_OUTPUT_DIR/.config

    make V=1 BR2_EXTERNAL=$BR2_EXTERNAL qemu_x86_64_defconfig O=$BUILDROOT_OUTPUT_DIR
    cat $BUILDROOT_CONFIG_DIR/buildroot_config_fragment_aarch64 >> $BUILDROOT_OUTPUT_DIR/.config
    make BR2_EXTERNAL=$BR2_EXTERNAL olddefconfig O=$BUILDROOT_OUTPUT_DIR
}

function do_clean {
    cd $BUILDROOT_DIR
    make clean O=$BUILDROOT_OUTPUT_DIR
    do_init
}

function do_run {
    cd $BUILDROOT_DIR

    qemu-system-x86_64 \
    -smp $(nproc --all) \
    -M pc \
    -s  \
    -kernel ./output/images/bzImage \
    -drive file=./output/images/rootfs.ext2,if=virtio,format=raw \
    -virtfs local,path=$SHARE_DIR,mount_tag=host0,security_model=none,id=host0 \
    -append "root=/dev/vda console=ttyS0 nokaslr" \
    -net nic,model=virtio -net user \
    -nographic
}

function do_run_debian {
    cd $BUILDROOT_DIR
    $SCRIPT_DIR/../debian-rootfs/run.sh ./output/images/bzImage $SHARE_DIR
}

function do_compilationdb {
    local buildroot_kernel=$BUILDROOT_DIR/output/build/linux-custom
    local src_kernel=$SRC_LINUX_HOST
    local out=$src_kernel/compile_commands.json
    cd $BUILDROOT_DIR/output/build/linux-custom/
    ./scripts/clang-tools/gen_compile_commands.py -o $out
    cd $src_kernel
    sed -i "s#$buildroot_kernel#$src_kernel#g" compile_commands.json
}

case "$1" in
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

    build_debian)
        do_compile
        do_run_debian
        ;;
    run)
        do_run
        ;;
    run_debian)
        do_run_debian
        ;;
    compilationdb)
        do_compilationdb
        ;;
    *)
        echo "unknown"
        exit 1
esac
