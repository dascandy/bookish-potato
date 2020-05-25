#!/bin/bash

llvm-objcopy -O binary build/rpi4/bin/kernel rpikernel.img
raspbootcom /dev/ttyUSB0 rpikernel.img

