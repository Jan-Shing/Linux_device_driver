#!/bin/sh

cp *.ko  ~/raspberry/busybox/busybox-1.21.1/_install/ 
cd  ~/raspberry/busybox/busybox-1.21.1/_install/
find . | cpio -o --format=newc > ~/raspberry/rootfs.img

