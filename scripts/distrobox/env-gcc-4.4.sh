#!/usr/bin/env bash

sudo update-alternatives --set gcc /usr/bin/gcc-4.4
sudo update-alternatives --set g++ /usr/bin/g++-4.4
export PATH="/usr/local/cuda-5.0/bin:$PATH"
