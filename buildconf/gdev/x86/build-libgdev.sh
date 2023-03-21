#!/bin/bash

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
PROJ_DIR=$SCRIPT_DIR/../../../src/gpu/gdev-guest
set -x

cd $PROJ_DIR/lib

rm -rf build
mkdir build
cd build
../configure --target=user
make -j$(nproc)
sudo make install

export LD_LIBRARY_PATH="/usr/local/gdev/lib64:$LD_LIBRARY_PATH"
export PATH="/usr/local/gdev/bin:$PATH"

