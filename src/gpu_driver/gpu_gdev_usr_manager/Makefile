SHELL := /bin/bash

.PHONY: all clean gpu_gdev_usr_manager

CFLAGS += -ggdb3 -O2 -Wno-unused-function -Wno-unused-result -Wno-unused-function -Wno-format

INC += -I./fpga_escape_libhook/../fh_host -I./fpga_escape_libhook/ -I./../gdev-guest/mod_stub/gdev/fh_kernel -I./include

USR_MNGR_BASE := ./fpga_escape_libhook/usr_manager.c

#
# unlike fpga manager we cant compile this statically
# because libdrm has no static lib support
#
#
LIBS += -lpthread -lgdev -L$(STAGING_DIR)/usr/local/gdev/lib64

all: gpu_gdev_usr_manager

update_headers:
	cp -rf ./gdev-guest_mod_stub/headers/* include/

gpu_gdev_usr_manager: $(USR_MNGR_BASE) gpu_comm.c
	cp -rf ./gdev-guest_mod_stub/headers/* include/ || true
	$(CC) $(CFLAGS) $(INC) -o $@ $^ $(LIBS)

test: test.c
	$(CC) $(CFLAGS) $(INC) -o $@ $^ $(LIBS)

clean:
	rm -f *.o ptedit/*.o gpu_usr_manager gpu_gdev_usr_manager

