#!/bin/sh

here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
root=$here/../../
fpga_drv=$root/src/fpga_driver/xdma_tests

set -x

cd $fpga_drv
./test_guest_load_driver.sh
