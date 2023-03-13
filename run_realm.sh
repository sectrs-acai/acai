#!/bin/sh

root=./tmp-sync/tfa-unmod-realm-ready

set -x
# --vfio-pci "0000:00:00.0"
nice -20 ./$root/lkvm run --realm  --disable-sve -c 1 -m 256 -k $root/../snapshots/Image-cca -i assets/rootfs-realm.cpio --debug --9p "/,host0" -p "fvp_escape_loop fvp_escape_on ip=off" --dump-dtb /tmp/lkvm.dtb
