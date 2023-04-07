#!/bin/sh
name="FVP_Base_RevC-2xAEMvA"

pid_target=$(pgrep -f -n "$name" | head -1)
while [ -z "$pid_target" ]
do
  echo "$name not found. please start it"
  sleep 3
  pid_target=$(pgrep -f "$name" | head -1)
done


sudo ./gpu_usr_manager $pid_target
