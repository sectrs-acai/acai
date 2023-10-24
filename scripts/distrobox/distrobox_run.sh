#!/usr/bin/env bash

#
# Run in container:
# ./distrobox_run.sh [<command>]
#

set -euo pipefail
readonly SCRIPT_DIR=$(readonly script_dir=$( dirname "$(readlink -f "$0")" ) )
name=trusted-periph-distrobox
home_dir=$SCRIPT_DIR/home

if [ "$(hostname | grep -c "$name")" -gt 0 ]; then
    echo "Error: already in container $name."
    exit
fi


if ! command -v distrobox &> /dev/null
then
    echo "<distrobox> could not be found. Download from https://github.com/89luca89/distrobox"
    exit
fi

if [ "$(distrobox-list | grep -c "$name")" -eq 0 ]; then
    echo "no container found with name $name. Create container first"
    echo "run $SCRIPT_DIR/distrobox_setup.sh"
    exit
fi


CMD=${1:-}

if [ -z "$CMD" ]; then
    distrobox enter $name
else
    distrobox enter $name -- $@
fi
