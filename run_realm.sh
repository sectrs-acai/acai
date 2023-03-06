#!/bin/sh

root=./tmp-sync/tfa-unmod-realm-ready

set -x
nice -20 ./$root/lkvm run --realm --disable-sve -c 1 -m 256 -k $root/../Image -i assets/rootfs-realm.cpio --debug --9p "/,host0" -p "fvp_escape_loop"
