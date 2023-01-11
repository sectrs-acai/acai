#!/usr/bin/env bash
set -euo pipefail
source $(git rev-parse --show-toplevel)/env.sh
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

$SCRIPT_DIR/../buildconf/linux-host/setup.sh init
$SCRIPT_DIR/../buildconf/linux-host/setup.sh build

$SCRIPT_DIR/../buildconf/linux-guest/setup.sh init
$SCRIPT_DIR/../buildconf/linux-guest/setup.sh build
