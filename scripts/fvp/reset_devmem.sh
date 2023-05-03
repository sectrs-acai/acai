#!/bin/sh

here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
root=$here/../../
dir=$root/src/fpga_driver/devmem_intercept

set -x

cd $dir
rmmod ./devmem_undelegate.ko
insmod ./devmem_undelegate.ko
rmmod ./devmem_undelegate.ko
