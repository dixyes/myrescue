#!/usr/bin/env bash

set -eo pipefail

ppwd="$(pwd)"

# make root
mkdir -p root root/usr/lib root/usr/bin
cd root
    ln -s usr/lib lib || :
    ln -s usr/lib lib64 || :
    ln -s usr/bin bin || :
    ln -s usr/bin sbin || :
cd ..

# bootstrap with apk static
download/sbin/apk.static \
    --arch x86_64 \
    -X https://mirrors.ustc.edu.cn/alpine/edge/main/ \
    -X https://mirrors.ustc.edu.cn/alpine/edge/community/ \
    -U \
    --allow-untrusted \
    --root root \
    --initdb add \
        eudev \
        vim \
        usbutils \
        pciutils \
        findutils \
        util-linux \
        kmod \
        zstd \
        gzip \
        tar \
        lz4 \
        xz \
        e2fsprogs \
        gptfdisk \
        e2fsprogs \
        xfsprogs \
        lvm2 \
        ntfs-3g \
        ntfs-3g-progs \
        dosfstools \
        download/glibc*.apk

# for usr/lib/libxx symlinks
symlink_pkgs=(libcrypto1.1 libssl1.1 eudev-libs)
for pkg in "${symlink_pkgs[@]}"
do
    rm "download/${pkg}"*.apk || :
done

download/sbin/apk.static \
    --arch x86_64 \
    -X https://mirrors.ustc.edu.cn/alpine/edge/main/ \
    -X https://mirrors.ustc.edu.cn/alpine/edge/community/ \
    -U \
    --allow-untrusted \
    --root root \
    fetch -o download "${symlink_pkgs[@]}"

for pkg in "${symlink_pkgs[@]}"
do
    tar -C root -hxf download/"$pkg"*.apk --exclude 'usr/lib/lib*.so*' --exclude '.*' 2>/dev/null
done

# for bin/xx symlinks
symlink_pkgs=(tar eudev gzip)
for pkg in "${symlink_pkgs[@]}"
do
    rm "download/${pkg}"*.apk || :
done

download/sbin/apk.static \
    --arch x86_64 \
    -X https://mirrors.ustc.edu.cn/alpine/edge/main/ \
    -X https://mirrors.ustc.edu.cn/alpine/edge/community/ \
    -U \
    --allow-untrusted \
    --root root \
    fetch -o download "${symlink_pkgs[@]}"

for pkg in "${symlink_pkgs[@]}"
do
    badlinks="$(tar -C root -tvf download/"$pkg"*.apk 2>/dev/null | grep -Pe '^lrwxrwxrwx.+(usr/bin|sbin)/(.+)\s+->\s+/bin/\2' | sed -E 's#.+((usr/bin|sbin)/.+)\s+->.+#\1#g')" || :
    tar_args=()
    for badlink in $badlinks
    do
        tar_args+=(--exclude "$badlink")
    done
    tar -C root -hxf download/"$pkg"*.apk --exclude 'usr/lib/lib*.so*' --exclude '.*' "${tar_args[@]}" 2>/dev/null
done

# make busybox aliases
cp download/usr/bin/busybox root/bin/
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

# rm cache
    rm -r var/cache/apk
cd ..

# copy init
cp init init.pre.sh init.shell.sh root/
sed -i 's/!mods!/'"$(sed 's/#.\+//g' "${ppwd}/modlist" | tr '\n' ' ')"'/g' root/init.pre.sh

# make initrd
cd root
    find . -print0 | cpio --null --create --format=newc | zstd > ../myinitramfs.img
cd ..

