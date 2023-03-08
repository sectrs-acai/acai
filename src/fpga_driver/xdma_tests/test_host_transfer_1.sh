#!/bin/zsh
# set -euo pipefail

script_dir=$(cd -P -- "$(dirname -- "$0")" && printf '%s\n' "$(pwd -P)")


$script_dir/test_host_load_driver.sh

#---------------------------------------------------------------------
# Script variables
#---------------------------------------------------------------------
tool_path=$script_dir/../xdma/linux-kernel/tools
test_path=$script_dir/../xdma/linux-kernel/tests
data_path=$script_dir/../xdma/linux-kernel/tests/data
testError=0

$tool_path/dma_to_device -d /dev/xdma0_h2c_0 -f $data_path/datafile0_4K.bin -s 1024 -a 0 -c 1
returnVal=$?
if [[ $returnVal -eq 1 ]]; then
	testError=1
fi
$tool_path/dma_from_device -d /dev/xdma0_c2h_0 -f $data_path/output_datafile0_4K.bin -s 1024 -a 0 -c 1
returnVal=$?
if [[ $returnVal -eq 1 ]]; then
	testError=1
fi

cmp $data_path/output_datafile0_4K.bin $data_path/datafile0_4K.bin -n 1024
returnVal=$?
if [[ $returnVal -eq 1 ]]; then
	testError=1
fi

# Exit with an error code if an error was found during testing
if [[ $testError -eq 1 ]]; then
	echo "Error: Test completed with Errors."
	exit 1
fi

# Report all tests passed and exit
echo "Info: All tests in run_tests.sh passed."
exit 0
