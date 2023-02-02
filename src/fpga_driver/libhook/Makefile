SHELL := /bin/bash

.PHONY: all clean

CFLAGS += -ggdb3 -O2 -Wno-unused-function -Wno-unused-result -Wno-unused-function
LIBS += -ldl -fPIC -shared

env_script := $(shell git rev-parse --show-toplevel)
asset_dir := $(env_script)/assets/fvp/bin

libc_version = $(shell ldd --version | head -n 1 | awk -F " " '{print $$NF}' )
output = libhook-libc-$(libc_version).so

all: libhook.so

.PHONY: libhook.so

#
# On cluster we use ubuntu 20.04 which ships with old version of libc
#
libhook.so: hook.c
	@echo Creating $(output)
	$(CC) -o $(output) $^ $(LIBS)
	cp $(output) $(asset_dir)


clean:
	rm -f *.o *.so

