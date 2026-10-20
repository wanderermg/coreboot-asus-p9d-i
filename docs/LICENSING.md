# Licensing and provenance

## Repository contents

The root [LICENSE](../LICENSE) is GPL-2.0-only, matching the board sources and
coreboot integration code. SPDX headers identify the license of individual
source files. Preserve original notices when copying code. The MIT text from
the repository's initial license-only commit is retained in `LICENSES/MIT.txt`
for provenance; it does not relicense GPL-derived coreboot code as MIT.

This repository is an overlay and patch set. Full coreboot, MrChromebox EDK II
and their dependencies are fetched at pinned revisions, rather than vendored.
Their original licenses remain applicable. In particular, the GPL license on
this port does not relicense EDK II, CPU microcode or proprietary Intel firmware.

## Excluded inputs

Intel Haswell MRC is required but is not distributed. coreboot explicitly
documents the redistribution restriction:
https://doc.coreboot.org/northbridge/intel/haswell/mrc.bin.html

Descriptor and ME/SPS are supplied privately by the owner from their own board.
No assertion of redistribution rights for them is made. Likewise, no ASUS/AMI
firmware, BoardView, schematic or vendor document is republished. Board-specific
initialization code records register settings and independently implemented
behavior; no proprietary reference archive is embedded.

The source overlay does not contain captured SPD records, DMI dumps, BMC
firmware disassembly, emulator input binaries or test-machine logs.

## Future binary releases

The current publication is source-only. Do not upload the private release
archive, full ROMs or the generated `artifacts/` directory. Removing unique
identifiers from a ROM does not remove proprietary code or establish rights to
redistribute it. Before any binary release, separately establish distribution
rights for every included component and satisfy the corresponding-source and
notice obligations of its licenses. See upstream coreboot's licensing notes:
https://github.com/coreboot/coreboot/blob/main/README.md
