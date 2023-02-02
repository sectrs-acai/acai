#!/bin/sh

tool_path=../tools

$tool_path/reg_rw /dev/xdma0_control 0x0100 w
$tool_path/reg_rw /dev/xdma0_control 0x0200 w
$tool_path/reg_rw /dev/xdma0_control 0x0300 w
