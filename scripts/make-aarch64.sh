#!/usr/bin/env bash

#
# make wrapper for aarch64, arm cca kernel headers
#

set -euo pipefail
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source $SCRIPT_DIR/../env.sh
source env-aarch64.sh
export LINUX_DIR=$OUTPUT_LINUX_CCA_GUEST_HEADERS

make "$@"
