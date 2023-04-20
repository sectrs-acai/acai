#!/bin/sh
set -x

# set realm=0 to run in ns
rmmod devmem_intercept.ko
insmod ./devmem_intercept.ko realm=1
