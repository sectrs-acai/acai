#!/bin/bash
script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
sync_dir=$script_dir/..

host=bean@192.33.93.250
target=/home/bean
ignore="$script_dir/.sync-ignore"


rsync -v --filter=":- $ignore" -a $sync_dir $host:$target
while inotifywait -r $script_dir; do
    rsync -v --filter=":- $ignore" -a $sync_dir $host:$target
    sleep 0.5
done
