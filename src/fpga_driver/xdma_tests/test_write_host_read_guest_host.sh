#!/bin/zsh
set -euo pipefail

script_dir=$(cd -P -- "$(dirname -- "$0")" && printf '%s\n' "$(pwd -P)")


# 1. create temp file
input=/tmp/output.host.dat

# 2. write file to xdma memory
size=$(stat -c%s "$input")

echo "size is $size"

tool_path=$script_dir/../xdma/linux-kernel/tools
test_path=$script_dir/../xdma/linux-kernel/tests
data_path=$script_dir/../xdma/linux-kernel/tests/data
testError=0

$tool_path/dma_to_device -d /dev/xdma0_h2c_0 -f $input -s $size -a 0 -c 1
returnVal=$?
if [[ $returnVal -eq 1 ]]; then
	testError=1
    echo "error happened"
    exit
fi


echo "ok"
