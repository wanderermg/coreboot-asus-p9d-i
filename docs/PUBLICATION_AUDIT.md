# Publication audit

Scope: the source-only Git index and reachable history of this repository.
Private build inputs and local verification artifacts are outside that scope and
must remain ignored. This report is a record of checks, not a guarantee that any
future added file is safe to publish.

## Content selected

- Independently written board initialization sources and public configuration.
- Text integration patch against the pinned upstream coreboot commit.
- Upstream revision lock, build helpers and BIOS-only update code.
- Build/install/recovery instructions and a summary of qualification limits.
- GPL license text, retained initial MIT text, SPDX notices and license scope.

## Content excluded

- All full-chip, BIOS, AMI, rollback and diagnostic images.
- MRC, Descriptor, ME/SPS and binary source bundles.
- Captured DIMM SPD records and their identifying provenance.
- Machine logs, UUID/serial/MAC/IP values, hostnames and private local paths.
- Passwords, login helpers, private keys, session files and BMC JNLP data.
- BoardView, schematics, vendor PDFs and reference-document screenshots.

Captured SPD files are not replaced by sanitized synthetic training data. The
public source simply drops their comparison-only loads; real SPD remains the
training input. This changes the public binary, so its hardware qualification is
explicitly separate from the private r13 baseline.

## Checks performed

- Tracked files manually reviewed by component and publication purpose.
- Publication path allowlist, file type/UTF-8 checks and identifier/secret-pattern
  scan passed on staged content and existing reachable history.
- Targeted comparison against private machine identifiers and full captured SPD
  bytes found no matches in staged files; identifier values are not in this report.
- Gitleaks v8.30.1 scanned an export of the staged Git tree: no findings.
  The tool was obtained from its official release and its archive checksum checked.
- Ten synthetic safety tests passed: NVRAM preservation, incorrect hash,
  unstable reads, different Descriptor, invalid FMAP, corrupted readback,
  unintended store replacement, extraction layout and publication rejection.
- `git diff --cached --check` passed.

The public full-build result is recorded in VALIDATION.md. No hardware was
flashed while preparing this publication, and no private ROM is included.

## Future changes

Run `python3 scripts/audit_public.py --staged --history` before publishing a
commit. Use an independent secret scanner as well. Review every staged path;
`.gitignore` is a convenience and can be bypassed. Do not commit generated
artifacts or add a binary release without a separate redistribution review.
