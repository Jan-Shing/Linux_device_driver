obj-m := timer_interval.o

all:
	make ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- -C ../../linux-3.10 M=`pwd` modules

clean:
	make ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- -C ../../linux-3.10  M=`pwd` clean
	
