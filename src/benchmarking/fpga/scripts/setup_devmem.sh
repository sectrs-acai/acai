#!/bin/zsh
#
set -euo pipefail
BENCH_OUT_DIR=/mnt/host/mnt/host/benchmark-fpga
HERE_DIR=${0:a:h}
BENCH_DIR=$HERE_DIR
RUNS=1
mkdir -p $BENCH_OUT_DIR

PIN_CORE=0x4
DO_PIN_CORE=0

DEVMEM_UNDELGATE=$HERE_DIR/../../scripts/fvp/reset_devmem.sh


cp $HERE_DIR/bench/bench.so /mnt/host/mnt/host/ || true
cp $HERE_DIR/bench/bench.so /mnt/host || true

function python_run {
    local BENCH_SCRIPT=$1

    TS=$(date +"%Y-%m-%d_%H-%M-%S")
    LOG=$BENCH_OUT_DIR/${BENCH_SCRIPT}_${TS}.txt
    set +x

    for i in {1..$RUNS}; do
        echo "executing $BENCH_SCRIPT" | tee -a $LOG
        echo "log file: $LOG" | tee -a $LOG

        cd $BENCH_DIR
        cat ./$BENCH_SCRIPT >> $LOG
        echo >> $LOG

        if [[ $DO_PIN_CORE -eq 1 ]]
        then
           set -x
           exec nice -20 taskset $PIN_CORE python ./$BENCH_SCRIPT 2>&1 | tee -a $LOG
           sh $DEVMEM_UNDELGATE
           set +x
        else
           set -x
           exec nice -20 python ./$BENCH_SCRIPT 2>&1 | tee -a $LOG
           sh $DEVMEM_UNDELGATE
           set +x
        fi
    done

}

function python_run_param {
    local BENCH_SCRIPT=$1
    local PARAM=$2

    TS=$(date +"%Y-%m-%d_%H-%M-%S")
    LOG=$BENCH_OUT_DIR/${BENCH_SCRIPT}_${TS}.txt
    set +x

    for i in {1..$RUNS}; do
        echo "executing $BENCH_SCRIPT" | tee -a $LOG
        echo "log file: $LOG" | tee -a $LOG

        cd $BENCH_DIR
        cat ./$BENCH_SCRIPT >> $LOG
        echo >> $LOG

        if [[ $DO_PIN_CORE -eq 1 ]]
        then
           set -x
           exec nice -20 taskset $PIN_CORE python ./$BENCH_SCRIPT $PARAM 2>&1 | tee -a $LOG
           #sh $DEVMEM_UNDELGATE
           set +x
        else
           set -x
           exec nice -20 python ./$BENCH_SCRIPT $PARAM 2>&1 | tee -a $LOG
           #sh $DEVMEM_UNDELGATE
           set +x
        fi
    done

}


STATUS_FILE=$HERE_DIR/run/status
function python_run_loop_param {
    local cmd=$1
    local arg=$2

    i=0
    while [ 1 ]
    do
        echo "iteration $i"
        python_run_param $cmd $arg
        echo 1 > $STATUS_FILE
        sh $DEVMEM_UNDELGATE
        wait=1
        while [ $wait -eq 1 ]
        do
            sleep 4
            stat=$(cat $STATUS_FILE)
            echo "waiting $stat"
            if [[ "$stat" == "0" ]]; then
                wait=0
            fi
        done
        i=$((i+1))
    done

}


if (( ${+2} )); then
case "$2" in
    --ns)
        echo "pinning core to $PIN_CORE"
        DO_PIN_CORE=1
        ;;
    *)
esac
fi

case "$1" in
    matmul)
        python_run "matmul.py"
        ;;
    matmul_enc)
        python_run "matmul_enc.py"
        ;;
    xdma)
        python_run "xdma.py"
        ;;
    xdma_enc)
        python_run "xdma_enc.py"
        ;;
    svd_32)
        python_run_loop_param "svd.py" 32
        # python_run_param "svd.py" 32
        ;;
    svd_32_enc)
        python_run_param "svd_enc.py" 32
        ;;
    svd_48)
        python_run_loop_param "svd.py" 48
        # python_run_param "svd.py" 48
        ;;
    svd_48_enc)
        python_run_param "svd_enc.py" 48
        ;;
    *)
        echo "unknown"
        exit 1
esac
