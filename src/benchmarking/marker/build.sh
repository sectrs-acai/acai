#!/usr/bin/env bash

HERE=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source $HERE/../../../scripts/env-aarch64.sh

aarch64-none-linux-gnu-gcc start.c -o start
aarch64-none-linux-gnu-gcc stop.c -o stop
