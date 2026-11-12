# Changelog

All notable changes to the public documentation and example set.
The commercial product itself is versioned separately.

## [Unreleased]

### Added
- The seller desk menu offers two issuance types: end-user licences and seller
  (vendor) licences with HWID, days, class and renewal credits. Vendor licences
  are written as endor_lisans_<HWID>.lic for activating another desk.
- The desk TUI keeps its banner and caret animation alive while the operator
  types into a form.
- A pure C++ chain console for sellers: certificate verification, signed ceilings,
  sealed pool, usage ledger, 57-character licence and chaotic-digest OTS ticket.
  No Python on the seller side; 	ools/prepare_cpp_desk.py builds the folder.
- Daily ladder burn in the native C++ stub: the client consumes one one-way
  rung per day, keeps the digest in a file + registry record sealed with an
  HMAC key derived from the licence root, and refuses rollback or tampering.

- Revocation: a 16-digit tag derived from the licence identifies it without
  storing the key; tags are embedded in new builds and also read from
  `revocation_list.json` at run time. Desk add/remove commands and a list
  mode in both the Python and the native panel.
- `docs/revocation.md`.
- The seller desk verifies the manufacturer's signature on its certificate
  before it issues anything, using the manufacturer's public key shipped with
  the seller package. A certificate that does not verify stops issuance; the
  desk no longer falls back to local mode.
- A certificate can require TPM-bound builds. When set, the packager refuses to
  produce a build without TPM binding, so the restriction cannot be sold around.
- `docs/seller_certificates.md` describes the seller material, the checks and the
  failure behaviour.

## [eval-7.4.6.0] — 2026-10-29

### Published

- Seventh evaluation build (Karmanti-Demo 7.4.6.0), GitHub Release asset:
  <https://github.com/xd4rch/Karmanti-Armour/releases/tag/eval-7.4.6.0>
  - credits and licence numbers are one counter, signed into the licence
  - the counter record is sealed by the TPM and bound to the product
  - trial record sealed to the machine TPM
  - 14 days, unlimited launches, tamper-terminal, clock-rollback detection
  - unsigned, SHA-256 in the archive and in the release body
- val-7.4.5.0 kept as the previous build for comparison.

## [eval-7.4.5.0] — 2026-10-29

### Published

- Sixth evaluation build (Karmanti-Demo 7.4.5.0), GitHub Release asset:
  <https://github.com/xd4rch/Karmanti-Armour/releases/tag/eval-7.4.5.0>
  - the trial record is sealed to the machine TPM; copying it fails and
    editing it disables the build
  - licences can carry a signed credit balance (N renewals, then refused)
  - 14 days, unlimited launches, tamper-terminal, clock-rollback detection
  - unsigned, SHA-256 in the archive and in the release body
- val-7.4.4.0 kept as the previous build for comparison.

## [eval-7.4.4.0] — 2026-10-29

### Published

- Fifth evaluation build (Karmanti-Demo 7.4.4.0), GitHub Release asset:
  <https://github.com/xd4rch/Karmanti-Armour/releases/tag/eval-7.4.4.0>
  - corporate tier can require a TPM; the launcher verifies it and refuses
    the licence when the machine has none
  - delay-proof (VDF) verification in the native launcher
  - finite one-time licence pool, two-sided limits, replay refusal
  - 14 days, unlimited launches, tamper-terminal, clock-rollback detection
  - unsigned, SHA-256 in the archive and in the release body
- val-7.4.3.0 kept as the previous build for comparison.

## [eval-7.4.3.0] — 2026-10-29

### Published

- Fourth evaluation build (Karmanti-Demo 7.4.3.0), GitHub Release asset:
  <https://github.com/xd4rch/Karmanti-Armour/releases/tag/eval-7.4.3.0>
  - delay-proof (VDF) verification in the native launcher: expiry can be
    tied to sequential work rather than the wall clock
  - finite one-time licence pool, two-sided limits, replay refusal
  - digest parity guard between native and Python
  - 14 days, unlimited launches, tamper-terminal, clock-rollback detection
  - unsigned, SHA-256 in the archive and in the release body
- val-7.4.2.0 kept as the previous build for comparison.

## [eval-7.4.2.0] — 2026-10-29

### Published

- Third evaluation build (Karmanti-Demo 7.4.2.0), GitHub Release asset:
  <https://github.com/xd4rch/Karmanti-Armour/releases/tag/eval-7.4.2.0>
  - licence pool is finite by construction: the seller cannot produce more
    licences than the factory pre-authorized
  - duration, tier and licence count are re-checked by the customer application
  - an older licence is refused once a newer one was used on the machine
  - digest parity guard between the native and Python implementations
  - 14 days, unlimited launches, tamper-terminal, clock-rollback detection
  - unsigned, SHA-256 published inside the archive and in the release body
- val-7.4.1.0 kept as the previous build for comparison.

## [Unreleased]

### Hardened
- **Pool consumption lives in the signing material.** Used one-time keys are
  recorded inside the sealed pool material itself, so deleting the local
  counter can no longer revive them; a reset attempt is refused.
- **Packaging warns when the pool is absent.** A build that carries a
  certificate but no one-time pool enforces the ceilings but not the
  licence count, and now says so loudly at packaging time.

- **Legacy licences are refused by hardened packages.** A package built with
  a factory certificate accepts only chain-format licences, so the older
  issuance path cannot be used as a back door.
