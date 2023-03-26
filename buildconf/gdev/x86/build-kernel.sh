#!/bin/bash
set -euo pipefail

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
PROJ_DIR=$SCRIPT_DIR/../../../src/linux-host
set -x

cd $PROJ_DIR
sudo make bzImage -j$(nproc)
sudo make modules -j$(nproc)
sudo make modules_install
sudo make install
