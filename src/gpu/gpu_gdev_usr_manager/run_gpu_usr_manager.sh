#!/bin/bash

name="FVP_Base_RevC-2xAEMvA"
CUR_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
# source $CUR_DIR/../../../scripts/env-x86.sh

pid_target=$(pgrep -f -n "$name" | head -1)

while [ -z "$pid_target" ]
do
  echo "$name not found. please start it"
  sleep 3
  pid_target=$(pgrep -f "$name" | head -1)
done

set -x
# LD_LIBRARY_PATH="$CUR_DIR/lib:$LD_LIBRARY_PATH"
# ensure to install with cmake

# this is quite hacky. quick and dirty workaround

# PATH_ADD="/usr/local/gdev/lib64/:$OUTPUT_LINUX_HOST_DIR/staging/usr/lib"

echo "old path: $LD_LIBRARY_PATH"
PATH_ADD="/usr/local/gdev/lib64/"
sudo LD_LIBRARY_PATH="$PATH_ADD" ./gpu_gdev_usr_manager $pid_target
