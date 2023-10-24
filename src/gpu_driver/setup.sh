#!/bin/bash

#
# builds all major components for gpu device interactions
# - requires buildroot toolchain installed
#

set -euo pipefail
source $(git rev-parse --show-toplevel)/env.sh

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)
PROJ_ROOT=$SCRIPT_DIR/../..

set +x

function do_init {
  echo "do init..."
}

function do_clean {
  echo "do clean..."
}

function do_fh_host {
  cd $PROJ_ROOT/scripts
  source env-x86.sh

  # build helpers
  cd $PROJ_ROOT/src/fpga_driver/fh_host
  make
}

function do_intercept {
  cd $PROJ_ROOT/scripts
  source env-aarch64.sh

  cd $PROJ_ROOT/src/fpga_driver/devmem_intercept
  make
}

function do_fh_fixup {
  cd $PROJ_ROOT/scripts
  source env-x86.sh

  cd $PROJ_ROOT/src/gpu_driver/fh_fixup
  make clean
  make
}

function do_libhook {
  cd $PROJ_ROOT/scripts
  source env-x86.sh

  # build libhook
  cd $PROJ_ROOT/src/fpga_driver/libhook
  make clean
  make
}

function do_gpu_usr_manager {
  cd $PROJ_ROOT/scripts
  source env-x86.sh

  # build userspace manager
  cd $PROJ_ROOT/src/gpu_driver/gpu_gdev_usr_manager
  make clean
  make
}

function do_pteditor {
  cd $PROJ_ROOT/scripts
  source env-x86.sh

  cd $PROJ_ROOT/ext/pteditor/module/ && make
}

function do_gdev_guest_kernel {
  cd $PROJ_ROOT/src/gpu_driver/gdev-guest
  ./build_gdev_stub_arch64.sh
}

function do_benchmarks {
  cd $PROJ_ROOT/src/gpu_driver/rodinia-bench/cuda
  #
  # XXX: We need cuda 5/nvcc to compile benchmarks
  #
  export PATH="/usr/local/cuda-5.0/bin:$PATH"
  bash ./setup.sh clean
  bash ./setup.sh nvcc
  bash ./setup.sh gcc
}


# "${@:2}"
case "$1" in
clean)
  do_clean
  ;;
init)
  do_init
  ;;
libhook)
  do_libhook
  ;;
fh_host)
  do_fh_host
  ;;
intercept)
  do_intercept
  ;;

fh_fixup)
  do_fh_fixup
  ;;
gpu_usr_manager)
  do_gpu_usr_manager
  ;;
gdev_guest_kernel)
  # build guest kernel module
  do_gdev_guest_kernel
  ;;
pteditor)
  do_pteditor
  ;;
benchmarks)
  do_benchmarks
  ;;
*)
  echo "unknown command given"
  exit 1
  ;;
esac

echo "-- script executed successfully"
