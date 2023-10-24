
import os
import numpy as np 
import pyaes
from ctypes import *

#
# XXX: set to 1 to show stdout prints
#      set warup and bench iters accordingly
#
print_output = 1
warmup_iter = 1
bench_iter = 1


def do_print(vargs):
    if print_output:
        print(vargs)

# so_file = "/mnt/host/bench.so"
# benchmark = CDLL(so_file)

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

def enc(buff):
    aes = pyaes.AESModeOfOperationCTR(os.urandom(32))
    ciphertext = aes.encrypt(str(buff))
    # print('Encrypted:', binascii.hexlify(ciphertext))
    return ciphertext

def dec(buff):
    aes = pyaes.AESModeOfOperationCTR(os.urandom(32))
    plaintext = aes.decrypt(str(buff))
    # print('Encrypted:', binascii.hexlify(ciphertext))
    return plaintext

size_r = 10
size_c = 10
size = size_r * size_c
buff_enc = []
buff_dec = []

print("size_r:", size_r)
print("size_c:", size_c)
print("size:", size)

def write_address():
    global buff_enc
    do_print("Writing address")
    buff_0 = bytearray([0x00, 0x00, 0x00, 0x00])

    ip_addr_1 = 0x10
    buff_addr = bytearray([0x00, 0x00, 0x00, 0xA0])
    write_data(ip_addr_1,buff_addr)
    ip_addr_2 = 0x14
    write_data(ip_addr_2,buff_0)
    buff_enc = enc(buff_0 + buff_addr)

    ip_addr_1 = 0x1c
    buff_addr = bytearray([0x00, 0x00, 0x00, 0xB0])
    write_data(ip_addr_1,bytearray(buff_addr))
    ip_addr_2 = 0x20
    write_data(ip_addr_2,buff_0)
    buff_enc = enc(buff_0 + buff_addr)

    ip_addr_1 = 0x28
    buff_addr = bytearray([0x00, 0x00, 0x00, 0xC0])
    write_data(ip_addr_1,buff_addr)
    ip_addr_2 = 0x2c
    write_data(ip_addr_2,buff_0)
    buff_enc = enc(buff_0 + buff_addr)

    ip_addr_1 = 0x34
    buff_addr = bytearray([0x00, 0x00, 0x00, 0x80])
    write_data(ip_addr_1,buff_addr)
    ip_addr_2 = 0x38
    write_data(ip_addr_2,buff_0)
    buff_enc = enc(buff_0 + buff_addr)

def h_to_d():
    global buff_enc
    do_print("Writing 1s")
    buff_enc = []
    buff = bytearray([0x01, 0x00, 0x00, 0x00] * size)
    data_addr_1 = 0xA0000000
    write_data(data_addr_1,buff)

    data_addr_2 = 0xB0000000
    write_data(data_addr_2,buff)
    buff_enc = enc(buff)

def start_exec():
    global buff_enc
    do_print("Starting execution")
    control_addr = 0x00
    buff_ctrl = bytearray([int('0b00000001',2)])

    buff_enc = enc(bytearray([int('0b00000001',2)]))
    write_data(control_addr,buff_ctrl)


def wait_for_done():
    global buff_dec
    control_addr = 0x00
    do_print("Wait for done")
    while int.from_bytes(read_data(control_addr, 1),'big') & int('0b00000010',2) == 0:
        buff_dec = dec(bytearray([1]))
        do_print("Waiting")

def d_to_h(verify_res = False):
    global buff_dec
    data_addr_3 = 0xC0000000
    do_print("Done")
    do_print("Reading results")
    res = read_data(data_addr_3, 4*size)
    buff_dec = dec(bytearray(res))

    if verify_res:
        expected_val = np.matmul(np.ones((size_r,size_c)), np.ones((size_r,size_c)))
        correct = 1
        do_print("Checking results")
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

do_benchmark(False)
