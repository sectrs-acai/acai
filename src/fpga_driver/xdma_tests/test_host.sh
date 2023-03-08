#!/usr/bin/env bash

base=../xdma/linux-kernel

cd $base/tests

./load_driver.sh 4
./run_test.sh
