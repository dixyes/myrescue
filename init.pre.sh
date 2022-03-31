#!/bin/busybox sh

for mod in !mods!
do
    /bin/modprobe "$mod"
done

sleep 1

mkdir -p /proc /sys /dev /run /tmp
mount -t proc -o nosuid,nodev,noexec proc /proc
mount -t sysfs -o nosuid,nodev,noexec sys /sys
mount -t tmpfs -o nosuid,nodev run /run
mount -t devtmpfs dev /dev
mkdir -p /dev/shm
mount -t tmpfs -o nosuid,nodev tmpfs /dev/shm
mount -t tmpfs -o nosuid,nodev tmpfs /tmp

if [ -f /proc/sys/kernel/hotplug ]
then
    echo '/bin/mdev' > /proc/sys/kernel/hotplug
else
    /bin/udevd --daemon --resolve-names=never
fi

udevadm trigger
udevadm settle
