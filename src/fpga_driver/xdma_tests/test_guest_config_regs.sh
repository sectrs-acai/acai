#!/bin/zsh
set -euo pipefail

script_dir=$(cd -P -- "$(dirname -- "$0")" && printf '%s\n' "$(pwd -P)")


$script_dir/test_guest_load_driver.sh

#---------------------------------------------------------------------
# Script variables
#---------------------------------------------------------------------
tool_path=$script_dir/../xdma_stub/linux-kernel/tools

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
h2cChannels=0
for ((i=0; i<=3; i++)); do
	v=`$tool_path/reg_rw /dev/xdma0_control 0x0${i}00 w`
	returnVal=$?
	if [ $returnVal -ne 0 ]; then
		break;
	fi

	#v=`echo $v | grep -o  '): 0x[0-9a-f]*'`
	statusRegVal=`$tool_path/reg_rw /dev/xdma0_control 0x0${i}00 w | grep "Read.*:" | sed 's/Read.*: 0x\([a-z0-9]*\)/\1/'`
	channelId=${statusRegVal:0:3}
	streamEnable=${statusRegVal:4:1}

	if [[ $channelId == "1fc" ]]; then
		h2cChannels=$((h2cChannels + 1))
		if [[ $streamEnable -eq "8" ]]; then
			isStreaming=1
		fi
	fi
done
echo "Info: Number of enabled h2c channels = $h2cChannels"

# Find enabled c2hChannels
c2hChannels=0
for ((i=0; i<=3; i++)); do
    
	v=`$tool_path/reg_rw /dev/xdma0_control 0x1${i}00 w`
	returnVal=$?
    echo $v
	if [[ $returnVal -ne 0 ]]; then
		break;
	fi

	$tool_path/reg_rw /dev/xdma0_control 0x1${i}00 w | grep "Read.*: 0x1fc" > /dev/null
	statusRegVal=`$tool_path/reg_rw /dev/xdma0_control 0x1${i}00 w | grep "Read.*:" | sed 's/Read.*: 0x\([a-z0-9]*\)/\1/'`
	channelId=${statusRegVal:0:3}

	# there will NOT be a mix of MM & ST channels, so no need to check
	# for streaming enabled
	if [[ $channelId == "1fc" ]]; then
		c2hChannels=$((c2hChannels + 1))
	fi
done
echo "Info: Number of enabled c2h channels = $c2hChannels"

# Report if the PCIe DMA core is memory mapped or streaming
if [[ $isStreaming -eq 0 ]]; then
	echo "Info: The PCIe DMA core is memory mapped."
else
	echo "Info: The PCIe DMA core is streaming."
fi

# Check to make sure atleast one channel was identified
if [[ $h2cChannels -eq 0 &&  $c2hChannels -eq 0 ]]; then
	echo "Error: No PCIe DMA channels were identified."
	exit 1
fi
