import os
import numpy as np
import struct
import pyaes

import os
import numpy as np
from ctypes import *
import sys

#
# XXX: set to 1 to show stdout do_prints
#      set warmup and bench iters accordingly
#
# SVD: can only run one run before flashing bitsream again!

print_output = 1
warmup_iter = 0
bench_iter = 1

if len(sys.argv) > 1:
    param_size = int(sys.argv[1])
else:
    param_size = 32

print("using param size: ", param_size)


def do_print(vargs):
    if print_output:
        print(vargs)


so_file = "/mnt/host/bench.so"
benchmark = CDLL(so_file)


c2h_dma_device='/dev/xdma0_c2h_1'
c2h_fd = os.open(c2h_dma_device, os.O_RDWR)

h2c_dma_device='/dev/xdma0_h2c_1'
h2c_fd = os.open(h2c_dma_device, os.O_RDWR)

buff_enc = []
buff_dev = []
def enc(buff):
    aes = pyaes.AESModeOfOperationCTR(os.urandom(32))
    ciphertext = aes.encrypt(str(buff))
    # do_print('Encrypted:', binascii.hexlify(ciphertext))
    return ciphertext

def dec(buff):
    aes = pyaes.AESModeOfOperationCTR(os.urandom(32))
    plaintext = aes.decrypt(str(buff))
    # do_print('Encrypted:', binascii.hexlify(ciphertext))
    return plaintext

def write_data(inst_buffer_addr,data):
    global buff_enc
    os.lseek(h2c_fd, inst_buffer_addr , 0)
    os.write(h2c_fd,data)
    buff_enc = enc(data)


def read_data(inst_buffer_addr,length):
    global buff_dec
    os.lseek(c2h_fd, inst_buffer_addr, 0)
    res = os.read(c2h_fd, length)
    buff_dec = dec(res)
    return res

A_addr = 0x0000000040000000
S_addr = 0x0000000050000000
U_addr = 0x0000000060000000
V_addr = 0x0000000070000000

control_addr = 0x00

size_r = param_size
size_c = param_size

arr_size = size_r * size_c
int_size = 4

print("size_r:", size_r)
print("size_c:", size_c)
print("arr_size:", arr_size)
print("int_size:", int_size)


A_recon_bytes = []
S_recon_bytes = []
U_recon_bytes = []
V_recon_bytes = []


def write_matrices():
    do_print("Writing 1s to A")
    write_data(A_addr,bytearray([0x01, 0x02, 0x03, 0x04] * arr_size))

    do_print("Writing 1s to S")
    write_data(S_addr,bytearray([0x01, 0x00, 0x00, 0x00] * arr_size))

    do_print("Writing 1s to U")
    write_data(U_addr,bytearray([0x01, 0x00, 0x00, 0x00] * arr_size))

    do_print("Writing 1s to V")
    write_data(V_addr,bytearray([0x01, 0x00, 0x00, 0x00] * arr_size))

def write_addresses():
    do_print("Writing address")
    buff_0 = bytearray([0x00, 0x00, 0x00, 0x00])
    ip_addr_1 = 0x30
    buff_addr = bytearray([0x00, 0x00, 0x00, 0x40])
    write_data(ip_addr_1,buff_addr)
    ip_addr_2 = 0x34
    write_data(ip_addr_2,buff_0)

    ip_addr_1 = 0x40
    buff_addr = bytearray([0x00, 0x00, 0x00, 0x50])
    write_data(ip_addr_1,buff_addr)
    ip_addr_2 = 0x44
    write_data(ip_addr_2,buff_0)

    ip_addr_1 = 0x50
    buff_addr = bytearray([0x00, 0x00, 0x00, 0x60])
    write_data(ip_addr_1,buff_addr)
    ip_addr_2 = 0x54
    write_data(ip_addr_2,buff_0)

    ip_addr_1 = 0x60
    buff_addr = bytearray([0x00, 0x00, 0x00, 0x70])
    write_data(ip_addr_1,buff_addr)
    ip_addr_2 = 0x64
    write_data(ip_addr_2,buff_0)

def start_execution():
    do_print("Starting execution")
    read_data(control_addr, 2)
    write_data(control_addr,bytearray([int('0b00000001',2)]))

def wait_for_done():
    print("Wait for done")
    val = int.from_bytes(read_data(control_addr, 1),'big')
    while val & 1 == 1:
        print("Waiting ", val)
        val = int.from_bytes(read_data(control_addr, 1),'big')
    print("Done")

def read_results():
    global A_recon_bytes
    global S_recon_bytes
    global U_recon_bytes
    global V_recon_bytes
    do_print("Reading results")
    do_print('A')
    A_recon_bytes = read_data(A_addr, int_size*arr_size)

    do_print('S')
    S_recon_bytes = read_data(S_addr, int_size*arr_size)

    do_print('U')
    U_recon_bytes = read_data(U_addr, int_size*arr_size)

    do_print('V')
    V_recon_bytes = read_data(V_addr, int_size*arr_size)

    do_print("done write/read ")


def check_results():
    do_print("Checking results")
    size_r = 32
    size_c = 32
    A = []
    S = []
    U = []
    V = []
    for i in range(0,size_r):
        A_rows = []
        S_rows = []
        U_rows = []
        V_rows = []
        for j in range(0,size_c):
            fr = ((i*size_c)+j)*4
            to = fr+4
            val = int.from_bytes(A_recon_bytes[fr : to], 'little')
            A_rows.append(val)

            val = int.from_bytes(S_recon_bytes[fr : to], 'little')
            S_rows.append(val)

            val = int.from_bytes(U_recon_bytes[fr : to], 'little')
            U_rows.append(val)

            val = int.from_bytes(V_recon_bytes[fr : to], 'little')
            V_rows.append(val)
        A.append(A_rows)
        S.append(S_rows)
        U.append(U_rows)
        V.append(V_rows)
    a = np.array(A)
    s = np.array(S)
    u = np.array(U)
    v = np.array(V)
    do_print(a)
    do_print(s)
    do_print(u)
    do_print(v)

    
def do_benchmark(verify_res = False):
    benchmark.CCA_H_TO_D_START()
    write_matrices()
    benchmark.CCA_H_TO_D_STOP()

    benchmark.CCA_INIT_START()
    write_addresses()
    benchmark.CCA_INIT_STOP()

    benchmark.CCA_EXEC_START()
    start_execution()
    wait_for_done()
    benchmark.CCA_EXEC_STOP()

    benchmark.CCA_D_TO_H_START()
    read_results()
    benchmark.CCA_D_TO_H_STOP()



if __name__ == '__main__':
    benchmark.CCA_FPGA_INIT()

    print_output = 1
    print("warmup iterations: ", warmup_iter)
    print("bench iterations: ", bench_iter)

    for i in range(warmup_iter):
        do_benchmark(verify_res=True)

    print("starting benchmark...")
    print_output = 0
    benchmark.CCA_FPGA_START()

    for i in range(warmup_iter):
        benchmark.CCA_FPGA_WARMUP_ITER()

    for i in range(bench_iter):
        benchmark.CCA_FPGA_BENCH_ITER()
        do_benchmark(verify_res=False)

    benchmark.CCA_FPGA_END()

    print("done")

