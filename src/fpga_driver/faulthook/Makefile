SHELL := /bin/bash

.PHONY: all clean

CFLAGS += -ggdb3 -O2 -Wno-unused-function -Wno-unused-result -Wno-unused-function -static
LIBS += -lpthread -lscanmem -lptedit

all: fpga_manager

fpga_manager:
	$(CC) $(CFLAGS) fpga_manager.c  -o fpga_manager.o  $(LIBS)


clean:
	rm -f *.o ptedit/*.o
