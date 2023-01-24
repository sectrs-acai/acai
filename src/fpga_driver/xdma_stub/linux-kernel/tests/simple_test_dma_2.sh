#!/bin/sh


../tools/dma_to_device -d /dev/xdma0_h2c_0 -f data/datafile0_4K.bin -s 1024 -a 0 -c 1

../tools/dma_from_device -d /dev/xdma0_c2h_0 -f data/output_datafile0_4K.bin -s 1024 -a 0 -c 1
