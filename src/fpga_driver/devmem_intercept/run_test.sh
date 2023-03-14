#!/bin/sh
set -x
rmmod devmem_intercept_test.ko
insmod devmem_intercept_test.ko realm=1
