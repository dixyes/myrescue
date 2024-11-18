
ARCH := $(shell uname -m)
HOST_ARCH := $(shell uname -m)

ifeq ($(ARCH), "x86_64")
	EFI_SUFFIX := x64
else ifeq ($(ARCH), "aarch64")
	EFI_SUFFIX := aa32
else ifeq ($(ARCH), "riscv64")
	EFI_SUFFIX := rv64
else ifeq ($(ARCH), "loongarch64")
	EFI_SUFFIX := la64
endif

ALPINE_MIRROR := https://mirrors.ustc.edu.cn/alpine
ALPINE_VERSION := v3.20
APK_TOOLS_APK := apk-tools-static-2.14.4-r4.apk
APK_TOOLS_URL := $(ALPINE_MIRROR)/$(ALPINE_VERSION)/main/$(HOST_ARCH)/$(APK_TOOLS_APK)
BUSYBOX_APK := busybox-1.37.0-r7.apk
BUSYBOX_URL := $(ALPINE_MIRROR)/$(ALPINE_VERSION)/main/$(ARCH)/$(BUSYBOX_APK)
PACKAGES := \
	eudev \
	vim \
	coreutils \
	binutils \
	usbutils \
	pciutils \
	findutils \
	util-linux \
	bind-tools \
	kmod \
	zstd \
	gzip \
	tar \
	lz4 \
	xz \
	gptfdisk \
	e2fsprogs \
	e2fsprogs-extra \
	xfsprogs \
	xfsprogs-extra \
	lvm2 \
	ntfs-3g \
	ntfs-3g-progs \
	dosfstools \
	cdrkit \
	udftools \
	arch-install-scripts 

all: myrescue$(EFI_SUFFIX).efi

download: download/$(APK_TOOLS_APK) download/$(ARCH)/$(BUSYBOX_APK)

download/$(APK_TOOLS_APK):
	mkdir -p download
	curl -fSL -o $@ $(APK_TOOLS_URL)

download/$(ARCH)/$(BUSYBOX_APK):
	mkdir -p download/$(ARCH)
	curl -fSL -o $@ $(BUSYBOX_URL)

download/sbin/apk.static: download/$(APK_TOOLS_APK)
	tar -C download -xf download/$(APK_TOOLS_APK) sbin/apk.static

root: download/sbin/apk.static
	# install packages
	download/sbin/apk.static \
		--arch x86_64 \
		-X $(ALPINE_MIRROR)/$(ALPINE_VERSION)/main/ \
		-X $(ALPINE_MIRROR)/$(ALPINE_VERSION)/community/ \
		-U \
		--allow-untrusted \
		--root root \
		--initdb \
		add $(PACKAGES) || :

init: root
	make -C init CC=$(CC) ARCH=$(ARCH)

initramfs.img: root init
	cp init/init root/init
	cp init.pre.sh init.shell.sh root/
	sed -i 's/!mods!/'"$(sed 's/#.\+//g' "${ppwd}/modlist" | tr '\n' ' ')"'/g' root/init.pre.sh
	cd root && \
    	find . -print0 | cpio --null --create --format=newc | zstd > ../initramfs.img

.PHONY: download root init
