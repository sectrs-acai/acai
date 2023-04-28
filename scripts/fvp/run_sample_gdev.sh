#!/bin/sh

here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
root=$here/../../
gpu_drv=$root/src/gpu_driver/gdev-guest/mod

set -x

# load a dummy test to trigger the
# kernel bug once message which always appears
# upon first gdev launch
#
cd $gpu_drv/../../rodinia-bench/test-gdev/cuda/user/madd
./user_test
