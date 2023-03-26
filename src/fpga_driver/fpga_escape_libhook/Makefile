SHELL := /bin/bash

.PHONY: all clean fpga_usr_manager

CFLAGS += -ggdb3 -O2 -Wno-unused-function -Wno-unused-result -Wno-unused-function -static
INC += -I../fh_host
LIBS += -lpthread  -lptedit
all: fpga_usr_manager

fpga_usr_manager: usr_manager.c fpga_comm.c
	$(CC) $(CFLAGS) $(INC) -o $@ $^ $(LIBS)

clean:
	rm -f *.o ptedit/*.o fpga_manager_test fpga_manager heap_target hooks fpga_libhook_manager fpga_usr_manager

