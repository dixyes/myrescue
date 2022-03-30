#!/bin/sh

for applet in $(./busybox --list)
do
    ln -s busybox "$applet" 2>/tmp/mkalias.log &&
    echo "linking $applet"
done

exit 0
