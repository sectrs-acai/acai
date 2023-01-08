#!/usr/bin/env bash

#
# run given kernel in debian rootfs
# usage: $0 <kernel> <shared-dir> <option qemu options>
#

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
gen_rootfs=$SCRIPT_DIR/create-debian-image.sh

rootfs=$SCRIPT_DIR/bullseye.img
kernel=""
share=""

if [ ! -f "$rootfs" ]; then
    echo "$rootfs does not exist."
    [ -f $gen_rootfs ] && $gen_rootfs

fi

if ! command -v qemu-system-x86_64  &> /dev/null
then
    echo "needs qemu-system-x86_64 installed"
    exit
fi

function help {
        echo $0 "<path to kernel> <path to shared dir> <option qemu args>"
}

if [ -z "$1" ]; then
    help
    exit 1
fi
kernel="$1"

if [ -z "$2" ]; then
    help
    exit 1
fi
shared="$2"

set -x
qemu-system-x86_64 \
        -m 8G \
        -smp $(nproc) \
        -s \
        -kernel $kernel \
        -append "console=ttyS0 root=/dev/sda earlyprintk=serial net.ifnames=0 nokaslr" \
        -drive file=$rootfs,format=raw \
        -net user,host=10.0.2.10,hostfwd=tcp:127.0.0.1:10021-:22 \
        -net nic,model=e1000 \
        -enable-kvm \
        -nographic \
        -virtfs local,path=$shared,mount_tag=host0,security_model=none,id=host0 \
        -pidfile vm.pid ${@:3} \
        2>&1 | tee vm.log
