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
sync $ASSETS_DIR/tfa/
sync $OUTPUT_LINUX_GUEST_DIR/images/

sync /home/b/2.5bay/mthesis-unsync/projects/shrinkwrap-assets/package



bl1=$target_dir/23-02-14_bl1-unmod-tfa-unlimited-mem.bin
fip=$target_dir/23-02-14_fip-unmod-tfa-unlimited-mem.bin

#bl1=$target_dir/bl1.bin
#fip=$target_dir/fip.bin

image=$target_dir/package/cca-3world/Image
rootfs=$target_dir/rootfs.ext2
p9_folder=$remote_home

#
# Host has old libc version
#
preload=$remote_home/assets/fvp/bin/libhook-libc-2.31.so

# ssh $host sudo pkill FVP
ssh -X $host $remote_home/scripts/run_fvp.sh $bl1 $fip $image $rootfs $p9_folder $preload "ssh"
