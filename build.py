#!/usr/bin/env python3

import platform
import shutil
import subprocess
import os, sys,re, requests
from typing import Literal

if os.environ.get("CI") == "true":
    ALPINE_MIRROR="http://dl-cdn.alpinelinux.org/alpine"
    DEBIAN_MIRROR="http://deb.debian.org/debian"
else:
    ALPINE_MIRROR="http://mirrors.ustc.edu.cn/alpine"
    DEBIAN_MIRROR="http://mirrors.ustc.edu.cn/debian"

ALPINE_VERSION="latest-stable"
DEBIAN_VERSION="sid"

def fetch_index(arch: Literal['x86_64', 'aarch64', 'riscv64', 'loongarch64']) -> str:
    os.makedirs(f"download/{arch}", exist_ok=True)

    req = requests.get(f"{ALPINE_MIRROR}/{ALPINE_VERSION}/main/{arch}/APKINDEX.tar.gz")
    req.raise_for_status()
    with open(f"download/{arch}/APKINDEX.tar.gz", "wb") as f:
        for chunk in req.iter_content(chunk_size=8192):
            f.write(chunk)
    
    os.system(f"tar -xf download/{arch}/APKINDEX.tar.gz -C download/{arch} APKINDEX")

    debian_arch = {
        "x86_64": "amd64",
        "aarch64": "arm64",
        "riscv64": "riscv64",
        "loongarch64": "loongarch64"
    }[arch]
    req = requests.get(f"{DEBIAN_MIRROR}/dists/{DEBIAN_VERSION}/main/binary-{debian_arch}/Packages.xz")
    req.raise_for_status()
    with open(f"download/{arch}/Packages.xz", "wb") as f:
        for chunk in req.iter_content(chunk_size=8192):
            f.write(chunk)
    
    os.system(f"xz -d download/{arch}/Packages.xz")


def determine_apk(arch: Literal['x86_64', 'aarch64', 'riscv64', 'loongarch64'], pkg: str) -> str:
    with open(f"download/{arch}/APKINDEX", "r") as f:
        for line in f:
            if line.startswith(f"P:{pkg}"):
                ver = None
                while True:
                    l = f.readline()
                    if l.startswith("V:"):
                        ver = l.split(":", 1)[1].strip()
                        break
                    if l == "\n":
                        break
                return f"{pkg}-{ver}.apk"

def determine_deb(arch: Literal['x86_64', 'aarch64', 'riscv64', 'loongarch64'], pkg: str) -> str:
    debian_arch = {
        "x86_64": "amd64",
        "aarch64": "arm64",
        "riscv64": "riscv64",
        "loongarch64": "loongarch64"
    }[arch]
    with open(f"download/{arch}/Packages", "r") as f:
        for line in f:
            if line == f"Package: {pkg}\n":
                ver = None
                while True:
                    l = f.readline()
                    if l.startswith("Version: "):
                        ver = l.split(":", 1)[1].strip()
                        break
                    if l == "\n":
                        break
                if ver is None:
                    raise ValueError(f"Invalid Packages: {pkg}")
                return f"{pkg}_{ver}_{debian_arch}.deb"

# def determine_linux(arch: Literal['x86_64', 'aarch64', 'riscv64', 'loongarch64']) -> str:
#     debian_arch = {
#         "x86_64": "amd64",
#         "aarch64": "arm64",
#         "riscv64": "riscv64",
#         "loongarch64": "loongarch64"
#     }[arch]
#     linux_req_re = re.compile(r"(?P<pkgname>linux-image-\d+\.\d+\.\d+-\d+-" + debian_arch + r") \(= (?P<ver>.+)\)")

#     with open(f"download/{arch}/Packages", "r") as f:
#         for line in f:
#             if line == f"Package: linux-image-{debian_arch}\n":
#                 depends = None
#                 while True:
#                     l = f.readline()
#                     if l.startswith("Depends: "):
#                         depends = l.split(":", 1)[1].strip()
#                         break
#                     if l == "\n":
#                         break
    
#     for depend in depends.split(","):
#         if m := linux_req_re.match(depend.strip()):
#             return f"{m['pkgname']}_{m['ver']}_{debian_arch}.deb"

def determine_linux_url(arch: Literal['x86_64', 'aarch64', 'riscv64', 'loongarch64']) -> str:
    debian_arch = {
        "x86_64": "amd64",
        "aarch64": "arm64",
        "riscv64": "riscv64",
        "loongarch64": "loongarch64"
    }[arch]
    linux_req_re = re.compile(r"(?P<pkgname>linux-image-\d+\.\d+\.\d+(?:-\d+)*-" + debian_arch + r") \(= (?P<ver>.+)\)")

    with open(f"download/{arch}/Packages", "r") as f:
        for line in f:
            if line == f"Package: linux-image-{debian_arch}\n":
                depends = None
                while True:
                    l = f.readline()
                    if l.startswith("Depends: "):
                        depends = l.split(":", 1)[1].strip()
                        break
                    if l == "\n":
                        break

        real_pkg = None
        for depend in depends.split(","):
            print(depend)
            if m := linux_req_re.match(depend.strip()):
                real_pkg = m['pkgname']
        
        if not real_pkg:
            raise ValueError("Failed to determine linux-image package name")
        
        f.seek(0)
        for line in f:
            if line == f"Package: {real_pkg}\n":
                while True:
                    l = f.readline()
                    if l.startswith("Filename: "):
                        filename = l.split(":", 1)[1].strip()
                        return f"{DEBIAN_MIRROR}/{filename}"
                    if l == "\n":
                        break
        raise ValueError("Failed to determine linux-image package URL")


if __name__ == "__main__":
    arch = sys.argv[1]
    # fetch_index(arch)
    APK_TOOLS_APK = determine_apk(arch, "apk-tools-static")
    LINUX_URL = determine_linux_url(arch)
    EFISTUB_DEB = determine_deb(arch, "systemd-boot-efi")
    if not APK_TOOLS_APK or not LINUX_URL or not EFISTUB_DEB:
        raise ValueError("Failed to determine apk-tools-static or linux-image or systemd-boot-efi")
    CROSS_COMPILE = ""
    if arch != platform.machine():
        # we are cross-compiling
        if arch == "aarch64":
            guess = [
                'aarch64-linux-gnu-',
                'aarch64-linux-musl-',
            ]
        elif arch == "riscv64":
            guess = [
                'riscv64-linux-gnu-',
                'riscv64-linux-musl-',
            ]
        elif arch == "loongarch64":
            guess = [
                'loongarch64-linux-gnu-',
                'loongarch64-linux-musl-',
            ]
        elif arch == "x86_64":
            guess = [
                'x86_64-linux-gnu-',
                'x86_64-linux-musl-',
            ]
        else:
            raise ValueError(f"Unknown arch: {arch}")
        for g in guess:
            if shutil.which(g + 'gcc'):
                CROSS_COMPILE = g
                break
    
    make_cmd = [
        "make",
        f"ARCH={arch}",
        f"CROSS_COMPILE={CROSS_COMPILE}",
        f"APK_TOOLS_APK={APK_TOOLS_APK}",
        f"LINUX_URL={LINUX_URL}",
        f"EFISTUB_DEB={EFISTUB_DEB}",
        f"ALPINE_MIRROR={ALPINE_MIRROR}",
        f"ALPINE_VERSION={ALPINE_VERSION}",
        f"DEBIAN_MIRROR={DEBIAN_MIRROR}",
    ]
    print(f"Running: {' '.join(make_cmd)}")

    # clean
    subprocess.run([*make_cmd, "clean"])
    # build
    subprocess.run([*make_cmd, "all"])
