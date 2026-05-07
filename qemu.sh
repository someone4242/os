#!/bin/sh
set -e
. ./iso.sh

qemu-system-$(./target-triplet-to-arch.sh $HOST) myos.iso -audiodev pa,id=speaker -machine pcspk-audiodev=speaker