- **Seller desk can issue from the fixed pool.** With a certificate and a
  one-time key pool next to it, the desk consumes exactly one key per
  licence, enforces the certificate ceilings and stops when the pool is
  empty; the signing material can be sealed to the machine TPM.

- **Delay-proof credit renewal.** A renewal proof can be redeemed once: the day
  it certifies must be greater than the last redeemed day, so a batch of proofs
  is a batch of credits and an old proof cannot be replayed.
- **Licence bound to the machine TPM.** On first use the licence digest is
  sealed with the platform crypto provider; on later runs it must unseal and
  match, so a licence file copied to another machine is refused.


- **Time without a clock.** Expiry can be tied to a verifiable delay function:
  proving that a day has passed requires a fixed amount of *sequential* work
  that cannot be parallelised, and the proof verifies in a single
  exponentiation. Freezing or rewinding the machine clock no longer helps.
- **Threshold key custody.** The signing key is split over a prime field with a
  2-of-3 threshold; a single stolen share is useless and a corrupted share is
  caught by the public key.
- **Hardware sealing.** The vendor key can be sealed to the machine TPM through
  the platform crypto provider, so it never exists on disk in the clear; when no
  TPM is present the tool reports the weaker backend instead of silently
  downgrading.
- **Fixed one-time signing pool.** A licence batch cannot exceed the number of
  one-time keys the factory pre-authorised; the next licence cannot be produced
  at all.
- **Two-sided limits.** Maximum duration, tier and licence count are signed by
  the factory and re-checked by the customer application.

## [eval-7.4.1.0] — 2026-10-28

### Published

- Second evaluation build (`Karmanti-Demo 7.4.1.0`), GitHub Release asset:
  <https://github.com/xd4rch/Karmanti-Armour/releases/tag/eval-7.4.1.0>
  - trial is now tamper-terminal: editing the record, rolling the clock back,
    freezing time or restoring a snapshot permanently disables the build
  - the launcher compares its clock with the file system and the machine boot
    epoch, so a frozen process clock is detected
  - 14 days, unlimited launches (the launch counter was dropped for the demo)
  - unsigned, SHA-256 published inside the archive
- `eval-7.4.0.0` kept as the previous build for comparison.

## [7.4] — 2026-10-28

### Added

- `docs/architecture_specification.md` — cryptographic primitives, 2-of-3
  hardware attestation, chaotic day-ladder.
- `docs/ntfs_ads_execution.md` — zero-disk ADS staging and the .NET
  device-path handling, including the deliberate exceptions to "zero-disk".
- `docs/anti_analysis_defense.md` — in-memory PE header erasure, kernel DACL
  hardening, anti-dump watchdog.
- `docs/verification_suite.md` — test matrix and measured results.
- `examples/telemetry_demo.cpp` — self-contained example that *measures* the
  protection of its own process (licence context, ADS staging, carrier state,
  memory-access denial) and prints a `PROTECTED` / `UNPROTECTED` verdict.
- `SECURITY.md`, `CONTRIBUTING.md`, `LICENSE`, `.gitignore`, `.gitattributes`,
  issue templates and a CI guard that rejects binaries and licence material.

### Changed

- README rewritten as a product overview: feature matrix, run-time pipeline,
  documentation index, example, evaluation process, antivirus guidance and an
  explicit "what Karmanti is not" section.
- Version wording normalised to **7.4** across all documents.

## [eval-7.4.0.0] — 2026-10-28

### Published

- First public evaluation build (Karmanti-Demo 7.4.0.0), distributed as a
  GitHub Release asset, not in the repository:
  <https://github.com/xd4rch/Karmanti-Armour/releases/tag/eval-7.4.0.0>
  - licence-free launcher, hardware independent, 14 days / 25 launches per machine
  - trial state in three redundant, HMAC-signed stores; clock rollback and
    state edits are refused
  - full protection chain retained (ADS staging, kernel DACL, payload wipe)
  - unsigned build: SHA-256 published with the archive, false-positive
    guidance included inside it

## [Unreleased]

### Hardened

- **One-time signing pool.** An evaluation or vendor key can produce exactly as
  many licences as the factory pre-authorised. Each licence consumes one
  one-time key; when the pool is empty the next licence cannot be produced at
  all, because the key material for it does not exist.
- **Two-sided enforcement.** Every limit is checked by the customer
  application as well: a certificate signed by the factory carries the maximum
  duration, tier and licence count, and a licence outside those bounds is
  rejected on the machine, not only in the seller panel.
- **Replay and rollback refusal.** A licence older than one already used on the
  machine is rejected; the highest licence index is stored in a signed local
  record.
- **Chaotic core parity guard.** A regression test now compares the native and
  Python implementations of the internal chaotic digest over twenty seed
  lengths, so a compiler or floating-point change cannot silently break
  licence verification.



### Hardened

- Evaluation builds: tampering is now terminal. Editing the trial state,
  rolling the system clock back, freezing time or restoring a machine
  snapshot permanently disables the build (self-destroying launcher,
  poisoned trial record) instead of only refusing the run.
- Time resistance is verified against the file system's own clock and the
  machine boot epoch, so a frozen or rewound process clock is detected.

### Added

- Evaluation integrity suite: 19 state-machine checks, 9 forged-record
  checks, 8 end-to-end checks and 13 tamper-response checks.

### Documentation

- README evaluation section and verification matrix updated.
