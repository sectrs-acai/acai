#!/usr/bin/env bash
set -euo pipefail
set -x
CUR_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source $CUR_DIR/../../env.sh

size=60G
dir=$RAMDISK_DIR
sudo mkdir -p $dir
set -x

echo "creating ramdisk $dir"
sudo mount -t tmpfs -o size=$size tmpfs $dir
ls -al $dir
