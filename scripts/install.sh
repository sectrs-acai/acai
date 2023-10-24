#!/usr/bin/env bash

#
# artifact evaluation:
#
# Downloads and builds all software packages
# to run a realm VM on the FVP
#
# we provide a buildroot environment to source into,
# with all host dependencies available to compile the projects.
# this script runs build scripts of all components in the repo
#

set -Eeou pipefail
SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)
PROJ_ROOT="$SCRIPT_DIR/.."

log_file="$SCRIPT_DIR/stdout.log"

rm "$log_file" || true

exec &> >(tee -a $log_file)

# platform
ok_init="skip"
ok_linux_cca_guest="skip"
ok_linux_cca_realm="skip"
ok_linux_host="skip"
ok_tfa="skip"
ok_kvmtool="skip"
ok_minimal_example="skip"

# fpga
ok_aarch64_intercept_mod="skip"
ok_aarch64_xdma_driver="skip"
ok_fh_host_lib="skip"
ok_libhook="skip"
ok_fpga_usr_manager="skip"
ok_xdma_host_driver="skip"
ok_pteditor="skip"

# gpu
ok_gpu_fh_host="skip"
ok_gpu_intercept="skip"
ok_gpu_libhook="skip"
ok_gpu_fh_fixup="skip"
ok_gpu_usr_manager="skip"
ok_gpu_gdev_guest_kernel="skip"
ok_gpu_pteditor="skip"
ok_gpu_benchmarks="skip"

trap 'error_handler $? $LINENO' ERR SIGINT
trap 'error_handler $? $LINENO' SIGINT

print_status() {

  echo "Summary:"
  echo ""
  echo "platform:"
  echo "init.............................................: $ok_init"
  echo "build Aarch64 CCA-enabled NS kernel and rootfs...: $ok_linux_cca_guest"
  echo "build Aarch64 CCA-enabled Realm rootfs...........: $ok_linux_cca_realm"
  echo "build x86 host...................................: $ok_linux_host"
  echo "build tfa, rmm...................................: $ok_tfa"
  echo "build minimal example............................: $ok_minimal_example"
  # echo "build kvmtool (VMM)..............................: $ok_kvmtool"
  echo ""
  echo "fpga:"
  echo "build aarch64_acai_kernel_helper................: $ok_aarch64_intercept_mod"
  echo "build aarch64_xdma_driver ......................: $ok_aarch64_xdma_driver"
  echo "build x86_xdma_driver ..........................: $ok_xdma_host_driver"
  echo "build fh_host ..................................: $ok_fh_host_lib"
  echo "build fvp_libhook ..............................: $ok_libhook"
  echo "build fpga_usr_manager .........................: $ok_fpga_usr_manager"
  echo "build pteditor .................................: $ok_pteditor"
  echo ""
  echo "gpu:"
  echo "build aarch64_acai_kernel_helper................: $ok_gpu_intercept"
  echo "build gpu_aarch64_kernel_module.................: $ok_gpu_gdev_guest_kernel"
  echo "build fh_host ..................................: $ok_gpu_fh_host"
  echo "build fvp_libhook ..............................: $ok_gpu_libhook"
  echo "build gpu_fh_fixup .............................: $ok_gpu_fh_fixup"
  echo "build gpu_usr_manager ..........................: $ok_gpu_usr_manager"
  echo "build gpu_pteditor .............................: $ok_gpu_pteditor"
  echo "build gpu_benchmarks ...........................: $ok_gpu_benchmarks"
  echo ""
  echo "Logfile: $log_file"

}

error_handler() {
  echo "Error in install.sh script: $? occurred on $1"
  print_status
  exit 1
}

function init_repo() {
  cd $PROJ_ROOT
  ./scripts/init.sh
  ok_init="success"
}

function build_linux_cca_guest() {
  cd $PROJ_ROOT/buildconf/linux-cca-guest/
  ./setup.sh init
  ./setup.sh build
  ok_linux_cca_guest="success"
}

function build_linux_cca_realm() {
  cd $PROJ_ROOT/buildconf/linux-cca-realm/
  ./setup.sh init
  ./setup.sh build
  ok_linux_cca_realm="success"
}

function build_linux_host() {
  cd $PROJ_ROOT/buildconf/linux-host/
  ./setup.sh init
  ./setup.sh build
  ok_linux_host="success"
}

function build_tfa() {
  cd $PROJ_ROOT/buildconf/tfa/
  ./setup.sh clean
  ./setup.sh init
  ./setup.sh linux
  ok_tfa="success"
}

function build_kvmtool() {
  cd $PROJ_ROOT/buildconf/shrinkwrap/
  ./setup.sh clean
  ./setup.sh init
  ./setup.sh kvmtool
  ok_kvmtool="success"
}

function build_minimal_example() {
  cd $PROJ_ROOT/src/testing/testengine
  ./build.sh
  ok_minimal_example="success"
}

function build_gpu() {
  cd $PROJ_ROOT/src/gpu_driver

  ./setup.sh clean
  ./setup.sh init

  ./setup.sh fh_host
  ok_gpu_fh_host="success"

  ./setup.sh intercept
  ok_gpu_intercept="success"

  ./setup.sh libhook
  ok_gpu_libhook="success"

  ./setup.sh fh_fixup
  ok_gpu_fh_fixup="success"

  ./setup.sh gpu_usr_manager
  ok_gpu_usr_manager="success"

  ./setup.sh gdev_guest_kernel
  ok_gpu_gdev_guest_kernel="success"

  ./setup.sh pteditor
  ok_gpu_pteditor="success"

  ./setup.sh benchmarks
  ok_gpu_benchmarks="success"
}

function build_fpga() {
  cd $PROJ_ROOT/src/fpga_driver
  ./setup.sh clean
  ./setup.sh init

  # aarch64 components
  ./setup.sh intercept
  ok_aarch64_intercept_mod="success"

  ./setup.sh xdma_stub
  ok_aarch64_xdma_driver="success"

   ./setup.sh xdma
  ok_xdma_host_driver="success"

  # x86 components
  ./setup.sh fh_host
  ok_fh_host_lib="success"

  ./setup.sh libhook
  ok_libhook="success"

  ./setup.sh fpga_usr_manager
  ok_fpga_usr_manager="success"

  ./setup.sh pteditor
  ok_pteditor="success"
}

function build_platform()
{
  build_linux_cca_guest
  build_linux_cca_realm
  build_linux_host
  build_tfa

  # No need to build lkvm tool all the time, just use binary asset
  # lkvmtool is built within a docker container already.
  # spawning a docker container from within distrobox causes some setup overhead.
  # So for simplicity we do not built it all the time in this run script.
  # build_kvmtool

  build_minimal_example
}

function do_build() {
  build_platform
  build_fpga
  build_gpu
}

function do_init_and_build() {
  init_repo
  do_build
}

case "${1-}" in
  build)
    echo "skipping git init phase"
    time do_build
    ;;
  gpu)
    time build_gpu
    ;;
  platform)
    time build_platform
    ;;
  fpga)
    time build_fpga
    ;;
  *)
    time do_init_and_build
    ;;
esac

print_status
