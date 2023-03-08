#!/usr/bin/env bash

#
# make wrapper for aarch64, lts 5.10 kernel headers
#

set -euo pipefail

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source $SCRIPT_DIR/../env.sh
source env-aarch64-5-10.sh
export LINUX_DIR=$OUTPUT_LINUX_GUEST_HEADERS

make "$@"
