#!/usr/bin/env bash
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source $(git rev-parse --show-toplevel)/env.sh

remote_home=/home/armcca/trusted-peripherals
target_dir=$remote_home/tmp-sync
host=armcca@192.33.93.245

function sync {
    rsync -v -a --checksum --ignore-times $1 $host:$target_dir
}

set -x

cd $ROOT_DIR
ssh $host mkdir -p $target_dir
sync $ASSETS_DIR/tfa/
sync $OUTPUT_LINUX_GUEST_DIR/images/

bl1=$target_dir/bl1-fvp
fip=$target_dir/fip-fvp
#bl1=$target_dir/mem-prealloc/bl1.bin
#fip=$target_dir/mem-prealloc/fip.bin

image=$target_dir/Image
rootfs=$target_dir/rootfs.ext2
p9_folder=$remote_home
preload=$remote_home/assets/fvp/libhook.libc-2.31.so

ssh -X $host $remote_home/scripts/run_fvp.sh $bl1 $fip $image $rootfs $p9_folder $preload
