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
