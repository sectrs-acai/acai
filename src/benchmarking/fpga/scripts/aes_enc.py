#Works for using_axi_master with encryption and decryption
#The corresponding vivado project is /home/ubuntu/arm-cca/gitlab/fpga/using_axi_master
#The bitstream that works is /home/ubuntu/Xilinx/Vivado2022/example_memory_enc_dec/ in *aes_cbc and *aes_ctr folders
#This was tested with aes cbc 128 algo. Also works with aes ctr 128 algo. 


import os
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
    print(res)
    return res



data_addr = 0x80000000
crypto_addr = 0xA0000000
arr_size = 128
int_size = 4

print("Writing 1s to crypto area")
read_data(crypto_addr, int_size*arr_size)
write_data(crypto_addr,bytearray([0x01, 0x00, 0x00, 0x00] * arr_size))
read_data(crypto_addr, int_size*arr_size)

print("Writing 1s")
read_data(data_addr, int_size*arr_size)
write_data(data_addr,bytearray([0x01, 0x00, 0x00, 0x00] * arr_size))
read_data(data_addr, int_size*arr_size)

print("Starting execution")
control_addr = 0x00
read_data(control_addr, 1)
write_data(control_addr,bytearray([int('0b00000001',2)]))

print("Wait for done")
while int.from_bytes(read_data(control_addr, 1),'big') & int('0b00000010',2) == 0:
    print("Waiting")

print("Done")
print("Reading results")
res = read_data(data_addr, int_size*arr_size)
res_crypto = read_data(crypto_addr, int_size*arr_size)
arr = [1]*arr_size
expected_val = [x+100 if i%2==0 else x for i,x in enumerate(arr)]

correct = 1
print("Checking results")
for i in range(0,arr_size):
    if res[i*int_size] == expected_val[i]:
        None
    else:
        correct = 0
        print("Did not match ", i, res[i*int_size], expected_val[i])
        break

if correct == 0:
    print("Test failed")
else:
    print("Test passed")