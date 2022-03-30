#!/usr/bin/env bash

set -eo pipefail

mkdir -p download

[ ! -f download/glibc-2.35-r0.apk ] &&
curl -fSL -o download/glibc-2.35-r0.apk https://github.com/sgerrand/alpine-pkg-glibc/releases/download/2.35-r0/glibc-2.35-r0.apk
[ ! -f download/apk-tools-static-2.12.7-r0.apk ] &&
curl -fSL -o download/apk-tools-static-2.12.7-r0.apk http://mirrors.ustc.edu.cn/alpine/v3.14/main/x86_64/apk-tools-static-2.12.7-r0.apk
tar -C download -xf download/apk-tools-static-*.apk

[ ! -f download/busybox-1.34.1-1-x86_64.pkg.tar.zst ] &&
curl -fSL -o download/busybox-1.34.1-1-x86_64.pkg.tar.zst http://mirrors.ustc.edu.cn/archlinux/community/os/x86_64/busybox-1.34.1-1-x86_64.pkg.tar.zst
tar -C download -xf download/busybox-*.tar.zst


