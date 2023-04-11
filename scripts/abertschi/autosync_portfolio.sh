#!/bin/bash
script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
sync_dir="$script_dir/../../../fast-models-portfolio"

host=armcca@192.33.93.245
target=/home/armcca/fast-models-portfolio
ignore="$script_dir/.sync-ignore"

cd $ROOT_DIR
set -x

function sync {
    rsync -v --exclude-from="$ignore" -a $sync_dir $host:$target --checksum --ignore-times
}

sync
# inotifywait -r $script_dir;
while true; do
    sync
    sleep 2
done
