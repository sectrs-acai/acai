#!/usr/bin/env bash
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
set -euo pipefail

source $(git rev-parse --show-toplevel)/env.sh

remote_home=/home/armcca/trusted-peripherals
target_dir=$remote_home/tmp-sync
host=armcca@192.33.93.245

function sync {
    rsync -r -v -a $1 $host:$target_dir
}

set -x

cd $ROOT_DIR
ssh $host mkdir -p $target_dir
sync $ASSETS_DIR/snapshots
ssh -L 59000:localhost:5901 -C -N -l armcca 192.33.93.245
