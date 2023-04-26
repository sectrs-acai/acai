#
# This makefile must be called from a sourced buildroot environment
# e.g. env-x86.sh in the root of the source distribution
#
SHELL := /bin/bash
.PHONY: all clean gpu_gdev_usr_manager

#
# XXX: We do O0 on purpose so that compiler does not intferere with benchmark code
#
CFLAGS += -ggdb3 -O2 -Wno-unused-function -Wno-unused-result -Wno-unused-function -Wno-format
INC += -I./fpga_escape_libhook/../fh_host -I./fpga_escape_libhook/ -I./../gdev-guest/mod/gdev/ -I./include
USR_MNGR_BASE := ./fpga_escape_libhook/usr_manager.c

#
# unlike fpga manager we cant compile this statically
# because libdrm has no static lib support
#
# XXX: STAGING_DIR is provided by buildroot
LIBS += -lpthread -lgdev -L$(STAGING_DIR)/usr/local/gdev/lib64 -lscanmem -lcrypto -lglib-2.0 $(shell pkg-config --cflags --libs glib-2.0)

all: gpu_gdev_usr_manager

ENCRYPT=1

ifdef ENCRYPT
CFLAGS += -DGPU__GDEV_BUS_ENC
$(warning Compiling with encryption on x86-style approach)
endif

update_headers:
	cp -rf ./gdev-guest/headers/* include/

gpu_gdev_usr_manager: $(USR_MNGR_BASE) gpu_comm.c bench/gpu_bench.c gpu_enc.c
	cp -rf ./gdev-guest/headers/* include/ || true
	$(CC) $(CFLAGS) $(INC) -o $@ $^ $(LIBS)

test: test.c
	$(CC) $(CFLAGS) $(INC) -o $@ $^ $(LIBS)

clean:
	rm -f *.o ptedit/*.o gpu_usr_manager gpu_gdev_usr_manager

