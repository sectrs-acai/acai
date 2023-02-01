SHELL := /bin/bash

.PHONY: all clean

CFLAGS += -ggdb3 -O2 -Wno-unused-function -Wno-unused-result -Wno-unused-function -static
INC += -I../fh_host
LIBS += -lpthread  -lptedit
all: fpga_libhook_manager

fpga_libhook_manager: fpga_libhook_manager.c fpga_comm.c
	$(CC) $(CFLAGS) $(INC) -o $@ fpga_libhook_manager.c fpga_comm.c $(LIBS)

clean:
	rm -f *.o ptedit/*.o fpga_manager_test fpga_manager heap_target hooks fpga_libhook_manager

