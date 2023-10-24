#!/usr/bin/env bash

set -euo pipefail
readonly SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
name=trusted-periph-distrobox
home_dir=$SCRIPT_DIR/home/

if ! command -v distrobox &> /dev/null
then
    echo "<distrobox> could not be found. Download from https://github.com/89luca89/distrobox"
    exit
fi

set -x
distrobox create --yes $name -i ubuntu:20.04 --home $home_dir

rm -rf $SCRIPT_DIR/build

cd $SCRIPT_DIR
echo "installing packages"
distrobox enter $name -- sudo ./install-ubuntu-packages.sh

distrobox enter $name -- sudo ./install-ubuntu-devpackages.sh

echo "installing cuda"
distrobox enter $name -- sudo ./install-cuda-5-0.sh
