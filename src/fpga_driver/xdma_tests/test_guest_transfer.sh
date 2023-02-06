#!/bin/zsh
set -euo pipefail

script_dir=$(cd -P -- "$(dirname -- "$0")" && printf '%s\n' "$(pwd -P)")


$script_dir/test_guest_load_driver.sh

#---------------------------------------------------------------------
# Script variables
#---------------------------------------------------------------------
tool_path=$script_dir/../xdma_stub/linux-kernel/tools
test_path=$script_dir/../xdma_stub/linux-kernel/tests

# Size of PCIe DMA transfers that will be used for this test.
# Make sure valid addresses exist in the FPGA when modifying this
# variable. Addresses in the range of 0 - (4 * transferSize) will
# be used for this test when the PCIe DMA core is setup for memory
# mapped transaction.
transferSize=1024
# Set the number of times each data transfer will be repeated.
# Increasing this number will allow transfers to accross multiple
# channels to over lap for a longer period of time.
transferCount=1

# Determine which Channels are enabled
# Determine if the core is Memory Mapped or Streaming

isStreaming=0
h2cChannels=4
c2hChannels=4


# Perform testing on the PCIe DMA core.
testError=0
if [[ $isStreaming -eq 0 ]]; then

	# Run the PCIe DMA memory mapped write read test
	#set -x
	$test_path/dma_memory_mapped_test.sh xdma0 $transferSize $transferCount $h2cChannels $c2hChannels
	returnVal=$?
	#set +x
	 if [ $returnVal -eq 1 ]; then
		testError=1
	fi

else

	# Run the PCIe DMA streaming test
	channelPairs=$(($h2cChannels < $c2hChannels ? $h2cChannels : $c2hChannels))
	if [[ $channelPairs -gt 0 ]]; then
		#set -x
		$test_path/dma_streaming_test.sh $transferSize $transferCount $channelPairs
		returnVal=$?
		#set +x
		if [[ $returnVal -eq 1 ]]; then
			testError=1
		fi
	else
		echo "Info: No PCIe DMA stream channels were tested because no h2c/c2h pairs were found."
	fi

fi

# Exit with an error code if an error was found during testing
if [[ $testError -eq 1 ]]; then
	echo "Error: Test completed with Errors."
	exit 1
fi

# Report all tests passed and exit
echo "Info: All tests in run_tests.sh passed."
exit 0
