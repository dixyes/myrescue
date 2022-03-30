#!/usr/bin/env bash

set -eo pipefail

ppwd="$(pwd)"

# make root
mkdir -p root

# bootstrap with pacstrap
pacstrap -i root busybox filesystem vim kmod usbutils pciutils util-linux

# make busybox aliases
cd root
    cd bin
        "${ppwd}/mkalias.sh"
    cd ..

# copy modules
    mkdir -p "usr/lib/modules/$(uname -r)/"
    cp "/lib/modules/$(uname -r)"/modules* "usr/lib/modules/$(uname -r)/"
    "${ppwd}/walkmod.sh" "${ppwd}/modlist" | while read -r modfile
    do
        relpath="usr/lib/modules/$(uname -r)/${modfile##*/lib/modules/"$(uname -r)"/}"
        mkdir -p "${relpath%/*}"
        cp "$modfile" "$relpath"
    done
    cp -r "/lib/modules/$(uname -r)/vmlinuz" ../vmlinuz

    # cleanup pacman
    rm -rf var/cache/pacman

cd ..

# copy init
cp init.sh root/init
sed -i 's/!mods!/'"$(tr '\n' ' ' < "${ppwd}/modlist")"'/g' root/init

# make initrd
cd root
    find . -print0 | cpio --null --create --format=newc | zstd > ../myinitramfs.img
cd ..

