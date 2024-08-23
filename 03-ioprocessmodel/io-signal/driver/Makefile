ARCH = arm64

ifeq ($(ARCH), arm64)
	kernel_dir = /home/yf/linux/tspi/kernel
else
	@echo "Invalid ARCH,ARCH must to be arm64"
	exit 1
endif

pwd = $(shell pwd)
# KBUILD_EXTRA_SYMBOLS += /home/linux/TestModules/my_test_module/my_test_moduleB/Module.symvers
all:
	make -C $(kernel_dir) modules M=$(pwd) 
clean:
	make -C $(kernel_dir) clean M=$(pwd)

obj-m += io_signal.o
