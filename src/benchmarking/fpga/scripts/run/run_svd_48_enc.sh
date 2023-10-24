#!/usr/bin/env bash
HERE_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
set -euo pipefail

STATUS_FILE=$HERE_DIR/status
echo 1 > $STATUS_FILE

VIVADO_HOME=/opt/Xilinx/Vivado/2020.2
BATCH_FILE=$HERE_DIR/vivado_svd_48_enc.txt
HOT_RELOAD=$HERE_DIR/../hot_reset.sh
LSPCI=17:00.0
RELOAD_DRIVER_DIR=/home/armcca/trusted-peripherals/src/fpga_driver/xdma/linux-kernel/tests/
ITERATIONS=20

set -euo pipefail
set -x

function flash() {
    source $VIVADO_HOME/settings64.sh
    cd $VIVADO_HOME
    ./bin/vivado -mode batch -source $BATCH_FILE

    sudo $HOT_RELOAD $LSPCI

    cd $RELOAD_DRIVER_DIR
    sudo ./load_driver.sh
}

function loop() {
    i=0
    while [ $i -lt $ITERATIONS ]
    do
        echo "iteration $i"
        flash
        echo 0 > $STATUS_FILE
        wait=1
        while [ $wait -eq 1 ]
        do
            sleep 10
            status=$(cat $STATUS_FILE)
            echo "status"
            if [[ "$status" == "1" ]]; then
                wait=0
            fi
        done
        i=$((i+1))
    done
}



# flash
loop
