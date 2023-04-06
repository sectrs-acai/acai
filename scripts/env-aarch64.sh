#!/usr/bin/env bash

#
# toolchain for aarch64 and arm cca kernel headers
#
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source $SCRIPT_DIR/../env.sh
source $OUTPUT_LINUX_CCA_GUEST_DIR/host/environment-setup

export PS1="tools-cca-aarch64: "
export LINUX_DIR=$OUTPUT_LINUX_CCA_GUEST_HEADERS
export LINUXDIR=$OUTPUT_LINUX_CCA_GUEST_HEADERS
echo $PATH

usr_lib=$OUTPUT_LINUX_CCA_GUEST_DIR/target/usr/lib
lib=$OUTPUT_LINUX_CCA_GUEST_DIR/target/lib
usr_local_lib=$OUTPUT_LINUX_CCA_GUEST_DIR/target/usr/local/lib
gdev_lib=$OUTPUT_LINUX_CCA_GUEST_DIR/target/usr/local/gdev/lib64

#export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$gdev_lib:$lib:$usr_lib:$usr_local_lib"
