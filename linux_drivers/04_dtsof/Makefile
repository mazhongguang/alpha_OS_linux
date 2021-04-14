KERNELDIR := /home/user/linux/IMX6ULL/linux/linux_nxp_mine

CURRENT_PATH := $(shell pwd)

obj-m := dtsof.o

build: kernel_modules

kernel_modules:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules
clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean
