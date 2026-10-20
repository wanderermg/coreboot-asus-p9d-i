# Building the public candidate

## Environment

Use Linux with Git, GNU make, GCC/G++, Python 3, Pillow, NASM, IASL, bison,
flex, ncurses development files, zlib development files, OpenSSL development
files, curl/wget and normal C/C++ build tools. Package names vary by distro.
The crossgcc script may download additional toolchain sources.

The private reference environment used host GCC 16.2.1, reference i386-elf GCC
15.2.0, binutils 2.46.1, NASM 3.02 and IASL 20251212. EDK II uses the host
compiler. A different host toolchain can change output bytes; a successful
compile does not qualify a new firmware image.

## 1. Fetch and apply the source overlay

From the root of this repository:

```sh
python3 scripts/prepare.py
```

The script creates a new `build/coreboot/`, fetches the exact revisions in
`sources.lock.json`, applies the infrastructure patch, copies the board overlay
and installs the path-independent GenFw rule helper. Existing destinations are
refused. It does not track upstream branch heads or invoke a programmer.

An optional `--source-cache /path/to/coreboot` uses the local Git objects of an
existing pinned checkout and its initialized submodules, including the EDK II
checkout at `payloads/external/edk2/workspace/mrchromebox`. Revisions are still
verified against the lock file. No private files are copied from that cache.

## 2. Supply private inputs

Read [BLOBS.md](BLOBS.md) first. Extract from **your own full 8 MiB raw backup**:

```sh
python3 scripts/extract_regions.py /private/path/original-spi.rom blobs
```

This creates a previously nonexistent `blobs/` with owner-only permissions and
validates the Intel Descriptor layout before extracting `descriptor.bin` and
`me.bin`. It does not accept ASUS capsule files or arbitrary flash layouts.
Put your separately obtained `mrc.bin` in the same private directory.

Expected layout:

```text
blobs/descriptor.bin  0x010000 bytes
blobs/me.bin          0x3f0000 bytes
blobs/mrc.bin         190180 bytes; validated Peppy MRC hash in BLOBS.md
```

No captured SPD images are required. Each boot reads the installed DIMMs.

## 3. Build

```sh
python3 scripts/build.py --jobs 4
```

This builds the reference crossgcc toolchain if no toolchain is supplied. To use
an existing compatible reference toolchain instead:

```sh
python3 scripts/build.py --toolchain /path/to/xgcc --jobs 4
```

The script verifies source revisions, MRC identity, private region sizes/layout
and configuration stability; it builds coreboot and EDK II, checks the resulting
flash layout, then creates:

```text
artifacts/p9di-r13-public.rom
artifacts/build-metadata.json
```

These files are private and ignored. The complete ROM includes your Descriptor
and ME/SPS. Do not attach it to a public release or issue. Metadata is generated
from your build and is the input to the BIOS-only updater.

The firmware version is `-p9di-r13-public`. Compared with private r13, SPD capture
files and the comparison-only CBFS requirement are removed. The version string
and blob paths also change. Public output must not be expected to match the
private r13 checksum or inherit its hardware qualification.

## 4. Check and qualify

```sh
python3 -m unittest discover -s tests -v
python3 scripts/audit_public.py
```

For an actual installation, follow [FLASHING.md](FLASHING.md) and repeat the
hardware checks in [VALIDATION.md](VALIDATION.md). No build script flashes or
reboots a machine.
