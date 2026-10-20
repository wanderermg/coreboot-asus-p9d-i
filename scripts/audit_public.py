#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Reject binary/private inputs and obvious secrets in tracked publication content."""
from pathlib import Path
import argparse, re, subprocess, sys
ROOT = Path(__file__).resolve().parents[1]
ALLOWED_ROOTS = {'board', 'configs', 'docs', 'patches', 'scripts', 'tests', 'LICENSES', '.github'}
ALLOWED_FILES = {'.gitignore', '.gitattributes', 'README.md', 'LICENSE', 'sources.lock.json'}
BLOCKED_EXT = {'.bin', '.rom', '.fd', '.cap', '.bundle', '.zip', '.7z', '.gz', '.xz', '.log', '.jnlp', '.pem', '.key'}
PATTERNS = {
    'private key': r'-----BEGIN (?:OPENSSH|RSA|EC|DSA|ENCRYPTED)? ?PRIVATE KEY-----',
    'GitHub token': r'\b(?:gh[pousr]_[A-Za-z0-9]{20,}|github_pat_[A-Za-z0-9_]{30,})\b',
    'credential URL': r'https?://[^\s/@]+:[^\s/@]+@',
    'LAN IPv4': r'\b(?:10(?:\.\d{1,3}){3}|192\.168(?:\.\d{1,3}){2}|172\.(?:1[6-9]|2\d|3[01])(?:\.\d{1,3}){2})\b',
    'MAC address': r'\b(?:[0-9a-fA-F]{2}:){5}[0-9a-fA-F]{2}\b',
    'local home path': r'/(?:home|Users)/[A-Za-z0-9_.-]+/',
    'private development path': r'gw' + r'2(?:[.-][A-Za-z0-9.-]+)?|gw' + r'2-spi-backup|codex-' + r'clipboard',
    'binary Git patch': r'GIT ' + r'binary patch|^literal \d+$',
}

def issues(name, data):
    path = Path(name)
    found = []
    if path.is_absolute() or '..' in path.parts:
        found.append('unsafe path')
    if name not in ALLOWED_FILES and path.parts[0] not in ALLOWED_ROOTS:
        found.append('path outside publication allowlist')
    if path.suffix.lower() in BLOCKED_EXT or 'spd' in path.parts or path.name == 'provenance.json':
        found.append('private/binary file type')
    try:
        text = data.decode('utf-8')
    except UnicodeDecodeError:
        return found + ['non-UTF-8 content']
    if '\0' in text:
        found.append('NUL/binary content')
    for label, pattern in PATTERNS.items():
        if re.search(pattern, text, re.M):
            found.append(label)
    return found

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--staged', action='store_true', help='Read index contents rather than working files')
    ap.add_argument('--history', action='store_true', help='Also inspect every reachable historical blob')
    args = ap.parse_args()
    names = subprocess.check_output(['git', 'ls-files', '-z'], cwd=ROOT).decode().split('\0')
    errors = []
    count = 0
    for name in filter(None, names):
        if (ROOT / name).is_symlink():
            errors.append((name, ['symlink not allowed'])); continue
        data = subprocess.check_output(['git', 'show', ':' + name], cwd=ROOT) if args.staged else (ROOT / name).read_bytes()
        found = issues(name, data)
        if found:
            errors.append((name, found))
        count += 1
    if args.history:
        objects = subprocess.check_output(['git', 'rev-list', '--objects', '--all'], cwd=ROOT, text=True)
        for line in objects.splitlines():
            parts = line.split(' ', 1)
            if len(parts) != 2:
                continue
            oid, name = parts
            if subprocess.check_output(['git', 'cat-file', '-t', oid], cwd=ROOT).strip() != b'blob':
                continue
            found = issues(name, subprocess.check_output(['git', 'cat-file', 'blob', oid], cwd=ROOT))
            if found:
                errors.append(('history:' + name, found))
    for name, found in errors:
        print(name + ': ' + ', '.join(found), file=sys.stderr)
    if errors:
        raise SystemExit(1)
    print(f'PASS: {count} tracked files; no blocked paths, binaries or matched secret/identity patterns.')
    print('This pattern scan complements manual review; it is not proof against every possible secret.')

if __name__ == '__main__':
    main()
