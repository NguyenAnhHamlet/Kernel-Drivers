obj-m += usbmouse.o

export CROSS_COMPILE=aarch64-linux-gnu-
export ARCH=arm64
 
KDIR = /lib/modules/$(shell uname -r)/build

all:
	make -C $(KDIR) M=$(shell pwd) modules
 
clean:
	make -C $(KDIR) M=$(shell pwd) clean

install: 
	sudo cp dht11.dtbo /boot/firmware/overlays
	sudo cp dht11.ko /lib/modules/$(shell uname -r)/kernel/drivers/

load:
	#sudo dtoverlay dht11.dtbo
	sudo insmod usbmouse.ko

unload:
	sudo rmmod usbmouse

clear:
	sudo dmesg -C

log: 
	sudo dmesg 
