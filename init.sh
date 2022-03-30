#!/usr/bin/busybox sh

for mod in !mods!
do
    modprobe "$mod"
done

sleep 1

mount -t proc none /proc
mount -t sysfs none /sys
mount -t devtmpfs none /dev

echo '/bin/mdev' > /proc/sys/kernel/hotplug

cat << EOF

[0;31m__________________  ._________________  ____ ___________._.
\______   \_____  \ |   ____/\_   ___ \|    |   \_____  \ |
 |       _/ _(__  < |____  \ /    \  \/|    |   / _(__  < |
 |    |   \/       \/       \\\\     \___|    |  / /       \|
 |____|_  /______  /______  / \______  /______/ /______  /_
        \/       \/       \/         \/                \/\/[0m

Let's mess the whole world up!

EOF

exec /usr/bin/busybox sh -c 'setsid cttyhack sh'


