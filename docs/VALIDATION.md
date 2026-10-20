# Qualification and remaining work

## Private r13 baseline: tested on hardware

These are measured results for the private r13 baseline, not qualification of
the public source derivative or arbitrary P9D-I installations. No unique board
identifiers, raw logs or firmware hashes tied to private regions are published.

| Check | Result |
| --- | --- |
| CPU / memory / BMC | Xeon E3-1246 v3; 2 × 8 GiB Kingston SL8D316E11D8KF ECC UDIMM; ASMB7/AST2300 firmware 1.09 |
| Boot cycles | 5 warm reboots + 5 S5 boots + BMC cold reset in S5 followed by boot: 11/11 |
| Physical G3 / AC removal | PSU disconnected for 20 seconds; no CMOS reset; first ASMB7 Power On succeeded |
| G3 follow-up | Linux on NVMe, ECC 3/3, all four EDAC counters zero, GPIO reset domains and BootOrder retained |
| ECC | Both populated channels in MAD_DIMM mode 3; EDAC registered; no recorded CE/UE |
| UEFI update/boot persistence | Fresh NVRAM preserved on update; test variable survived each cycle and was then removed |
| ASMB7 | Remote power control, KVM/keyboard, virtual media/HID and ten numeric sensors checked |
| CPU load | stress-ng, 8 workers, all methods + verify, 20 minutes, exit 0 |
| Memory patterns | memtester, mlocked 12000 MiB, all patterns, one full pass, exit 0 |
| Combined test elapsed | About 64 minutes; maximum CPU DTS 79 °C |
| Monitoring under load | 66 BMC samples; ten required numeric sensors healthy |
| NVMe data path | New 1 GiB file, write/fsync/read/hash passed under combined load |
| NVMe health | Critical warning, media error and error-log counts zero |
| KVM Ctrl+Alt+Del | Graceful Linux reboot and subsequent NVMe boot succeeded |
| Firmware build | Three private ROM builds matched byte-for-byte with the reference tools |

The ten numeric sensors are CPU1 Temperature, MB1 Temperature, VCORE1, +VDDQ,
+12V, +5V, +3.3V, +3VSB, VBAT and CPU_FAN1. SYS_FAN1's pre-existing 0 RPM
threshold status is not counted as a healthy numeric fan result.

Private coreboot time to payload was approximately 5.6–6.2 seconds, excluding
subsequent EDK II and OS work. This is not a button-to-login measurement.

## Public tree: separate qualification required

The public tree omits the two captured SPD comparison files, their provenance
and the mandatory comparison-only CBFS loads. Live reads, CRC/type/ECC checks,
MRC input and post-MRC ECC guards remain. Its local version identifies it as
`r13-public`. The public scripts build with user-supplied private binary inputs.

The public source preparation and complete coreboot + EDK II build succeeded
with owner-supplied private inputs and the reference toolchain. The build checked
configuration stability, MRC identity, final image size/FMAP and byte-for-byte
preservation of Descriptor/ME. Captured SPD files are absent from CBFS. This was
a build-only validation; no board was flashed.

The public candidate must be tested on hardware before a stable binary release
is claimed. Recommended acceptance work:

1. First boot with POST capture; verify version, live SPD and both ECC channels.
2. Repeat warm/S5 and several physical AC/G3 boots without CMOS reset.
3. Check NVMe boot, UEFI persistence, EDAC and BMC sensors after each scenario.
4. Repeat CPU/memory/NVMe load; inspect kernel, SMART and BMC event logs.
5. Observe normal server use for 24–48 hours.
6. Perform a controlled compatible rollback/forward update, retaining recovery.

## Explicit limits

- A functioning ASMB7 and its OEM shared-bus command behavior are required.
- Both DIMM slots are required. Other CPUs/DIMMs/revisions are unqualified.
- ECC correction under injected faults has not been tested.
- S1/S3 are not advertised; S4, SOL, Secure Boot policy, measured boot policy,
  unused SATA/USB ports and second-I210 traffic are unqualified.
- USB overcurrent mapping remains provisional; not all physical ports were traced.
- RTC battery retention was not independently separated from OS time synchronization.
- Factory board/system UUID and serial values are not transplanted automatically.
- There is no qualified signed-capsule update process.
- No CSM is supplied. AMI Setup features are not reproduced by EDK II.

On the tested Debian kernel, duplicate coreboot and EFI framebuffer registration
was avoided with `initcall_blacklist=framebuffer_driver_init`; EFI GOP and
astdrmfb worked with kernel taint zero. Apply this workaround only if the same
conflict is observed on a target installation. Network interface names may also
change after replacing AMI; check the actual OS configuration.
