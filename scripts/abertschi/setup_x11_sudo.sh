#!/usr/bin/env bash
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
set -euo pipefail

#
# make display accessible over ssh
# when running fvp with sudo (needed for page pinning)
set -x
xauth list | grep unix`echo $DISPLAY | cut -c10-12` > /tmp/xauth
sudo xauth add `cat /tmp/xauth`

