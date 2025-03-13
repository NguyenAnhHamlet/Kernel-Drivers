obj-m += usbkeyboard.o

export CROSS_COMPILE=aarch64-linux-gnu-
export ARCH=arm64
 
KDIR = /lib/modules/$(shell uname -r)/build

all:
	make -C $(KDIR) M=$(shell pwd) modules
 
clean:
	make -C $(KDIR) M=$(shell pwd) clean

install: 
	sudo cp usbkeyboard.ko /lib/modules/$(shell uname -r)/kernel/drivers/

load:
	sudo insmod usbkeyboard.ko

unload:
	sudo rmmod usbkeyboard

clear:
	sudo dmesg -C

log: 
	sudo dmesg 
