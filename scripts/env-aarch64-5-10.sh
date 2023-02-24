#!/usr/bin/env bash

#
# toolchain for aarch64 and lts 5.10 kernel headers
#

source $(git rev-parse --show-toplevel)/env.sh
source $OUTPUT_LINUX_GUEST_DIR/host/environment-setup

export PS1="tools-aarch64: "
alias make=$SCRIPTS_DIR/make-aarch64.sh
