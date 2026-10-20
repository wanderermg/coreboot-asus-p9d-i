# Private binary inputs

This port is not blob-free. It depends on Intel Haswell MRC, CPU microcode and
the board's existing Intel ME/SPS and flash Descriptor. No such binary is
included in this repository.

## MRC / system-agent binary

Use Peppy Haswell **MRC 1.6.1 Build 2**, extracted from
`Google_Peppy.4389.117.0` in the private baseline. Expected identity:

- Size: 190180 bytes.
- SHA256: `d368ba45096a3b5490ed27014e1f9004bc363434ffdce0c368c08a89c4746722`.

coreboot's [Haswell MRC documentation](https://doc.coreboot.org/northbridge/intel/haswell/mrc.bin.html)
describes local extraction and explicitly states that `mrc.bin` cannot be
redistributed as part of coreboot. Starting in the prepared coreboot directory:

```sh
make -C util/cbfstool
cd util/chromeos
./crosfirmware.sh peppy
```

Inspect which image was downloaded; extract the MRC from the chosen Peppy image
using `util/cbfstool/cbfstool`, as documented upstream. For the validated image,
from `util/chromeos/`:

```sh
../cbfstool/cbfstool coreboot-Google_Peppy.4389.117.0.bin extract \
  -n mrc.bin -f /private/path/mrc.bin -r RO_SECTION
```

Copy that extracted file into this repository's ignored `blobs/` directory.
Use an explicit filename when several images are present. The build script checks the size and hash and
refuses another MRC. If the downloaded image no longer contains this version,
stop and investigate; do not silently bypass the identity check.

A hash is provided for identification, not as permission to redistribute the
binary. No automatic upload or MRC redistribution step is provided.

## Flash Descriptor and ME/SPS

Extract these locally from a verified raw SPI backup of the board being flashed.
Do not use another owner's dump or a vendor capsule as a substitute. This project
preserves their content; it does not modify ME policy or clean/reconfigure ME.
The BIOS-only updater excludes both regions from writes.

See [coreboot's extraction documentation](https://doc.coreboot.org/util/ifdtool/binary_extraction.html)
for background. The local extraction helper validates this port's exact layout.
Neither the full backup nor extracted regions belongs in Git.

## CPU microcode and upstream dependencies

Microcode is obtained in the pinned upstream build dependencies. Its license is
separate from the port's GPL license. The source-preparation step fetches upstream
repositories; it does not vendor their binary contents into this repository.
Check the applicable upstream licenses before redistributing any resulting
firmware or dependency archive.

## Material deliberately excluded

- Complete SPI images, AMI modules, diagnostic firmware and rollback binaries.
- Descriptor, ME/SPS, MRC and captured DIMM SPD data.
- BoardView archives, schematics, vendor manuals and screenshots from them.
- Raw CBMEM/DMI/IPMI logs, serial numbers, UUIDs, MAC/IP addresses, credentials,
  private keys and BMC session/JNLP files.

Hardware register values and independently written initialization code remain
in the board sources. Proprietary reference documents are not packaged.
