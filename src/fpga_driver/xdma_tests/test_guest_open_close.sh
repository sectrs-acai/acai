#!/bin/zsh
# set -euo pipefail

script_dir=$(cd -P -- "$(dirname -- "$0")" && printf '%s\n' "$(pwd -P)")


$script_dir/test_guest_load_driver.sh

#---------------------------------------------------------------------
# Script variables
#---------------------------------------------------------------------
tool_path=$script_dir/../xdma_stub/linux-kernel/tools
has_error=0
times=10

for ((i=0; i<=10; i++)); do
	v=`$tool_path/reg_rw /dev/xdma0_control 0x0${i}00 w`
	returnVal=$?
	if [[ $returnVal -ne 0 ]]; then
		echo "got error: $returnVal"
		has_error=1
	fi
done

if [[ $has_error -eq 1 ]]; then
		echo "got errors: "
fi
