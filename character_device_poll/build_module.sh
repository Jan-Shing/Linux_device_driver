#!/bin/sh

cp *.ko  ~/raspberry/busybox/busybox-1.21.1/_install/driver
cp poll  ~/raspberry/busybox/busybox-1.21.1/_install/driver
cp helloarm  ~/raspberry/busybox/busybox-1.21.1/_install/driver
cd  ~/raspberry/busybox/busybox-1.21.1/_install/
find . | cpio -o --format=newc > ~/raspberry/rootfs.img

