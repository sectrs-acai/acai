#!/bin/bash
set -euo pipefail
source $(git rev-parse --show-toplevel)/env.sh
SCRIPT_DIR=$( dirname "$(readlink -f "$0")" )

BUILDROOT_CONFIG_DIR=$SCRIPT_DIR/buildroot
BUILDROOT_OUTPUT_DIR=$OUTPUT_LINUX_HOST_DIR
BUILDROOT_DIR=$EXT_BUILDROOT_DIR

SRC_LINUX=$SRC_DIR/linux-host

BR2_EXTERNAL=$SCRIPT_DIR/../buildroot_packages
BR2_JLEVEL=$(nproc)
SHARE_DIR=$ROOT_DIR

function do_init {
    cd $BUILDROOT_DIR
    rm -f .config
    rm -f $BUILDROOT_OUTPUT_DIR/.config

    make V=1 BR2_EXTERNAL=$BR2_EXTERNAL O=$BUILDROOT_OUTPUT_DIR qemu_x86_64_defconfig
    cat $BUILDROOT_CONFIG_DIR/buildroot_config_fragment >> $BUILDROOT_OUTPUT_DIR/.config
    make V=1 BR2_EXTERNAL=$BR2_EXTERNAL O=$BUILDROOT_OUTPUT_DIR olddefconfig
}

function do_clean {
    cd $BUILDROOT_DIR
    make clean O=$BUILDROOT_OUTPUT_DIR
    do_init
}

function do_compile {
    cd $BUILDROOT_DIR
    env -u LD_LIBRARY_PATH \
        time make BR2_EXTERNAL=$BR2_EXTERNAL BR2_JLEVEL=$BR2_JLEVEL  O=$BUILDROOT_OUTPUT_DIR \
        fvp_escape_host-rebuild all
}


function do_run {
    # XXX: use host qemu, we get some error otherwise
    # # cd $SCRIPT_DIR && source $SCRIPTS_DIR/env-x86.sh

    cd $BUILDROOT_OUTPUT_DIR
    qemu-system-x86_64 \
        -smp $BR2_JLEVEL \
        -M pc \
        -enable-kvm \
        -s \
        -kernel ./images/bzImage \
        -drive file=./images/rootfs.ext2,if=virtio,format=raw \
        -virtfs local,path=$SHARE_DIR,mount_tag=host0,security_model=none,id=host0 \
        -append "root=/dev/vda console=ttyS0 nokaslr" \
        -net nic,model=virtio -net user \
        -nographic
}

function do_run_debian {
    cd $SCRIPT_DIR && source $SCRIPTS_DIR/env-x86.sh
    cd $BUILDROOT_OUTPUT_DIR

    $SCRIPT_DIR/../debian-rootfs/run.sh ./images/bzImage $SHARE_DIR
}

function do_compilationdb {
    # TODO: Fix this
    echo "fix this"
    local buildroot_kernel=$BUILDROOT_OUTPUT_DIR/build/linux-custom
    local src_kernel=$SRC_LINUX
    local out=$src_kernel/compile_commands.json

    cd $BUILDROOT_OUTPUT_DIR/output/build/linux-custom/
    ./scripts/clang-tools/gen_compile_commands.py -o $out
    cd $src_kernel
    sed -i "s#$buildroot_kernel#$src_kernel#g" compile_commands.json
}

case "$1" in
    clean)
        do_clean
        do_init
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
