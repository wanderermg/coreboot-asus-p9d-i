# Installing, updating and recovering

This port currently targets P9D-I REV1.02 with the hardware and restrictions in
[VALIDATION.md](VALIDATION.md). The public candidate needs hardware qualification.
Keep an external programmer and a verified original full SPI backup available.

## Jumpers and recovery preparation

Normal positions established for the tested board are:

| Jumper | Position |
| --- | --- |
| ME_PWRMODE1 | 2–3 |
| FDO1 | 1–2 |
| CLRTC1 | 1–2 |

Do not change jumpers while the board is powered. Incorrect ME_PWRMODE1 can
prevent power-on even when the BMC is reachable. RAID_SEL1 is independent of
NVMe boot and must be chosen for the intended SATA setup.

The tested chip is an 8 MiB W25Q64FV. Before a first external write, remove AC,
use verified 3.3 V supply **and SPI signal levels**, confirm chip orientation and
isolate the chip from the powered board. Some CH341A boards need level fixes.
Read the chip twice and compare the complete backups before writing anything.
Store backups outside this repository and retain the ability to restore them.

## Initial installation from AMI: external programmer

Build from **this board's** Descriptor/ME/SPS and the required MRC. Check that the
complete new image retains the original first 0x400000 bytes byte-for-byte.
The new BIOS region replaces AMI; AMI settings are not compatible with SMMSTORE.
The supplied internal updater intentionally rejects the AMI layout.

Example for an isolated chip on a correctly configured CH341A:

```sh
flashrom -p ch341a_spi -c 'W25Q64BV/W25Q64CV/W25Q64FV' -r /private/path/read1.rom
flashrom -p ch341a_spi -c 'W25Q64BV/W25Q64CV/W25Q64FV' -r /private/path/read2.rom
cmp /private/path/read1.rom /private/path/read2.rom
```

Confirm both reads match your intended board backup, and that the ROM was built
from those exact private regions. Then write your locally built complete image:

```sh
flashrom -p ch341a_spi -c 'W25Q64BV/W25Q64CV/W25Q64FV' -w artifacts/p9di-r13-public.rom
flashrom -p ch341a_spi -c 'W25Q64BV/W25Q64CV/W25Q64FV' -v artifacts/p9di-r13-public.rom
```

Require success from both commands. Reinstall the chip only after disconnecting
the programmer. Do not use a full image assembled from someone else's inputs.

## Compatible coreboot-to-coreboot update from Linux

Use a diagnostic Linux whose kernel permits flashrom internal access. Keep the
production kernel's security settings. Do not force flashrom past an access or
layout error. Root, Python 3 and flashrom are required.

Prepare only, without writing:

```sh
sudo python3 scripts/update.py artifacts/p9di-r13-public.rom \
  artifacts/build-metadata.json --output /var/tmp/p9di-prepare
```

The output directory must be new. Review the result and retained readbacks. To
perform the actual update, use another new directory:

```sh
sudo python3 scripts/update.py artifacts/p9di-r13-public.rom \
  artifacts/build-metadata.json --output /var/tmp/p9di-update --write
```

The updater:

1. Checks board name, image SHA256, compatible FMAP and Descriptor.
2. Reads BIOS twice and requires stable contents.
3. Preserves the current BIOS prefix 0x400000–0x48ffff, including SMMSTORE.
4. Writes **only BIOS 0x400000–0x7fffff**, excluding Descriptor/ME/SPS.
5. Verifies and independently reads back BIOS and Descriptor.

Only `state: verified` in `result.json` means the complete procedure succeeded.
If writing or verification fails, keep the machine powered, retain all logs and
investigate before rebooting. The script does not reboot automatically.
It checks integrity and layout, not a cryptographic firmware signature.

Selected-read containers have an 8 MiB file size but are **not full ME/SPS
backups**. Keep the original external full backup separately. ASMB7 firmware and
I210 NVM are outside this updater's scope.

After a successful update and backup retention:

```sh
sudo systemctl reboot
```

Verify version, root device, both ECC channels, EDAC and ASMB7 sensors. Check
`efibootmgr` before changing boot entries; entry numbers are local to each board.

## Recovery

For a compatible previous coreboot build, use its local ROM and matching metadata
with the same BIOS-only updater. An actual public-candidate rollback/forward
cycle is not yet qualified.

If the board cannot POST, power it down and externally restore the verified
original full SPI backup. Restoring AMI is a full external recovery operation;
do not feed an AMI dump to the internal coreboot updater. Do not overwrite BMC
firmware or I210 NVM as part of BIOS recovery.
