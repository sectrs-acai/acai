ccflags-y += -Wno-unused-result

ifneq ($(KERNELRELEASE),)
obj-m	:= fh_fixup_mod.o
else
ifndef LINUX_DIR
LINUX_DIR	:= /lib/modules/$(shell uname -r)/build
endif
PWD	:= $(shell pwd)
endif

all:
	$(MAKE) -C $(LINUX_DIR) M=$(PWD) ENABLE_WARN_DEPRECATED=1 modules
clean:
	$(MAKE) -C $(LINUX_DIR) M=$(PWD) clean
	rm -f *~ *.o *.ko
	rm -f .built_in.o.cmd built_in.o
	rm -f Module.symvers modules.order
	rm -f .*.cmd *.ko *.mod.c
	rm -rf .tmp_versions

install:
	sudo cp fh_fixup_mod.ko $(LINUX_DIR)/../kernel
	sudo depmod -a

uninstall:
	sudo rm -rf $(LINUX_DIR)/../kernel/fh_fixup_mod.ko
	sudo depmod -a