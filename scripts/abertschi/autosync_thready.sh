#!/bin/bash
script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
sync_dir=$script_dir/../..

host=bean@192.33.93.250
target=/home/bean/trusted-peripherals
ignore="$script_dir/.sync-ignore"

cd $ROOT_DIR
set -x

function sync {
    rsync -v --exclude-from="$ignore" -a $sync_dir $host:$target --checksum --ignore-times
}

sync
while inotifywait -r $script_dir; do
    sync
    sleep 0.5
done
