#!/bin/bash

name="FVP_Base_RevC-2xAEMvA"
CUR_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source $CUR_DIR/../../../scripts/env-x86.sh

pid_target=$(pgrep -f -n "$name" | head -1)

while [ -z "$pid_target" ]
do
  echo "$name not found. please start it"
  sleep 5
  pid_target=$(pgrep -f "$name" | head -1)
done

set -x

#LD_LIBRARY_PATH="$STAGING_DIR/lib/:$LD_LIBRARY_PATH"
LD_LIBRARY_PATH="$STAGING_DIR/usr/lib/:$LD_LIBRARY_PATH"
LD_LIBRARY_PATH="$STAGING_DIR/usr/local/gdev/lib64/:$LD_LIBRARY_PATH"

sudo nice -20 LD_LIBRARY_PATH="$LD_LIBRARY_PATH" ./gpu_gdev_usr_manager $pid_target


