#!/bin/bash
script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

host=bean@192.33.93.250
target=/home/bean/scripts-sidechannel

ignore="$script_dir/.gitignore"
rsync -v --filter=":- $ignore" -a $script_dir $host:$target

while inotifywait -r $script_dir; do
    rsync -v --filter=":- $ignore" -a $script_dir $host:$target
    sleep 0.5
done
