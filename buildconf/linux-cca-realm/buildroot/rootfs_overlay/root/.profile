#!/bin/sh

set -x
mount -t 9p -o trans=virtio,version=9p2000.L host0 /mnt
set +x

# load shared libraries for develoment
# export LD_LIBRARY_PATH="/mnt/mnt/host/assets/snapshots/aarch64-lib:$LD_LIBRARY_PATH"

export LD_LIBRARY_PATH="/usr/local/gdev/lib64:$LD_LIBRARY_PATH"
