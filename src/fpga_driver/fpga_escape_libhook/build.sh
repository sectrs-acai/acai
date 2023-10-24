#!/bin/bash
set -euo pipefail
source $(git rev-parse --show-toplevel)/env.sh

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
PROJ_ROOT=$SCRIPT_DIR/../../../

function do_init {
    echo "do init..."
}

function do_clean {
   echo "do clean..."
}

function do_build {
    cd $PROJ_ROOT/scripts
    source env-x86.sh
    
    # build helpers
    cd $PROJ_ROOT/src/fpga_driver/fh_host
    make

    # build libhook
    cd $PROJ_ROOT/src/fpga_driver/libhook
    make

    # build userspace manager
    cd $PROJ_ROOT/src/fpga_driver/fpga_escape_libhook
    make
}

# "${@:2}"
case "$1" in
    clean)
        do_clean
        ;;
    init)
        do_init
        ;;
    build)
        do_build
        ;;
    *)
      echo "unknown command given"
      exit 1
       ;;
esac
