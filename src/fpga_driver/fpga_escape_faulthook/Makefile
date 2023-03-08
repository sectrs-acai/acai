SHELL := /bin/bash

.PHONY: all clean

CFLAGS += -ggdb3 -O2 -Wno-unused-function -Wno-unused-result -Wno-unused-function -static
LIBS += -lpthread -lscanmem -lptedit
INC += -I../fh_host

all: fpga_manager fpga_manager_test

fpga_manager: fpga_manager.c
	$(CC) $(CFLAGS) $(INC) fpga_manager.c -o fpga_manager.o  $(LIBS)

fpga_manager_test: fpga_manager_test.c
	$(CC) $(CFLAGS) $(INC) -o $@ $<  $(LIBS)

clean:
	rm -f *.o ptedit/*.o fpga_manager_test fpga_manager heap_target hooks