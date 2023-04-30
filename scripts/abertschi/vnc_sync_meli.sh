#!/usr/bin/env bash
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
set -euo pipefail

source $(git rev-parse --show-toplevel)/env.sh

remote_home=/home/armcca/trusted-peripherals
target_dir=$remote_home/tmp-sync
host=armcca@192.33.93.245

function sync {
    rsync -r -v -a --checksum --ignore-times $1 $host:$target_dir
}

set -x

cd $ROOT_DIR
ssh $host mkdir -p $target_dir
#sync $ASSETS_DIR/tfa/
#sync $OUTPUT_LINUX_CCA_GUEST_DIR/images/
sync $ASSETS_DIR/snapshots

# shrinkwrap output
# shrinkwrap=/home/armcca/.shrinkwrap/package/cca-3world
# bl1=$shrinkwrap/bl1.bin
# fip=$shrinkwrap/fip.bin

# assets output
bl1=$target_dir/snapshots/bl1.bin
fip=$target_dir/snapshots/fip.bin
#
#bl1=$target_dir/tfa-unmod-realm-ready/2gb-memcap-unmod-bl1.bin
#fip=$target_dir/tfa-unmod-realm-ready/2gb-memcap-unmod-fip.bin

image=$target_dir/Image
rootfs=$target_dir/rootfs.ext2
p9_folder=$remote_home

ssh -L 59000:localhost:5901 -C -N -l armcca 192.33.93.245
