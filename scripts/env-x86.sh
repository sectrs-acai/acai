#!/usr/bin/env bash
source $(git rev-parse --show-toplevel)/env.sh
source $OUTPUT_LINUX_HOST_DIR/host/environment-setup

target_usr_lib=$OUTPUT_LINUX_HOST_DIR/target/usr/lib
target_lib=$OUTPUT_LINUX_HOST_DIR/target/lib
host_lib=$OUTPUT_LINUX_HOST_DIR/host/lib
host_usr_lib=$OUTPUT_LINUX_HOST_DIR/host/usr/lib

# XXX: All if we run something outside of the rootfs we add search path to LD_LIBRARY_PATH
# so we can access shared libraries on host too
export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$host_lib:$host_usr_lib"

export PS1="x86: "
