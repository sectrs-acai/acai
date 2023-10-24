#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

source $(git rev-parse --show-toplevel)/env.sh

$ASSETS_DIR/toolchain-arm/download-toolchain.sh
$ASSETS_DIR/toolchain-x86/download-toolchain.sh
$ASSETS_DIR/fvp/download-fvp.sh


echo "-- init completed successfully."
