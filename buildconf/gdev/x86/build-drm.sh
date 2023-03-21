#!/bin/bash
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
PROJ_DIR=$SCRIPT_DIR/../../../src/gpu/libdrm-guest

ensure_binary() {
    local cmd="$1"
    if ! command -v "$cmd" &> /dev/null
    then
        echo "$cmd could not be found"
        exit 1
    fi
}
ensure_binary meson
ensure_binary ninja

cd $PROJ_DIR
rm -rf build
meson build
ninja -C build

# ninja -C install
