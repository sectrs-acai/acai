LINUX_DIR ?= /lib/modules/$(shell uname -r)/build
obj-m += devmem_intercept.o
obj-m += devmem_undelegate.o

ccflags-y := -DDEBUG -g -std=gnu99 -Wno-declaration-after-statement -Wframe-larger-than=1000000000 -Wno-unused-result -Wno-format -Wno-unused-variable

.PHONY: all clean

all:
	echo $(LINUX_DIR)
	$(MAKE) V=1 -C '$(LINUX_DIR)' M='$(PWD)' modules

clean:
	$(MAKE) -C '$(LINUX_DIR)' M='$(PWD)' clean
