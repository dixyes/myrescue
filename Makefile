
ARCH := $(shell uname -m)
HOST_ARCH := $(shell uname -m)
CROSS_COMPILE :=

EFI_SUFFIX :=
DEBIAN_ARCH :=
ifeq ($(ARCH), x86_64)
	EFI_SUFFIX = x64
	DEBIAN_ARCH := amd64
else ifeq ($(ARCH), aarch64)
	EFI_SUFFIX = aa64
	DEBIAN_ARCH := arm64
else ifeq ($(ARCH), riscv64)
	EFI_SUFFIX = rv64
	DEBIAN_ARCH := riscv64
else ifeq ($(ARCH), loongarch64)
	EFI_SUFFIX = la64
	DEBIAN_ARCH := loongarch64
else
	ERR = $(error Unsupported architecture: $(ARCH))
endif

ALPINE_MIRROR := https://mirrors.ustc.edu.cn/alpine
ALPINE_VERSION := v3.20
APK_TOOLS_APK := apk-tools-static-2.14.4-r1.apk
APK_TOOLS_URL := $(ALPINE_MIRROR)/$(ALPINE_VERSION)/main/$(HOST_ARCH)/$(APK_TOOLS_APK)
BUSYBOX_APK := busybox-1.37.0-r7.apk
BUSYBOX_URL := $(ALPINE_MIRROR)/$(ALPINE_VERSION)/main/$(ARCH)/$(BUSYBOX_APK)

DEBIAN_MIRROR := https://mirrors.ustc.edu.cn/debian
LINUX_DEB := linux-image-6.11.9-$(DEBIAN_ARCH)_6.11.9-1_$(DEBIAN_ARCH).deb
LINUX_URL := $(DEBIAN_MIRROR)/pool/main/l/linux-signed-$(DEBIAN_ARCH)/$(LINUX_DEB)
EFISTUB_DEB := systemd-boot-efi_257~rc2-3_$(DEBIAN_ARCH).deb
EFISTUB_URL := $(DEBIAN_MIRROR)/pool/main/s/systemd/$(EFISTUB_DEB)

PACKAGES := \
	eudev \
	vim \
	curl \
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

download/$(ARCH)/$(LINUX_DEB):
	mkdir -p download/$(ARCH)
	curl -fSL -o $@ $(LINUX_URL)

download/$(ARCH)/$(EFISTUB_DEB):
	mkdir -p download/$(ARCH)
	curl -fSL -o $@ $(EFISTUB_URL)

download/$(ARCH)/linux.tar.xz: download/$(ARCH)/$(LINUX_DEB)
	cd download/$(ARCH) && \
	ar -x \
		$(LINUX_DEB) \
		data.tar.xz && \
	mv data.tar.xz linux.tar.xz || \
	rm -f data.tar.xz

download/sbin/apk.static: download/$(APK_TOOLS_APK)
	tar -m -C download -xf download/$(APK_TOOLS_APK) sbin/apk.static

init/init:
	$(MAKE) -C init CC=$(CROSS_COMPILE)gcc ARCH=$(ARCH)
	$(CROSS_COMPILE)strip -s init/init

root: download/sbin/apk.static download/$(ARCH)/linux.tar.xz
	# extract kernel modules
	mkdir -p root/lib/modules
	tar -m \
		-C root \
		--strip-components=2 \
		-xf download/$(ARCH)/linux.tar.xz \
		./usr/lib/modules
	# install packages
	download/sbin/apk.static \
		--arch $(ARCH) \
		-X $(ALPINE_MIRROR)/$(ALPINE_VERSION)/main/ \
		-X $(ALPINE_MIRROR)/$(ALPINE_VERSION)/community/ \
		-U \
		--allow-untrusted \
		--root root \
		--initdb \
		add $(PACKAGES) || :
	# post-installation
	./post_root_install.sh

initramfs.img: root init/init init.pre.sh init.shell.sh modlist
	# copy files and shell scripts
	cp init/init init.pre.sh init.shell.sh root/
	sed -i 's/!mods!/'"$$(sed 's/#.\+//g' "modlist" | tr '\n' ' ')"'/g' root/init.pre.sh
	cd root && \
    	find . -print0 | cpio --null --create --format=newc | zstd > ../initramfs.img

download/$(ARCH)/linux$(EFI_SUFFIX).efi.stub: download/$(ARCH)/$(EFISTUB_DEB)
	ar -x \
		--output download/$(ARCH) \
		download/$(ARCH)/$(EFISTUB_DEB) \
		data.tar.xz
	tar -m -C download/$(ARCH) \
		-xf download/$(ARCH)/data.tar.xz \
		--strip-components=6 \
		./usr/lib/systemd/boot/efi/linux$(EFI_SUFFIX).efi.stub
	rm download/$(ARCH)/data.tar.xz

vmlinuz: download/$(ARCH)/linux.tar.xz
	tar -m --strip-components=2 \
		-xf download/$(ARCH)/linux.tar.xz \
		--wildcards \
		./boot/vmlinuz-*
	mv vmlinuz-* vmlinuz

myrescue$(EFI_SUFFIX).efi: initramfs.img vmlinuz cmdline download/$(ARCH)/linux$(EFI_SUFFIX).efi.stub
	lastsec=$$($(CROSS_COMPILE)objdump -h download/$(ARCH)/linux$(EFI_SUFFIX).efi.stub | tail -2 | head -1) ; \
	lastvmaend=$$( \
		echo $$lastsec | \
			gawk '{printf "0x%x", lshift(rshift(strtonum("0x"$$4) + strtonum("0x"$$3), 12), 12)}'\
	) ; \
	cmdlinevma=$$(echo | gawk '{printf "0x%x", strtonum('"$$lastvmaend"') + 0x30000}') ; \
	linuxvma=$$(echo | gawk '{printf "0x%x", strtonum('"$$lastvmaend"') + 0x1000000}') ; \
	initrdvma=$$(echo | gawk '{printf "0x%x", strtonum('"$$lastvmaend"') + 0x3000000}') ; \
	set -x ; \
	$(CROSS_COMPILE)objcopy \
		--add-section=.cmdline=cmdline \
		--change-section-vma=.cmdline="$$cmdlinevma" \
		--add-section=.linux=vmlinuz \
		--change-section-vma=.linux="$$linuxvma" \
		--add-section=.initrd=initramfs.img \
		--change-section-vma=.initrd="$$initrdvma" \
		download/$(ARCH)/linux$(EFI_SUFFIX).efi.stub myrescue$(EFI_SUFFIX).efi

clean:
	$(MAKE) -C init CC=$(CROSS_COMPILE)gcc ARCH=$(ARCH) clean
	rm -rf root initramfs.img vmlinuz myrescue$(EFI_SUFFIX).efi

.PHONY: all clean
