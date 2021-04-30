TARGET = key

KERNELDIR := /home/user/linux/IMX6ULL/linux/linux_nxp_mine

CURRENT_PATH := $(shell pwd)

obj-m := $(TARGET).o

BUILD_DIR := build

CC := arm-linux-gnueabihf-gcc

all: kernel_modules
	rm -Rf $(BUILD_DIR)/.[dt]* $(BUILD_DIR)/*
	mv .tmp* .$(TARGET)* *.mod.* [Mm]odule* -t $(BUILD_DIR)
	mv *.o $(BUILD_DIR)

kernel_modules: $(BUILD_DIR)
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules

$(BUILD_DIR):
	mkdir build

clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean
	rm -Rf build $(TARGET)app

cp:$(TARGET).ko $(TARGET)app
	sudo cp $(TARGET).ko $(TARGET)app ~/linux/nfs/rootfs/lib/modules/4.1.15 -f

$(TARGET)app : $(TARGET)app.c
	$(CC) -o $(TARGET)app $(TARGET)app.c

#$(TARGET).ko : all
