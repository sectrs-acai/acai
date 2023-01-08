#!/usr/bin/env bash
set -euo pipefail

source $(git rev-parse --show-toplevel)/env.sh
source env-x86.sh

export LINUX_DIR=$OUTPUT_LINUX_HOST_HEADERS
make "$@"
