# ASUS P9D-I

Board-specific coreboot / EDK II port for P9D-I REV1.02, Intel C222,
Xeon E3-1246 v3, Nuvoton NCT6779D and ASMB7/AST2300. Validation uses
2 x 8 GiB Kingston SL8D316E11D8KF DDR3 ECC UDIMMs and a PCIe x4 NVMe SSD.
Release status and the exact tested image hashes belong in the accompanying
release validation report; successful compilation alone is not qualification.

## Memory and shared SMBus

DIMM_A1 is at SMBus 0x50 and DIMM_B1 at 0x52. Read both live 256-byte
SPDs before MRC; require DDR3 UDIMM, valid CRC and a 64+8-bit bus.
Both slots must be populated in this configuration. No captured SPD images
are included or required; training and SMBIOS use the installed DIMMs.

ASMB7 and the PCH share SPD/HWM through QU9. Before any live SPD access,
initialize KCS in romstage and request OEM netfn 0x30 command 0x02 payload 0.
The BMC stops scanning and drives BMC_HWMSW# high, selecting the PCH.
Require a successful, correctly framed response before continuing. Retain
ownership through MRC; ramstage returns the bus with payload 1 after HWM
setup. Reading SPD while BMC scanning remained active caused intermittent
boot failures, including warm reboot failures.

Use Haswell MRC with fresh training on every boot. Both MRC cache lookup
and save are bypassed; existing cache bytes are ignored, not erased.
After MRC, halt if either populated channel's MAD_DIMM ECC mode differs
from 3. Linux EDAC registration and zero error counters are checked during
qualification. These checks do not demonstrate correction of injected errors.
SMBIOS DIMM identity and 64+8-bit widths come from the live SPD.

## Board initialization

* PCIe topology: CPU PEG NVMe, PCH RP0/RP1 I210 and RP2 ASPEED.
* Factory LPC generic decodes; KCS at 0xca2/0xca3, stride 1.
* GPIO mode/direction/output masks from captured factory firmware.
  Reset selectors explicitly match 0/0/0x200: GPIO24 BMC_INIT# uses PWROK,
  GPIO73 DDR3 voltage selection uses RSMRST.
* NCT6779D COM1 at 0x3f8/IRQ4 and COM2 at 0x2f8/IRQ3.
  BoardView connects COM2 to ASMB7 UART4; live SOL routing is unqualified.
* NCT6779D HWM SMBus pins 75/76 enabled with masked CR1A changes.
  CPUTIN thermistor selection clears bit2 of HWM 5d/5e; bit0 of 5d enables
  VBAT updates, matching the Linux NCT6779 initialization policy.
  Fan modes, PWM outputs, curves and thresholds are not overwritten.
* NCT6779D Deep Sleep LDN16 is explicitly off, preserving standby power
  and ASMB7 availability in S5.
* ASMB7 S0/S5 notification and sensor scanning restored in ramstage;
  sleep notifications use the factory IPMI command pattern.
* ASPEED native framebuffer, EDK II RELEASE payload, NVMe boot support and
  persistent UEFI variables through SMMSTORE. No CSM is supplied.

Normal builds retain single POST checkpoints around bus ownership, SPD,
MRC and ECC, without diagnostic pacing delays. Detailed logs use CBMEM.
Optional historical UART/framed POST diagnostics are disabled in the release
configuration. S1/S3 are not advertised because wake paths are unqualified.

## Jumpers and flash layout

Factory normal positions verified against BoardView:
ME_PWRMODE1 **2-3**, FDO1 **1-2**, CLRTC1 **1-2**.
RAID_SEL1 is independent of NVMe and is left at the owner's existing setting.
The wrong ME_PWRMODE1 position can prevent power-on until CMOS is cleared.

The 8 MiB SPI contains a 4 MiB BIOS region at 0x400000. Preserve Descriptor
and SPS/ME byte-for-byte. Internal updates must freshly preserve the first
0x90000 bytes of the BIOS region, including SMMSTORE at 0x410000-0x48ffff.
Use the accompanying verified BIOS-only updater, retaining its backups and
independent readback results. ASMB7 firmware and I210 NVM are separate and
are outside this port's update scope.

## Qualification boundaries

Only the exact tested board, CPU, DIMMs and populated peripherals are covered
by the release report. Unused SATA/USB ports, USB overcurrent mapping,
second-NIC traffic, SOL, S3 and CSM are not qualified. Factory board/system
DMI serial and UUID are not transplanted; DIMM identity is obtained from SPD. BMC virtual media/HID,
KVM, sensors and remote power control are checked separately.

Some Debian 6.12 builds include a built-in coreboot framebuffer driver that
registers the same framebuffer already supplied by EFI GOP, causing an AST
BAR/PAT warning. On the tested installation,
`initcall_blacklist=framebuffer_driver_init` avoids that duplicate driver;
EFI framebuffer and astdrmfb remain available. This is a Linux integration
setting, not a change to the kernel binary or firmware framebuffer tables.
