#!/bin/bash

# For UEFI
# qemu-system-x86_64 -bios /usr/share/ovmf/OVMF.fd
#    -smp 1,cores=4,threads=2,sockets=1,maxcpus=8 \
# This is stupid. But true.
objcopy -I elf64-x86-64 -O elf32-i386 build/amd64/bin/kernel build/amd64/bin/kernel.i386
#qemu-system-x86_64 -machine q35 --cpu Skylake-Client -enable-kvm \
qemu-system-x86_64 -machine q35 --cpu host,-x2apic -enable-kvm \
    -global hpet.msi=true \
    -smp cores=8,sockets=1 \
    -vga none \
    -m 4G \
    -device bochs-display,bus=pcie.0 \
    -device pcie-pci-bridge,id=pci,bus=pcie.0 \
    -device intel-hda,bus=pci,addr=1 -device hda-duplex \
    -device qemu-xhci,bus=pcie.0 \
    -device usb-kbd -device usb-mouse \
    -d trace:*nvme* \
    -debugcon stdio \
    -d trace:usb_xhci_* \
    -kernel build/amd64/bin/kernel.i386
# -nic tap,ipv6=on,ipv4=off,model=e1000,mac=52:54:98:76:54:32

#    -device nvme,drive=D22,serial=1234 \
#    -drive file=disk.img,if=none,id=D22,driver=file \
#    -device sdhci-pci --device sd-card \
#    -device virtio-gpu,bus=pcie.0 \

