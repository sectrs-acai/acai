#!/bin/sh

root=../assets/snapshots

core=0x4

# script to run a realm VM:
#
# change memory (in MB) of realm with -m flag
#

echo "Booting a realm..."
echo "Command:"

set -x

nice -n -20 taskset $core ./$root/lkvm run --realm  --disable-sve -c 1 -m 800 \
    -k $root/Image-cca -i $root/rootfs.realm.cpio --9p "/,host0" -p "fvp_escape_loop fvp_escape_off ip=off"
