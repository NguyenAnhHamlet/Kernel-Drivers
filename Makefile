obj-m += dht11.o
 
KDIR = /lib/modules/$(shell uname -r)/build

all:
	make -C $(KDIR) M=$(shell pwd) modules
	dtc -@ -I dts -O dtb -o dht11.dtbo dht11.dts
 
clean:
	make -C $(KDIR) M=$(shell pwd) clean

install: 
	sudo cp dht11.dtb /boot/firmware/overlays
	sudo mkdir -p /lib/modules/$(shell uname -r)/kernel/drivers/dht11
	sudo cp dht11.ko /lib/modules/$(shell uname -r)/kernel/drivers/dht11

load:
	sudo modprobe dht11
