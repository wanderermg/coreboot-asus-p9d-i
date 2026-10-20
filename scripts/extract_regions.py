#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Extract private Descriptor and ME/SPS from the owner's full SPI backup. Never flash."""
from pathlib import Path
import argparse, os, struct

SIZE = 0x800000

def regions(image):
    if len(image) != SIZE or image[0x10:0x14] != bytes.fromhex('5aa5f00f'):
        raise ValueError('Expected an 8 MiB raw Intel SPI image, not an ASUS capsule')
    frba = ((struct.unpack_from('<I', image, 0x14)[0] >> 16) & 0xff) * 16
    if not 0x20 <= frba <= 0x1000 - 12:
        raise ValueError('Invalid flash-region table')
    actual = []
    for index in range(3):
        value = struct.unpack_from('<I', image, frba + 4 * index)[0]
        actual.append(((value & 0x7fff) << 12, (((value >> 16) & 0x7fff) + 1) << 12))
    if actual != [(0, 0x10000), (0x400000, SIZE), (0x10000, 0x400000)]:
        raise ValueError('Unsupported Descriptor/BIOS/ME layout; refusing extraction')
    return image[:0x10000], image[0x10000:0x400000]

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('backup', type=Path)
    ap.add_argument('destination', type=Path)
    args = ap.parse_args()
    descriptor, me = regions(args.backup.read_bytes())
    os.umask(0o077)
    args.destination.mkdir(parents=True, exist_ok=False)
    (args.destination / 'descriptor.bin').write_bytes(descriptor)
    (args.destination / 'me.bin').write_bytes(me)
    print('Private regions extracted. Add your own mrc.bin here. No hardware was written.')

if __name__ == '__main__':
    main()
