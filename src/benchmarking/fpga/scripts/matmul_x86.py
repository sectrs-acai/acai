#Works for matmul in /home/ubuntu/arm-cca/gitlab/fpga/matmul
#The corresponding vivado project is /home/ubuntu/Xilinx/Vivado2022/matmul
#The bitstream that works is in /home/ubuntu/Xilinx/Vivado2022/matmul/example_memory_working_matmul.runs for matrices of size 500x500 
#IP in /home/ubuntu/vitis-hls-ip-2022/matmul_100.zip for 100x100 and 
#bitstream in /home/ubuntu/Xilinx/Vivado2022/matmul_100/example_memory.runs/impl_1/design_1_wrapper.bit
#The saxi offsets are in ./axi_offsets/matmul.png

import os
import numpy as np 
from ctypes import *

#
# XXX: set to 1 to show stdout prints
#      set warup and bench iters accordingly
#
print_output = 1
warmup_iter = 5
bench_iter = 20

def do_print(vargs):
    if print_output:
        print(vargs)




c2h_dma_device='/dev/xdma0_c2h_1'
c2h_fd = os.open(c2h_dma_device, os.O_RDWR)

h2c_dma_device='/dev/xdma0_h2c_1'
h2c_fd = os.open(h2c_dma_device, os.O_RDWR)

def write_data(inst_buffer_addr,data):
    os.lseek(h2c_fd, inst_buffer_addr , 0)
    os.write(h2c_fd,data)

def read_data(inst_buffer_addr,length):
    os.lseek(c2h_fd, inst_buffer_addr, 0)
    res = os.read(c2h_fd, length)
    return res

size_r = 5 #500
size_c = 5 #500
size = size_r * size_c

print("size_r:", size_r)
print("size_c:", size_c)
print("size:", size)

def write_address():
    do_print("Writing address")
    ip_addr_1 = 0x10
    write_data(ip_addr_1,bytearray([0x00, 0x00, 0x00, 0x80]))
    ip_addr_2 = 0x14
    write_data(ip_addr_2,bytearray([0x00, 0x00, 0x00, 0x00]))

    ip_addr_1 = 0x1c
    write_data(ip_addr_1,bytearray([0x00, 0x00, 0x00, 0x90]))
    ip_addr_2 = 0x20
    write_data(ip_addr_2,bytearray([0x00, 0x00, 0x00, 0x00]))

    ip_addr_1 = 0x28
    write_data(ip_addr_1,bytearray([0x00, 0x00, 0x00, 0xA0]))
    ip_addr_2 = 0x2c
    write_data(ip_addr_2,bytearray([0x00, 0x00, 0x00, 0x00]))

def h_to_d():
    do_print("Writing 1s")
    data_addr_1 = 0x80000000
    write_data(data_addr_1,bytearray([0x01, 0x00, 0x00, 0x00] * size))
    data_addr_2 = 0x90000000
    write_data(data_addr_2,bytearray([0x01, 0x00, 0x00, 0x00] * size))

def start_exec():
    do_print("Starting execution")
    control_addr = 0x00
    write_data(control_addr,bytearray([int('0b00000001',2)]))

def wait_for_done():
    control_addr = 0x00
    do_print("Wait for done")
    while int.from_bytes(read_data(control_addr, 1),'big') & int('0b00000010',2) == 0:
        do_print("Waiting")
    do_print("Done")


def d_to_h(verify_res = False):
    data_addr_3 = 0xA0000000

    do_print("Reading results")

    res = read_data(data_addr_3, 4*size)

    if verify_res:
        expected_val = np.matmul(np.ones((size_r,size_c)), np.ones((size_r,size_c)))
        correct = 1
        print("Checking results")
        for i in range(0,size_r):
            for j in range(0,size_c):
                fr = ((i*size_c)+j)*4
                to = fr+4
                val = int.from_bytes(res[fr : to], 'little')
                if j%2 == 1:
                    continue
                if val == expected_val[i][j]:
                    None
                else:
                    correct = 0
                    print("Did not match ", i, j, val, expected_val[i][j])
                    break
        if correct == 0:
            print("Test failed")
        else:
            print("Test passed")


def do_benchmark(verify_res = False):
    write_address()

    h_to_d()

    start_exec()
    wait_for_done()
    d_to_h(verify_res)

do_benchmark()