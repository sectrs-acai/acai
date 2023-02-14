#!/bin/zsh
set -euo pipefail
script_dir=$(cd -P -- "$(dirname -- "$0")" && printf '%s\n' "$(pwd -P)")


$script_dir/test_guest_load_driver.sh
chunk=4096

tool_path=$script_dir/../xdma_stub/linux-kernel/tools
test_path=$script_dir/../xdma_stub/linux-kernel/tests
data_path=$script_dir/../xdma_stub/linux-kernel/tests/data
testError=0


