#!/usr/bin/env bash

ROOT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
EXT_DIR=$ROOT_DIR/ext
EXT_BUILDROOT_DIR=$EXT_DIR/buildroot

SRC_DIR=$ROOT_DIR/src

OUTPUT_DIR=$ROOT_DIR/output

BUILDCONF_DIR=$ROOT_DIR/buildconf

ASSETS_DIR=$ROOT_DIR/assets

SCRIPTS_DIR=$ROOT_DIR/scripts

export PATH="$PATH:$ASSETS_DIR/toolchain-arm/aarch64-none-linux-gnu/bin"
export PATH="$PATH:$ASSETS_DIR/fvp/bin"
