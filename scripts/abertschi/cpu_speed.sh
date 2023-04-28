#!/usr/bin/env bash
set -euo pipefail
HERE=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

set +x

freq=3000000
gov=performance
boost=0

echo $boost | sudo tee /sys/devices/system/cpu/cpufreq/boost

echo "current scaling freq: "
grep . /sys/devices/system/cpu/cpu*/cpufreq/scaling_cur_freq
grep . /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor


echo $gov | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
echo $freq | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_min_freq
echo $freq | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_max_freq

sleep 2

grep . /sys/devices/system/cpu/cpu*/cpufreq/scaling_cur_freq
grep . /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
grep . /sys/devices/system/cpu/cpu*/cpufreq/scaling_max_freq
grep . /sys/devices/system/cpu/cpu*/cpufreq/scaling_min_freq
