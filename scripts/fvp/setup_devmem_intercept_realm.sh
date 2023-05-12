#!/bin/sh

here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
root=$here/../../
fpga_drv=$root/src/fpga_driver/xdma_tests
devmem=$root/src/fpga_driver/devmem_intercept

set -x

cd $devmem
rmmod devmem_intercept.ko
insmod ./devmem_intercept.ko realm=1 kprobe=1
