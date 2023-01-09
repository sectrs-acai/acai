#!/usr/bin/env bash
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source $(git rev-parse --show-toplevel)/env.sh

remote_home=/home/bean/trusted-periph
target_dir=$remote_home/tmp-sync
host=armcca@192.33.93.245

ssh $host mkdir -p $target_dir
scp -Rv $ASSETS_DIR/tfa $host:$target_dir
scp -Rv $OUTPUT_LINUX_GUEST_DIR/images/ $host:$target_dir

bl1=$target_dir/bl1-fvp
fip=$target_dir/fip-fvp
image=$target_dir/images/Image
rootfs=$target_dir/images/rootfs.ext2
p9_folder=$remote_home


ssh -X $host $remote_home/scripts/run_fvp.sh $bl1 $fip $image $rootfs $p9_folder
