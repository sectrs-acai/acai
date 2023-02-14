#!/bin/zsh
# set -euo pipefail

script_dir=$(cd -P -- "$(dirname -- "$0")" && printf '%s\n' "$(pwd -P)")


$script_dir/test_guest_load_driver.sh
chunk=$(( 10 * 4096 ))

echo $chunk
transfile=datafile_32M.bin

#---------------------------------------------------------------------
# Script variables
#---------------------------------------------------------------------
tool_path=$script_dir/../xdma_stub/linux-kernel/tools
test_path=$script_dir/../xdma_stub/linux-kernel/tests
data_path=$script_dir/../xdma_stub/linux-kernel/tests/data
testError=0

$tool_path/dma_to_device -d /dev/xdma0_h2c_0 -f $data_path/$transfile -s $chunk -a 0 -c 1
returnVal=$?
if [[ $returnVal -eq 1 ]]; then
	testError=1
fi
$tool_path/dma_from_device -d /dev/xdma0_c2h_0 -f $data_path/output_${transfile} -s $chunk -a 0 -c 1
returnVal=$?
if [[ $returnVal -eq 1 ]]; then
	testError=1
fi

cmp $data_path/output_$transfile $data_path/$transfile -n $chunk
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
