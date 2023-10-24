#!/bin/bash

# cuda 5.5
# XXX Installing nvcc with support for fermi is a real pain
# We automate this in the container
#

# set -euo pipefail
readonly SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

export DEBIAN_FRONTEND=noninteractive

set -x

mkdir -p $SCRIPT_DIR/build
cd $SCRIPT_DIR/build
wget http://developer.download.nvidia.com/compute/cuda/5_5/rel/installers/cuda_5.5.22_linux_64.run
sh ./cuda*run --tar mxvf
sudo cp InstallUtils.pm /usr/lib/x86_64-linux-gnu/perl-base
export $PERL5LIB
sudo sh ./run_files/cuda-linux64-rel-5.5.22-16488124.run -noprompt

ls -al /usr/local/cuda
