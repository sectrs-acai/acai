DRIVER=nouveau_stub

ifneq ($(KERNELRELEASE),)
obj-m	:= $(DRIVER).o
else
ifndef LINUX_DIR
LINUX_DIR	:= /lib/modules/$(shell uname -r)/build
endif
PWD	:= $(shell pwd)

default:
	$(MAKE)	-C $(LINUX_DIR) M=$(PWD) ENABLE_WARN_DEPRECATED=1 modules
endif

clean:
	rm -f *~ *.o *.ko
	rm -f .built_in.o.cmd built_in.o
	rm -f Module.symvers modules.order
	rm -f .*.cmd *.ko *.mod.c
	rm -rf .tmp_versions

install: default
	sudo cp $(DRIVER).ko /lib/modules/$(shell uname -r)/kernel
	sudo depmod -a

uninstall:
	sudo rm -rf /lib/modules/$(shell uname -r)/kernel/$(DRIVER).ko
	sudo depmod -a

all: default
.PHONY: clean uninstall all