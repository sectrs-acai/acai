

import os
import numpy as np
import struct
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

A_addr = 0x0000000040000000
S_addr = 0x0000000050000000
U_addr = 0x0000000060000000
V_addr = 0x0000000070000000

control_addr = 0x00

size_r = 32
size_c = 32
arr_size = size_r * size_c
int_size = 4

A_recon_bytes = []
S_recon_bytes = []
U_recon_bytes = []
V_recon_bytes = []


def write_matrices():
    print("Writing 1s to A")
    write_data(A_addr,bytearray([0x01, 0x02, 0x03, 0x04] * arr_size))

    print("Writing 1s to S")
    write_data(S_addr,bytearray([0x01, 0x00, 0x00, 0x00] * arr_size))

    print("Writing 1s to U")
    write_data(U_addr,bytearray([0x01, 0x00, 0x00, 0x00] * arr_size))

    print("Writing 1s to V")
    write_data(V_addr,bytearray([0x01, 0x00, 0x00, 0x00] * arr_size))

def write_addresses():
    print("Writing address")
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
    print("Starting execution")
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
    print("Reading results")
    print('A')
    A_recon_bytes = read_data(A_addr, int_size*arr_size)

    print('S')
    S_recon_bytes = read_data(S_addr, int_size*arr_size)

    print('U')
    U_recon_bytes = read_data(U_addr, int_size*arr_size)

    print('V')
    V_recon_bytes = read_data(V_addr, int_size*arr_size)

    print("done write/read ")


def check_results():
    print("Checking results")
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
    print(a)
    print(s)
    print(u)
    print(v)

    

write_matrices()
write_addresses()
start_execution()
wait_for_done()
read_results()



