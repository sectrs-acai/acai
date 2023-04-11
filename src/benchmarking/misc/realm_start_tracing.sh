#!/usr/bin/env sh
set +x
core=3
p=$(pgrep lkvm)
taskset -c $core -p $p

set -x
echo "Starting tracing"
