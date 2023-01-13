#!/bin/sh

#---------------------------------------------------------------------
# Script variables
#---------------------------------------------------------------------
tool_path=tools

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
i = 1

$tool_path/reg_rw /dev/xdma0_control 0x0${i}00 w 0x10
$tool_path/reg_rw /dev/xdma0_control 0x0${i}00 w

