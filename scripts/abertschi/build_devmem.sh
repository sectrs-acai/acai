#!/usr/bin/env bash

HERE_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
set -euo pipefail

source $HERE_DIR/../../env.sh

set +x

rm -rf $ROOT_DIR/assets/snapshots
mkdir $ROOT_DIR/assets/snapshots

cd $ROOT_DIR/src/rmm
git checkout trusted-periph/master
git pull gitlab-rmm trusted-periph/master

cd $ROOT_DIR/src/tfa
git checkout trusted-periph/master
git pull gitlab-rmm trusted-periph/master

cd $ROOT_DIR/src/kvmtool/kvmtool
git checkout trusted-periph/master
git pull gitlab-rmm trusted-periph/master

cd $ROOT_DIR/src/linux-cca-guest
git checkout trusted-periph/linux-cca-guest
git pull gitlab-rmm trusted-periph/linux-cca-guest

cd $ROOT_DIR/buildconf/tfa
./setup.sh clean
./setup.sh linux

cd $ROOT_DIR/buildconf/shrinkwrap
./setup.sh clean
./setup.sh kvmtool

cd $ROOT_DIR/buildconf/linux-cca-guest
./setup.sh linux
