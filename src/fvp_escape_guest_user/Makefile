SHELL := /bin/bash

.PHONY: all clean

CFLAGS_EXTRA ?= -ggdb3 -O0
IN_EXT ?= .c
OUT_EXT ?= .out

OUTS := $(addsuffix $(OUT_EXT), $(basename $(wildcard *$(IN_EXT))))

all: $(OUTS)

%$(OUT_EXT): %$(IN_EXT)
	$(CC) $(CFLAGS) $(CFLAGS_EXTRA) -o '$@' '$<'

clean:
	rm -f *'$(OUT_EXT)'
