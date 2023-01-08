#!/usr/bin/env bash
source $(git rev-parse --show-toplevel)/env.sh

export ARCH=arm64
export PATH="$PATH:$OUTPUT_DIR/buildroot-linux-guest2/host/bin"
export CROSS_COMPILE=aarch64-linux-
