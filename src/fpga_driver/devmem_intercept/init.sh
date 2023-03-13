#!/bin/sh

rmmod devmem_intercept.ko
insmod devmem_intercept.ko realm=1
