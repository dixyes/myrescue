#!/usr/bin/env bash

set -eo pipefail

objcopy \
      --add-section .osrel=/etc/os-release --change-section-vma .osrel=0x20000 \
      --add-section .cmdline="cmdline" --change-section-vma .cmdline=0x30000 \
      --add-section .linux="vmlinuz" --change-section-vma .linux=0x40000 \
      --add-section .initrd="myinitramfs.img" --change-section-vma .initrd=0x3000000 \
      /usr/lib/systemd/boot/efi/linuxx64.efi.stub rescue.efi
