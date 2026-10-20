# SPDX-License-Identifier: GPL-2.0-only
"""Synthetic fixtures only: no ROM, device identity or proprietary input needed."""
from pathlib import Path
import hashlib, struct, sys, unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
import update
from extract_regions import regions
from audit_public import issues


def descriptor_image():
    image = bytearray(0x800000)
    image[0x10:0x14] = bytes.fromhex('5aa5f00f')
    struct.pack_into('<I', image, 0x14, 4 << 16)
    for index, (start, end) in enumerate([(0, 0x10000), (0x400000, 0x800000), (0x10000, 0x400000)]):
        struct.pack_into('<I', image, 0x40 + 4 * index, start // 4096 | ((end // 4096 - 1) << 16))
    return image


def firmware(fill):
    image = descriptor_image()
    image[0x400000:] = bytes([fill]) * 0x400000
    areas = [('SMMSTORE', 0x410000, 0x80000), ('RW_MRC_CACHE', 0x400000, 0x10000), ('COREBOOT', 0x491000, 0x36f000)]
    struct.pack_into('<8sBBQI32sH', image, 0x490000, b'__FMAP__', 1, 1, 0xff800000, len(image), b'FLASH', len(areas))
    for i, (name, offset, size) in enumerate(areas):
        struct.pack_into('<II32sH', image, 0x490000 + 56 + i * 42, offset, size, name.encode(), 0)
    return bytes(image)


class FlashSafety(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.template = firmware(0xa5)
        cls.current = firmware(0x5a)
        cls.fd = cls.current
        cls.sha = hashlib.sha256(cls.template).hexdigest()

    def test_preserve_current_store_but_take_new_code(self):
        result = update.prepare(self.template, self.current, self.current, self.fd, self.sha)
        self.assertEqual(result[0x400000:0x490000], self.current[0x400000:0x490000])
        self.assertEqual(result[0x490000:], self.template[0x490000:])
        update.verify(result, self.current, result, self.fd, self.fd)

    def test_refuse_wrong_hash(self):
        with self.assertRaises(RuntimeError):
            update.prepare(self.template, self.current, self.current, self.fd, '0' * 64)

    def test_refuse_unstable_reads(self):
        changed = bytearray(self.current); changed[0x410001] ^= 1
        with self.assertRaises(RuntimeError):
            update.prepare(self.template, self.current, bytes(changed), self.fd, self.sha)

    def test_refuse_different_descriptor(self):
        changed = bytearray(self.fd); changed[0x100] ^= 1
        with self.assertRaises(RuntimeError):
            update.prepare(self.template, self.current, self.current, bytes(changed), self.sha)

    def test_refuse_AMI_or_broken_fmap(self):
        for image in [bytes(0x800000), self.current[:0x1000]]:
            with self.assertRaises(RuntimeError):
                update.fmap(image)

    def test_refuse_readback_corruption(self):
        prepared = update.prepare(self.template, self.current, self.current, self.fd, self.sha)
        for offset in [0x410000, 0x7fffff]:
            changed = bytearray(prepared); changed[offset] ^= 1
            with self.assertRaises(RuntimeError):
                update.verify(prepared, self.current, bytes(changed), self.fd, self.fd)
        changed = bytearray(self.fd); changed[1] ^= 1
        with self.assertRaises(RuntimeError):
            update.verify(prepared, self.current, prepared, self.fd, bytes(changed))

    def test_refuse_store_replaced_even_if_code_matches(self):
        changed = bytearray(self.template)
        with self.assertRaises(RuntimeError):
            update.verify(bytes(changed), self.current, bytes(changed), self.fd, self.fd)


class InputSafety(unittest.TestCase):
    def test_descriptor_extraction(self):
        image = bytes(descriptor_image())
        fd, me = regions(image)
        self.assertEqual(fd, image[:0x10000])
        self.assertEqual(me, image[0x10000:0x400000])

    def test_refuse_capsule_wrong_size_and_layout(self):
        for image in [bytes(0x800000), bytes(0x10000), b'capsule' + bytes(descriptor_image())]:
            with self.assertRaises(ValueError):
                regions(image)
        changed = descriptor_image(); struct.pack_into('<I', changed, 0x44, 0)
        with self.assertRaises(ValueError):
            regions(changed)

    def test_publication_rejects_private_content(self):
        self.assertTrue(issues('blobs/mrc.bin', b'input'))
        self.assertTrue(issues('board/spd.c', b'\0'))
        self.assertTrue(issues('docs/test.md', ('ghp_' + 'a' * 36).encode()))
        self.assertTrue(issues('docs/test.md', ('192.' + '168.1.123').encode()))
        self.assertTrue(issues('docs/test.md', ('-----BEGIN ' + 'OPENSSH PRIVATE KEY-----').encode()))
        self.assertFalse(issues('board/spd.c', b'/* SPDX-License-Identifier: GPL-2.0-only */'))

if __name__ == '__main__':
    unittest.main()
