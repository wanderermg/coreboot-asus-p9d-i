#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Build the public P9D-I candidate using private, owner-supplied inputs. Never flash."""
from pathlib import Path
import argparse, hashlib, json, os, shutil, subprocess
from extract_regions import regions
ROOT = Path(__file__).resolve().parents[1]
MRC_SHA256 = 'd368ba45096a3b5490ed27014e1f9004bc363434ffdce0c368c08a89c4746722'

def sha(b):
    return hashlib.sha256(b).hexdigest()

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--tree', type=Path, default=ROOT / 'build/coreboot')
    ap.add_argument('--blobs', type=Path, default=ROOT / 'blobs')
    ap.add_argument('--toolchain', type=Path, help='Existing reference xgcc directory; otherwise build crossgcc')
    ap.add_argument('--jobs', type=int, default=4)
    args = ap.parse_args()
    if args.jobs < 1:
        raise SystemExit('--jobs must be positive')
    tree = args.tree.resolve()
    lock = json.loads((ROOT / 'sources.lock.json').read_text())
    for path, expected in [(tree, lock['coreboot']['commit']), (tree / 'payloads/external/edk2/workspace/mrchromebox', lock['EDK2']['commit'])]:
        if subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=path, text=True).strip() != expected:
            raise SystemExit('Source revision differs from lock file')
    mrc = (args.blobs / 'mrc.bin').read_bytes()
    if len(mrc) != 190180 or sha(mrc) != MRC_SHA256:
        raise SystemExit('MRC differs from the validated Peppy 1.6.1 Build 2 binary')
    fd = (args.blobs / 'descriptor.bin').read_bytes()
    me = (args.blobs / 'me.bin').read_bytes()
    if len(fd) != 0x10000 or len(me) != 0x3f0000:
        raise SystemExit('Private region size mismatch')
    regions(fd + me + bytes(0x400000))
    os.umask(0o077)
    private = tree / 'private'
    private.mkdir(exist_ok=True)
    for name, data in [('descriptor.bin', fd), ('me.bin', me), ('mrc.bin', mrc)]:
        (private / name).write_bytes(data)
    xgcc = tree / 'util/crossgcc/xgcc'
    if args.toolchain:
        target = args.toolchain.resolve()
        if xgcc.is_symlink() and xgcc.resolve() != target:
            raise SystemExit('Existing toolchain symlink points elsewhere')
        if not xgcc.exists():
            xgcc.symlink_to(target, target_is_directory=True)
    else:
        subprocess.run(['make', 'crossgcc-i386', 'CPUS=' + str(args.jobs)], cwd=tree, check=True)
    env = dict(os.environ)
    env['PATH'] = str(xgcc / 'bin') + os.pathsep + env['PATH']
    subprocess.run(['make', 'olddefconfig'], cwd=tree, env=env, check=True)
    expected_config = (ROOT / 'configs/p9di.config').read_bytes()
    if (tree / '.config').read_bytes() != expected_config:
        raise SystemExit('Configuration changed; inspect the diff before building')
    subprocess.run(['make', '-j' + str(args.jobs), 'UPDATED_SUBMODULES=1'], cwd=tree, env=env, check=True)
    image = (tree / 'build/coreboot.rom').read_bytes()
    if len(image) != 0x800000 or image[:0x400000] != fd + me:
        raise SystemExit('Built image size or Descriptor/ME differs from private inputs')
    from update import fmap
    fmap(image)
    artifacts = ROOT / 'artifacts'
    artifacts.mkdir(exist_ok=True)
    target = artifacts / 'p9di-r13-public.rom'
    shutil.copy2(tree / 'build/coreboot.rom', target)
    (artifacts / 'build-metadata.json').write_text(json.dumps({'image_filename': target.name, 'image_sha256': sha(image), 'revision': 'r13-public', 'status': 'BUILD-ONLY: public derivative, not hardware qualified', 'coreboot': lock['coreboot']['commit'], 'edk2': lock['EDK2']['commit']}, indent=2) + '\n')
    print('Built private candidate:', target, sha(image))
    print('This derivative is not the hardware-qualified private r13 ROM. No hardware was written.')

if __name__ == '__main__':
    main()
