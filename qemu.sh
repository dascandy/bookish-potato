#!/bin/bash

# This is stupid. But true.
objcopy -I elf64-x86-64 -O elf32-i386 build/amd64/bin/kernel build/amd64/bin/kernel.i386
qemu-system-x86_64 -machine q35 --cpu Skylake-Client \
    -smp 1,cores=4,threads=2,sockets=1,maxcpus=8 \
    -vga none \
    -m 4G \
    -device bochs-display,bus=pcie.0 \
    -device pcie-pci-bridge,id=pci,bus=pcie.0 \
    -device intel-hda,bus=pci,addr=1 -device hda-duplex \
    -device qemu-xhci,bus=pcie.0 -device usb-kbd -device usb-mouse \
    -drive file=disk.img,if=none,id=D22,driver=file \
    -device nvme,drive=D22,serial=1234 \
    -d trace:usb_xhci_* \
    -debugcon stdio \
    -kernel build/amd64/bin/kernel.i386
# -nic tap,ipv6=on,ipv4=off,model=e1000,mac=52:54:98:76:54:32

#    -device sdhci-pci --device sd-card \
#    -device virtio-gpu,bus=pcie.0 \

