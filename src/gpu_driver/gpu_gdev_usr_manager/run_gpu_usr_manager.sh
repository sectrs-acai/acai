#!/bin/bash

name="FVP_Base_RevC-2xAEMvA"
CUR_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source $CUR_DIR/../../../scripts/env-x86.sh

pid_target=$(pgrep -f -n "$name" | head -1)

if (( $# > 0))
then
  echo "using pid from argument"
  pid_target=$1
else
  while [ -z "$pid_target" ]
  do
    echo "$name not found. please start it"
    sleep 5
    pid_target=$(pgrep -f "$name" | head -1)
  done
fi
set -x

#LD_LIBRARY_PATH="$STAGING_DIR/lib/:$LD_LIBRARY_PATH"
LD_LIBRARY_PATH="$STAGING_DIR/usr/lib/:$LD_LIBRARY_PATH"
LD_LIBRARY_PATH="$STAGING_DIR/usr/local/gdev/lib64/:$LD_LIBRARY_PATH"



cat /proc/$pid_target/cmdline


# XXX: we pin manager to a single core
sudo LD_LIBRARY_PATH="$LD_LIBRARY_PATH" nice -20 taskset -c 3 ./gpu_gdev_usr_manager $pid_target
