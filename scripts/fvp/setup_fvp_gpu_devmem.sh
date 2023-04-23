#!/bin/sh

here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
root=$here/../../
gpu_drv=$root/src/gpu_driver/gdev-guest/mod
devmem=$root/src/fpga_driver/devmem_intercept

set -x

cd $gpu_drv
./load_driver.sh

cd $devmem
./run_module.sh
