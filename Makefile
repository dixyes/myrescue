
ARCH := $(shell uname -m)
HOST_ARCH := $(shell uname -m)
CROSS_COMPILE :=

EFI_SUFFIX :=
ifeq ($(ARCH), "x86_64")
	EFI_SUFFIX = x64
else ifeq ($(ARCH), "aarch64")
	EFI_SUFFIX = aa64
else ifeq ($(ARCH), "riscv64")
	EFI_SUFFIX = rv64
else ifeq ($(ARCH), "loongarch64")
	EFI_SUFFIX = la64
else
	ERR = $(error Unsupported architecture: $(ARCH))
endif

ALPINE_MIRROR := https://mirrors.ustc.edu.cn/alpine
ALPINE_VERSION := v3.20
APK_TOOLS_APK := apk-tools-static-2.14.4-r4.apk
APK_TOOLS_URL := $(ALPINE_MIRROR)/$(ALPINE_VERSION)/main/$(HOST_ARCH)/$(APK_TOOLS_APK)
BUSYBOX_APK := busybox-1.37.0-r7.apk
BUSYBOX_URL := $(ALPINE_MIRROR)/$(ALPINE_VERSION)/main/$(ARCH)/$(BUSYBOX_APK)
LINUX_APK := linux-lts-6.6.61-r0.apk
LINUX_URL := $(ALPINE_MIRROR)/$(ALPINE_VERSION)/main/$(ARCH)/$(LINUX_APK)
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

download/$(APK_TOOLS_APK):
	mkdir -p download
	curl -fSL -o $@ $(APK_TOOLS_URL)

# download/$(ARCH)/$(BUSYBOX_APK):
# 	mkdir -p download/$(ARCH)
# 	curl -fSL -o $@ $(BUSYBOX_URL)

download/$(ARCH)/$(LINUX_APK):
	mkdir -p download/$(ARCH)
	curl -fSL -o $@ $(LINUX_URL)

download/sbin/apk.static: download/$(APK_TOOLS_APK)
	tar -m -C download -xf download/$(APK_TOOLS_APK) sbin/apk.static

init/init:
	$(MAKE) -C init CC=$(CROSS_COMPILE)gcc ARCH=$(ARCH)
	$(CROSS_COMPILE)strip -s init/init

root: download/sbin/apk.static init/init
	# extract kernel modules
	mkdir -p root/lib/modules
	tar -m -C root/lib/modules -xf download/$(ARCH)/$(LINUX_APK) lib/modules
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
	# copy files and shell scripts
	cp init/init root/init
	cp init.pre.sh init.shell.sh root/
	sed -i 's/!mods!/'"$(sed 's/#.\+//g' "${ppwd}/modlist" | tr '\n' ' ')"'/g' root/init.pre.sh

initramfs.img: root init/init
	cd root && \
    	find . -print0 | cpio --null --create --format=newc | zstd > ../initramfs.img

gnu-efi_inst:
	$(MAKE) -C gnu-efi \
		CC=$(CROSS_COMPILE)gcc \
		LD=$(CROSS_COMPILE)ld \
		OBJCOPY=$(CROSS_COMPILE)objcopy \
		ARCH=$(ARCH) -j $(shell nproc)
	$(MAKE) -C gnu-efi \
		CC=$(CROSS_COMPILE)gcc \
		LD=$(CROSS_COMPILE)ld \
		OBJCOPY=$(CROSS_COMPILE)objcopy \
		ARCH=$(ARCH) \
		install DESTDIR=../gnu-efi_inst PREFIX=/

stubby.efi: gnu-efi_inst
	$(MAKE) -C stubby \
		CC=$(CROSS_COMPILE)gcc \
		LD=$(CROSS_COMPILE)ld \
		OBJCOPY=$(CROSS_COMPILE)objcopy \
		ARCH=$(ARCH) \
		EFIINC=../gnu-efi_inst/include/efi \
		EFILIB=../gnu-efi_inst/lib \
		-j $(shell nproc) \
		build
	if [ -z "$(CROSS_COMPILE)" ] ; \
	then \
		$(MAKE) -C stubby \
			CC=$(CROSS_COMPILE)gcc \
			LD=$(CROSS_COMPILE)ld \
			OBJCOPY=$(CROSS_COMPILE)objcopy \
			ARCH=$(ARCH) \
			EFIINC=../gnu-efi_inst/include/efi \
			EFILIB=../gnu-efi_inst/lib \
			test ; \
	fi
	cp stubby/stubby.efi .

vmlinuz:
	tar -m --strip-components=1 \
		-xf download/$(ARCH)/$(LINUX_APK) \
		--wildcards \
		boot/vmlinuz-*
	mv vmlinuz-* vmlinuz

myrescue$(EFI_SUFFIX).efi: initramfs.img vmlinuz cmdline stubby.efi
	$(CROSS_COMPILE)objcopy \
		--add-section=.cmdline=cmdline \
		--change-section-vma=.cmdline=0x30000 \
		--add-section=.linux=vmlinuz \
		--change-section-vma=.linux=0x1000000 \
		--add-section=.initrd=initramfs.img \
		--change-section-vma=.initrd=0x3000000 \
		stubby.efi myrescue$(EFI_SUFFIX).efi

clean:
	$(MAKE) -C init CC=$(CROSS_COMPILE)gcc ARCH=$(ARCH) clean
	$(MAKE) -C gnu-efi \
		CC=$(CROSS_COMPILE)gcc \
		LD=$(CROSS_COMPILE)ld \
		OBJCOPY=$(CROSS_COMPILE)objcopy \
		ARCH=$(ARCH) clean
	$(MAKE) -C stubby \
		CC=$(CROSS_COMPILE)gcc \
		LD=$(CROSS_COMPILE)ld \
		OBJCOPY=$(CROSS_COMPILE)objcopy \
		ARCH=$(ARCH) clean
	rm -rf root initramfs.img vmlinuz myrescue$(EFI_SUFFIX).efi

.PHONY: all clean
