#!/usr/bin/env sh
set +x
core=0
p=$(pgrep lkvm)
taskset -c $core -p $p

set -x
echo "Stop tracing"
