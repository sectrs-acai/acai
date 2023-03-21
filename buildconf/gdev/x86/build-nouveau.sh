#!/bin/bash

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
PROJ_DIR=$SCRIPT_DIR/../../../src/linux-host
set -x

cd $PROJ_DIR
sudo make modules -j$(nproc)
sudo make M=drivers/gpu/drm/nouveau -j$(nproc)
sudo make modules_install
sudo modprobe -r nouveau
sudo modprobe nouveau
