#!/usr/bin/env bash
#
# this is executed before buildroot creates target rootfilesystem
#

set -e
set -x

BUILDROOT_DIR=$PWD
cd "$TARGET_DIR"

ls -al "$TARGET_DIR/lib"
ls -al "$STAGING_DIR/lib"

