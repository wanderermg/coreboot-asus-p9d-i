# Early POST checkpoints

`P9DI_SIMPLE_POST=y` emits one-byte checkpoints without diagnostic delays. These
codes are intended for ASMB7's POST history during early coreboot memory init.
Later coreboot/EDK II/OS stages may reuse the same numeric values: interpret the
sequence and boot stage, not a final byte in isolation.

| Code | Meaning |
| --- | --- |
| d0 / d1 / d2 | Shared SPD bus claim: enter / success / failure |
| e0 / e1 / e2 | Romstage / PCH ready / northbridge ready |
| e3 / e4 | RAM initialization / SPD read entry |
| e5 / e6 | Channel 0 SPD entry / success |
| e7 / e8 | Channel 1 SPD entry / success |
| e9 / ea / eb | Both SPD reads ready / map MRC / enter MRC |
| ec / ed | MRC return success / failure |
| ee / ef | Memory-controller init acknowledgement present / absent |
| f0 / f1 / f2 | CBMEM recovery entry / completed / RAM initialization complete |
| f3 / f4 / f5 / f6 | ECC guard entry / channel 0 / channel 1 / success |
| f7 / f8 | ECC guard failure for channel 0 / 1 |
| c0 / c1 | SPD read failure for channel 0 / 1 |
| c2 / c3 | SPD CRC/type failure for channel 0 / 1 |
| c4 / c5 | Unsupported DIMM type or data width for channel 0 / 1 |
| c6 / c7 | Missing ECC extension for channel 0 / 1 |

The source of truth is `board/simple_post.c`. Historical framed/UART diagnostics
remain optional and are disabled by the public configuration. No remote BMC
login script, password or machine address is included. Use your own trusted
management client and redact unique identifiers before posting logs.
