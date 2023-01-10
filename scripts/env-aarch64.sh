#!/usr/bin/env bash
source $(git rev-parse --show-toplevel)/env.sh
source $OUTPUT_LINUX_GUEST_DIR/host/environment-setup

export PS1="tools-aarch64: "
alias make=$SCRIPTS_DIR/make-aarch64.sh