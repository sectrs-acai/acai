#!/usr/bin/env bash

#
# toolchain for aarch64 and lts 5.10 kernel headers
#

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source $SCRIPT_DIR/../env.sh
source $OUTPUT_LINUX_GUEST_DIR/host/environment-setup

export PS1="tools-aarch64: "
alias make=$SCRIPTS_DIR/make-aarch64.sh
