ARCH = arm64
kernel_dir = /home/yf/linux/tspi/kernel

pwd = $(shell pwd)

all:
	make -C $(kernel_dir) modules M=$(pwd) 
clean:
	make -C $(kernel_dir) clean M=$(pwd)

obj-m += platform_device.o 
