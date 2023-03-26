#!/bin/bash
set -euo pipefail
set -x
CUR_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source $CUR_DIR/../../env.sh

PROJ_DIR=$CUR_DIR/../../src/gpu/gdev-guest
export LINUX_DIR=$OUTPUT_LINUX_CCA_GUEST_HEADERS
export LINUXDIR=$OUTPUT_LINUX_CCA_GUEST_HEADERS

TARGET=$CUR_DIR/build-gdev-kernel/
cd $PROJ_DIR
rm -rf $TARGET
rm -rf release

mkdir -p $CUR_DIR/install
TOOLCHAIN_FILE=$OUTPUT_LINUX_CCA_GUEST_DIR/host/share/buildroot/toolchainfile.cmake

cmake -DCMAKE_TOOLCHAIN_FILE=$TOOLCHAIN_FILE -DCMAKE_INSTALL_PREFIX=$CUR_DIR/install  -H. -B$TARGET \
    -Ddriver=nouveau \
    -Duser=OFF \
    -Druntime=OFF \
    -Dusched=OFF \
    -Duse_as=OFF \
    -DCMAKE_BUILD_TYPE=Release

source $CUR_DIR/../../scripts/env-aarch64.sh
make -C $TARGET
# make -C $TARGET install

SYSROOT_FROM=$CUR_DIR/install/gdev
SYSROOT_TO=$OUTPUT_LINUX_CCA_GUEST_DIR/staging
cp -rf $SYSROOT_FROM/bin/* $SYSROOT_TO/bin/
cp -rf $SYSROOT_FROM/lib64/* $SYSROOT_TO/lib64/
cp -rf $SYSROOT_FROM/include/* $SYSROOT_TO/usr/include/
cp -rf $SYSROOT_FROM/lib64/* $ASSETS_DIR/snapshots/lib64/
