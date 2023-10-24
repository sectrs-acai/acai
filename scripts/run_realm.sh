#!/bin/sh

root=../assets/snapshots

set -x

# useful flags:
# set fvp_escape_off to disable fvp escape tagging
# --dump-dtb /tmp/lkvm.dtb
# --debug


# XXX: core 3 is reserved for tracing
core=0x4

nice -n -20 taskset $core ./$root/lkvm run --realm  --disable-sve -c 1 -m 1000 \
    -k $root/Image-cca -i $root/rootfs.realm.cpio --9p "/,host0" -p "fvp_escape_loop fvp_escape_on ip=off"
