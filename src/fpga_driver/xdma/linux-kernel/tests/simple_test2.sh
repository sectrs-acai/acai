#!/bin/sh

#---------------------------------------------------------------------
# Script variables
#---------------------------------------------------------------------
tool_path=../tools

$tool_path/reg_rw /dev/xdma0_control 0x0000 w
$tool_path/reg_rw /dev/xdma0_control 0x0100 w
$tool_path/reg_rw /dev/xdma0_control 0x0200 w
$tool_path/reg_rw /dev/xdma0_control 0x0300 w


../tools/dma_to_device -d /dev/xdma0_h2c_0 -f data/datafile0_4K.bin -s 1024 -a 0 -c 1

../tools/dma_from_device -d /dev/xdma0_c2h_0 -f data/output_datafile0_4K.bin -s 1024 -a 0 -c 1
