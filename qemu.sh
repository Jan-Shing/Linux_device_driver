#!/bin/sh
lib_module_dir="/root/raspberry/busybox/busybox-1.21.1/_install/lib/module"
if [ -d /root/raspberry/busybox/busybox-1.21.1/_install/lib ]
then
	echo "lib/module already exist"
else
	mkdir -p $lib_module_dir
	cp -af /lib/modules/3.10.0 $lib_module_dir
fi
qemu-system-arm -M vexpress-a9 -m 256M -kernel linux-3.10/arch/arm/boot/zImage -initrd rootfs.img -append "root=/dev/ram rdinit=/sbin/init"
