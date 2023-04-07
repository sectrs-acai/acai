SHELL := /bin/bash

.PHONY: all clean gpu_usr_manager

CFLAGS += -ggdb3 -O2 -Wno-unused-function -Wno-unused-result -Wno-unused-function -static
INC += -I./fpga_escape_libhook/../fh_host -I../nouveau_stub/ -I./fpga_escape_libhook/
LIBS += -lpthread  -lptedit
USR_MNGR_BASE := ./fpga_escape_libhook/usr_manager.c

all: gpu_usr_manager

gpu_usr_manager: $(USR_MNGR_BASE) gpu_comm.c
	$(CC) $(CFLAGS) $(INC) -o $@ $^ $(LIBS)

test: test.c
	$(CC) $(CFLAGS) $(INC) -o $@ $^ $(LIBS)

clean:
	rm -f *.o ptedit/*.o gpu_usr_manager

