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
O_NAME="cuda_5.0.run"
URL="http://developer.download.nvidia.com/compute/cuda/5_0/rel-update-1/installers/cuda_5.0.35_linux_64_ubuntu11.10-1.run"

wget $URL -O $O_NAME
sh ./$O_NAME --tar mxvf

sudo cp InstallUtils.pm /usr/lib/x86_64-linux-gnu/perl-base || true
export $PERL5LIB

sh ./run_files/cudatoolkit_5.0.35_linux_64_ubuntu11.10.run -noprompt

ls -al /usr/local/cuda
