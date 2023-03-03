#!/usr/bin/env bash

#
# toolchain for aarch64 and arm cca kernel headers
#
source $(git rev-parse --show-toplevel)/env.sh
source $OUTPUT_LINUX_CCA_REALM_DIR/host/environment-setup

export PS1="tools-cca-realm-aarch64: "
alias make=$SCRIPTS_DIR/make-aarch64-realm.sh
