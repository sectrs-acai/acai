#!/usr/bin/env bash

#
# installs all optional dev packages
#

export DEBIAN_FRONTEND=noninteractive

apt-get install -y  \
    emacs tmux screen