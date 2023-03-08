#!/usr/bin/env zsh
set -euo pipefail
f=/tmp/output.dat
echo $f

dd if=/dev/random  of=$f  bs=1M  count=5
