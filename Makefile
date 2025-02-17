obj-m += dht11.o
 
KDIR = /lib/modules/$(shell uname -r)/build

all:
	make -C $(KDIR) M=$(shell pwd) modules
	dtc -@ -I dts -O dtb -o dht11.dtbo dht11.dts
 
clean:
	make -C $(KDIR) M=$(shell pwd) clean

install: 
	sudo cp dht11.dtbo /boot/firmware/overlays
	sudo cp dht11.ko /lib/modules/$(shell uname -r)/kernel/drivers/

load:
	sudo insmod dht11.ko

unload:
	sudo rmmod dht11
