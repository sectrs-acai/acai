SHELL := /bin/bash

.PHONY: all clean

CFLAGS += -ggdb3 -O2 -Wno-unused-function -Wno-unused-result -Wno-unused-function -static
LIBS += -lpthread -lscanmem -lptedit

all: header fh_host.o

header: fh_host.c fh_host.h
	echo "#pragma once" > fh_host_header.h
	cat ptedit_header.h fh_host.h fh_host.c | \
	sed -e 's/#include ".*"//g' -e "1i // Warning: this file was generated by make. DO NOT EDIT!" | sed 's/#define fh_host_fn/#define fh_host_fn static/g' >> fh_host_header.h

clean:
	rm -f *.o ptedit/*.o fpga_manager_test fpga_manager heap_target hooks

