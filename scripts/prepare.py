#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Fetch pinned upstream sources and install the public board overlay. Never flash."""
from pathlib import Path
import argparse, json, shutil, subprocess

ROOT = Path(__file__).resolve().parents[1]

def run(args, cwd=None):
    subprocess.run([str(a) for a in args], cwd=cwd, check=True)

def fetch(url, path, commit):
    path.mkdir(parents=True, exist_ok=False)
    run(['git', 'init', '--quiet', path])
    run(['git', 'remote', 'add', 'origin', url], path)
    run(['git', 'fetch', '--depth=1', 'origin', commit], path)
    run(['git', 'checkout', '--detach', commit], path)
    actual = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=path, text=True).strip()
    if actual != commit:
        raise RuntimeError('Unexpected upstream commit')

def modules(tree, spec, cache):
    for line in spec['submodules']:
        if line.startswith('-'):
            continue
        commit, rel = line[1:].split()[:2]
        target = tree / rel
        # Git checkout creates an empty submodule directory.
        if target.exists():
            target.rmdir()
        if cache:
            fetch((cache / rel).resolve().as_uri(), target, commit)
        else:
            run(['git', 'submodule', 'update', '--init', '--checkout', '--depth=1', '--', rel], tree)
        actual = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=target, text=True).strip()
        if actual != commit:
            raise RuntimeError('Unexpected submodule revision: ' + rel)

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--destination', type=Path, default=ROOT / 'build/coreboot')
    ap.add_argument('--source-cache', type=Path, help='Optional existing pinned coreboot checkout; use its local Git objects')
    args = ap.parse_args()
    dst = args.destination.resolve()
    if dst.exists():
        raise SystemExit('Destination already exists; choose a new directory')
    lock = json.loads((ROOT / 'sources.lock.json').read_text())
    cache = args.source_cache.resolve() if args.source_cache else None
    fetch(cache.as_uri() if cache else 'https://review.coreboot.org/coreboot', dst, lock['coreboot']['commit'])
    run(['git', 'apply', '--check', ROOT / 'patches/0001-p9di-integration.patch'], dst)
    run(['git', 'apply', ROOT / 'patches/0001-p9di-integration.patch'], dst)
    shutil.copytree(ROOT / 'board', dst / 'src/mainboard/asus/p9d-i')
    shutil.copy2(ROOT / 'scripts/relative_genfw.py', dst / 'payloads/external/edk2/relative_genfw.py')
    modules(dst, lock['coreboot'], cache)
    rel = Path('payloads/external/edk2/workspace/mrchromebox')
    edk_cache = cache / rel if cache else None
    fetch(edk_cache.as_uri() if edk_cache else 'https://github.com/mrchromebox/edk2', dst / rel, lock['EDK2']['commit'])
    modules(dst / rel, lock['EDK2'], edk_cache)
    shutil.copy2(ROOT / 'configs/p9di.config', dst / '.config')
    print('Prepared pinned public source tree:', dst)

if __name__ == '__main__':
    main()
