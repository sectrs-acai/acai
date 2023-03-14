#!/bin/sh

root=assets/snapshots

set -x
# set fvp_escape_off to disable fvp escape tagging
nice -20 ./$root/lkvm run --realm  --disable-sve -c 1 -m 256 -k $root/Image-cca -i $root/rootfs-realm.cpio --debug --9p "/,host0" -p "fvp_escape_loop fvp_escape_on ip=off" --dump-dtb /tmp/lkvm.dtb
