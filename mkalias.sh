#!/bin/sh

for applet in $(./busybox --list)
do
    echo "linking $applet"
    ln -s busybox "$applet" 2>/tmp/mkalias.log
done

exit 0
