SHELL := /bin/bash

.PHONY: all clean

CFLAGS +=
CFLAGS_EXTRA += -ggdb3 -O0 -Wno-unused-function -Wno-unused-result -Wno-unused-function
IN_EXT ?= .c
OUT_EXT ?= .out
LIBS += -lpthread -lptedit -lscanmem

#OUTS := $(addsuffix $(OUT_EXT), $(basename $(wildcard *$(IN_EXT))))
OUTS = host_fork_test.out host_fvp_test.out


host_fork_test.out: host_fork_test.c pagescan.c
	$(CC) $(CFLAGS) $(CFLAGS_EXTRA) $(LIBS) -o host_fork_test.out host_fork_test.c pagescan.c

#%$(OUT_EXT): %$(IN_EXT)
#	$(CC) $(CFLAGS) $(CFLAGS_EXTRA) $(LIBS) -o '$@' '$<'


host_fvp_test.out: host_fvp_test.c pagescan.c
	$(CC) $(CFLAGS) $(CFLAGS_EXTRA) $(LIBS) -o host_fvp_test.out host_fvp_test.c  pagescan.c

#all:

all: $(OUTS)

clean:
	rm -f *'$(OUT_EXT)'
