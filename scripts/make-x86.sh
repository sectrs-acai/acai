#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source $SCRIPT_DIR/../env.sh
source env-x86.sh

export LINUX_DIR=$OUTPUT_LINUX_HOST_HEADERS
make "$@"
