SHELL := /bin/bash

.PHONY: all clean

CFLAGS += -ggdb3 -O0 -Wno-unused-function -Wno-unused-result -Wno-unused-function -static
LIBS += -lpthread -lscanmem -lptedit

all: host_fork_test host_fvp_test host_fvp_test_kernel

host_fork_test:
	$(CC) $(CFLAGS) host_fork_test.c pagescan.c -o host_fork_test.o  $(LIBS)

host_fvp_test:
	$(CC) $(CFLAGS) host_fvp_test.c pagescan.c -o host_fvp_test.o $(LIBS)

host_fvp_test_kernel:
	$(CC) $(CFLAGS) host_fvp_test_kernel.c pagescan.c -o host_fvp_test_kernel.o $(LIBS)


clean:
	rm -f *.o ptedit/*.o
