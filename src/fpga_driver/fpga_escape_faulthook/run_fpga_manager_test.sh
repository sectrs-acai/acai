#!/bin/bash

p=`pgrep -f FVP_Base_RevC-2xAEMvA | tail -1`
sudo ./fpga_manager_test $p