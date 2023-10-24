LINUX_DIR ?= /lib/modules/$(shell uname -r)/build
obj-m += sample.o

ccflags-y := -DDEBUG -g -std=gnu99 -Wno-declaration-after-statement -Wframe-larger-than=1000000000 -Wno-unused-result

.PHONY: all clean

all:
	$(MAKE) V=1 -C '$(LINUX_DIR)' M='$(PWD)' modules

clean:
	$(MAKE) -C '$(LINUX_DIR)' M='$(PWD)' clean
