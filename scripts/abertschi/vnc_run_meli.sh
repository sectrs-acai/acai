#!/usr/bin/env bash
HERE_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
set -euo pipefail

source $(git rev-parse --show-toplevel)/env.sh

remote_home=/home/armcca/trusted-peripherals
target_dir=$remote_home/tmp-sync

preload=$ASSETS_DIR/fvp/bin/libhook-libc-2.31.so

$HERE_DIR/cpu_speed.sh

dir=$target_dir/snapshots/
bl1=$dir/bl1.bin
fip=$dir/fip.bin
image=$dir/Image-cca
rootfs=$dir/rootfs-ns.ext2
p9_folder=$ROOT_DIR

# allow root to access X11
xhost local:$USER

$SCRIPTS_DIR/run_fvp.sh \
    --bl1=$bl1 \
    --fip=$fip \
    --kernel=$image \
    --rootfs=$rootfs \
    --p9=$p9_folder \
    --hook=$preload \
    --benchmark
