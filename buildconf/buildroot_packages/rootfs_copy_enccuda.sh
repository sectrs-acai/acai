#!/usr/bin/env bash

#
# copy static lib enc cuda assets from assets dir
#

set -x

BUILDROOT_DIR=$(pwd)

ROOT_DIR=$BUILDROOT_DIR/../../
ENC_CUDA=$ROOT_DIR/assets/encrypted-cuda/install

# share
cp -rf $ENC_CUDA/share/* $STAGING_DIR/usr/share/
cp -rf $ENC_CUDA/share/* $TARGET_DIR/usr/share/

#lib needs to be in /usr/lib not /lib
cp -rf $ENC_CUDA/lib/* $TARGET_DIR/usr/lib/
cp -rf $ENC_CUDA/lib/* $STAGING_DIR/usr/lib/

# include
mkdir -p $STAGING_DIR/usr/include
mkdir -p $TARGET_DIR/usr/include

cp -rf $ENC_CUDA/include/* $STAGING_DIR/usr/include/
cp -rf $ENC_CUDA/include/* $TARGET_DIR/usr/include/
