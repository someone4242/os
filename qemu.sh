#!/bin/sh
set -e
. ./iso.sh

qemu-system-$(./target-triplet-to-arch.sh $HOST) -drive format=raw,file=disque_dur -cdrom myos.iso
