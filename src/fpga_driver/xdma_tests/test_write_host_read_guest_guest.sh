#!/bin/zsh
set -euo pipefail

script_dir=$(cd -P -- "$(dirname -- "$0")" && printf '%s\n' "$(pwd -P)")


# 1. create temp file
output=/tmp/output.guest.dat
size=20971520

tool_path=$script_dir/../xdma_stub/linux-kernel/tools
test_path=$script_dir/../xdma_stub/linux-kernel/tests
data_path=$script_dir/../xdma_stub/linux-kernel/tests/data
testError=0


$tool_path/dma_from_device -d /dev/xdma0_c2h_0 -f $output -s $size -a 0 -c 1
returnVal=$?
if [[ $returnVal -eq 1 ]]; then
	testError=1
	echo "error"
	exit
fi

echo ok
