#!/usr/bin/env bash

CUR_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source $(git rev-parse --show-toplevel)/env.sh

set -x

cd $EXT_PTEDIT_DIR/module/ && make
sudo insmod $EXT_PTEDIT_DIR/module/pteditor.ko

cd $CUR_DIR/../xdma/linux-kernel/xdma && make clean && make
cd $CUR_DIR/../xdma/linux-kernel/tests/
sudo ./load_driver.sh 4


cd $CUR_DIR
sudo ./run_fpga_usr_manager.sh
