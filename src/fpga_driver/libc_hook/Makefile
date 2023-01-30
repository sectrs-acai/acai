SHELL := /bin/bash

.PHONY: all clean

CFLAGS += -ggdb3 -O2 -Wno-unused-function -Wno-unused-result -Wno-unused-function
LIBS +=

env_script := $(shell git rev-parse --show-toplevel)
asset_dir := $(env_script)/assets/fvp/bin

all: libhook.so

.PHONY: libhook.so
libhook.so: hook.c
	$(CC) -fPIC -shared -o $@ $^
	cp $@ $(asset_dir)


clean:
	rm -f *.o libhook.so

