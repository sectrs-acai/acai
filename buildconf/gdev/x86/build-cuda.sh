#!/bin/bash

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
PROJ_DIR=$SCRIPT_DIR/../../../src/gpu/gdev-guest
set -x

ls -al $PROJ_DIR
cd $PROJ_DIR/cuda
rm -rf build
mkdir build
cd build

../configure  --disable-runtime
make -j$(nproc)
sudo make install
