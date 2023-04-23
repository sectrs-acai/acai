#!/bin/sh

here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
root=$here/../../
gpu_drv=$root/src/gpu_driver/gdev-guest/mod

set -x

cd $gpu_drv
./load_driver.sh
