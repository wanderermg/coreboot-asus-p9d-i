# coreboot + EDK II for ASUS P9D-I

A community firmware port for **ASUS P9D-I REV1.02**, Intel C222, Nuvoton
NCT6779D and ASMB7/AST2300. It provides UEFI NVMe boot, live DDR3 ECC SPD
validation, an ECC controller guard and ASMB7 hardware monitoring.

**Status: public source candidate derived from privately tested r13.**
The public tree removes device-specific SPD comparison captures. It retains live
SPD validation and ECC checks, but its resulting ROM is a different build and
has not yet been tested on hardware. Do not describe it as a hardware-qualified
r13 binary. No prebuilt firmware or private binary inputs are distributed.

## Supported configuration and behavior

The private r13 baseline was tested with a Xeon E3-1246 v3, two 8 GiB Kingston
SL8D316E11D8KF ECC UDIMMs, ASMB7 and an NVMe drive on CPU PEG.
Both DIMM slots must be populated; other CPUs, DIMMs and board revisions require
separate validation. An operational ASMB7 with the tested OEM SMBus command
behavior is required; operation without the module is not supported by this port.

- Acquire shared SPD/HWM bus ownership from ASMB7 before SPD access and MRC.
- Read both installed DIMMs at SMBus addresses 0x50/0x52; check DDR3 UDIMM,
  CRC, 64-bit data width and 8-bit ECC extension.
- Train memory on every boot, bypassing MRC cache reads and writes.
- Stop boot unless both channels report MAD_DIMM ECC mode 3.
- Restore BMC scanning and configure NCT6779D monitoring without rewriting fan
  curves or thresholds.
- Disable Deep S5 to retain BMC availability in standby.
- Boot through EDK II with persistent UEFI variables in SMMSTORE. No CSM.

See [qualification and limits](docs/VALIDATION.md),
[board implementation](board/README.md) and [POST checkpoints](docs/POST.md).
ECC mode checks and zero EDAC counters do not establish correction of injected
errors. S3, SOL and Secure Boot policy are not qualified.

## Build and install

1. Read [binary inputs and licensing](docs/BLOBS.md). Obtain your own complete
   SPI backup and the required MRC locally.
2. Follow [the pinned-source build instructions](docs/BUILDING.md).
3. Follow [installation, updating and recovery](docs/FLASHING.md).

You need a working external recovery programmer before experimenting. A failed
build or flash can prevent POST and network boot. Keep a verified backup outside
this repository. The internal updater is for compatible **coreboot-to-coreboot**
updates; it is not an AMI-to-coreboot converter.

## Repository layout

| Path | Purpose |
| --- | --- |
| `board/` | Board source overlay for `src/mainboard/asus/p9d-i/` |
| `patches/` | Changes to pinned coreboot infrastructure |
| `configs/` | Public configuration; private input paths are relative |
| `sources.lock.json` | Exact coreboot, EDK II and submodule revisions |
| `scripts/` | Source preparation, private input extraction, build and BIOS-only update |
| `tests/` | Synthetic flash-preservation and input-rejection checks |
| `docs/` | Build, install, licensing, qualification and publication audit |

Run `python3 -m unittest discover -s tests -v` and
`python3 scripts/audit_public.py` before publishing changes.
The audit examines tracked files; it does not make private build output safe to
upload. Never force-add ignored blobs, ROMs, backups or logs.

## License and publication scope

Port sources, integration changes and scripts are GPL-2.0-only unless a file
states otherwise; preserve upstream notices. See [LICENSE](LICENSE) and
[licensing notes](docs/LICENSING.md) and the [publication audit](docs/PUBLICATION_AUDIT.md). This is an independent community port,
not an ASUS, AMI, Intel or coreboot-endorsed firmware release.

This repository contains no MRC, ME/SPS, flash Descriptor, captured SPD, complete
ROM, BoardView/schematic, firmware dump, credentials or raw machine logs. Inputs
in `blobs/`, source builds in `build/` and local artifacts in `artifacts/` are
ignored and must remain private.
