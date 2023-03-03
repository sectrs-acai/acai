#!/usr/bin/env bash

#
# make wrapper for aarch64, arm cca kernel headers
#

set -euo pipefail
source $(git rev-parse --show-toplevel)/env.sh
source env-aarch64-realm.sh
export LINUX_DIR=$OUTPUT_LINUX_CCA_REALM_HEADERS

make "$@"
