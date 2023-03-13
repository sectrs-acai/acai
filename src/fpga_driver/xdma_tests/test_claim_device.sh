#!/bin/sh

set -x

driver_new=xdma-pci-stub
driver_old=virtio-pci
devid="1af4 1001"

echo -n "0000:00:00.0" > /sys/bus/pci/drivers/$driver_old/unbind
echo -n "0000:00:02.0" > /sys/bus/pci/drivers/$driver_old/unbind

echo -n "0000:00:00.0" > /sys/bus/pci/drivers/$driver_new/unbind
echo -n "0000:00:02.0" > /sys/bus/pci/drivers/$driver_new/unbind

echo -n $devid > /sys/bus/pci/drivers/$driver_new/new_id

ls -al /sys/bus/pci/devices/0000:00:00.0
ls -al /sys/bus/pci/devices/0000:00:02.0
