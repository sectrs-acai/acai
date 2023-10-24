#!/usr/bin/env bash

sudo update-alternatives --set gcc /usr/bin/gcc-4.8
sudo update-alternatives --set g++ /usr/bin/g++-4.8
export PATH="/usr/local/cuda/bin:$PATH"
