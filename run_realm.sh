#!/bin/sh

root=assets/snapshots

set -x
# set fvp_escape_off to disable fvp escape tagging
# --dump-dtb /tmp/lkvm.dtb
# --debug

nice -20 ./$root/lkvm run --realm  --disable-sve -c 8 -m 1000 \
    -k $root/Image-cca -i $root/rootfs.realm.cpio --9p "/,host0" -p "fvp_escape_loop fvp_escape_on ip=off"
