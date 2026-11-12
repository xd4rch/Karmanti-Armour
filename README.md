# Karmanti-Armour

**Karmanti** is a licensing and anti-dump armour for Windows applications.
A binary protector that turns a normal `.exe` (or Python application) into a
licensed, zero-disk, dump-resistant executable — without a kernel driver, without
a server, and without leaving the payload on disk.

![version](https://img.shields.io/badge/version-7.4-1f6feb)
![platform](https://img.shields.io/badge/platform-Windows%2010%20%2F%2011%20x64-0078d4)
![licence](https://img.shields.io/badge/licence-proprietary-red)

> [!NOTE]
> Karmanti is **not** a kernel driver, a hypervisor or a sandbox. It runs in user
> mode, needs no service or driver installation, and makes **no network calls** —
> everything, including licence verification, is offline.

```
your-app.exe  ──►  app.krm  +  app_native.exe  ──►  licence-gated, dump-resistant run
```

![Karmanti demo](assets/KarmantiDemo.gif)

> [!TIP]
> **Requirements:** Windows 10/11 x64, and an **NTFS** volume for the zero-disk
> path. The generated launcher is a single native `.exe` — no Python runtime
> needed on the customer machine.

---

## What it does

Karmanti takes a finished Windows binary, encrypts it into a `.krm` package and
gives you a **self-contained native launcher**. At run time the launcher verifies
a cryptographic licence, decrypts the payload **in memory**, executes it through
a hardened native loader, and wipes the staging artefacts when the application
exits.

Nothing is written to disk in plain form. The payload process is not readable by
the same user that launched it.

---

## Feature overview

| Layer | What it does |
|---|---|
| **Packaging** | `KRM3` package: LZNT1-compressed payload + chaotic-stream encryption + 1024-bit Schnorr seal over the vendor public key |
| **Licensing** | 57-character licence keys: Lagrange 2-of-3 hardware binding over GF(999 999 999 989), 1024-bit Schnorr signature, tier + duration + credit metadata, revocable through 16-digit tags |
| **Hardware binding** | Fuzzy HWID from three components (CPU, SMBIOS, disk). Any **two of three** are enough, so a single hardware change does not brick a customer |
| **Zero-disk execution** | Payload runs from a 0-byte carrier with an NTFS ADS stream; the stream is overwritten and deleted on exit |
| **Anti-dump** | The launcher wipes its own PE header in RAM and locks the child process with a kernel DACL that denies memory-read, memory-write, memory-operation and handle-duplication rights to the interactive user before the first instruction runs |
| **Anti-analysis** | Watchdog thread: debugger detection, known tool-name scan, page-protection checks, DACL weakening detection, hardware breakpoint registers; silent decoy exit instead of a helpful error |
| **Clock protection** | Triple monotonic-clock store (registry + DPAPI state file + NTFS ADS) so rolling the system clock back does not extend a licence |
| **Vendor desk** | Separate seller panel (Python and pure C++ builds) locked to one product ID. Unlocks only with a valid master licence; signing keys are never stored in plaintext |
| **Evaluation builds** | Licence-free, hardware-independent trial launchers with a time + launch limit counted per machine, signed state, and rollback/clock-tamper detection |
| **Runtime revocation** | A native launcher reads a signed, local revocation list on every start; a revoked licence stops working without rebuilding the application |
| **Native compatibility** | In-process x64 loader with exception-table registration; GUI, resource-bearing and delay-load binaries use the kernel-loader fallback |

---

## How it works

1. **Locate the licence** — `--license <key>`, a sidecar `<launcher>.lic`, the
   DPAPI vault, or the interactive licence entry.
2. **Verify offline** — 1024-bit Schnorr signature, 2-of-3 hardware match,
   duration/tier/credit checks, one-time-key burn ladder, VDF, and chain index.
3. **Unseal** — the `.krm` package is decrypted in memory; the text seal and API
   integrity mask are checked before and after decryption.
4. **Execute** — the payload is mapped by the native loader, or started through
   the kernel loader for GUI/resource/delay-load binaries.
5. **Wipe** — staging streams, temporary files, decrypted buffers and key
   material are zeroed before exit.

A normal application does not need source changes. The protector wraps the
finished binary and the licence gate sits in front of it.

---

## Evaluation

Evaluation builds are generated **per machine and per product ID**. There is no
universal trial binary in this repository.

An evaluation build:

- needs **no licence key** and runs on **any machine**,
- starts a trial on first launch: **14 days** and a fixed number of launches,
  counted per machine (a rebuild does not reset it),
- keeps the full protection chain,
- stores trial state in three redundant, HMAC-signed locations and treats state
  editing, clock rollback, time freezing or snapshot restore as terminal.

Licensed builds are issued from a **fixed pool of one-time signing keys**: the
factory pre-authorises a number of licences, each one consumes exactly one key,
and when the pool is empty the next licence cannot be produced at all.

To request an evaluation build or a commercial licence, open a private security
advisory on this repository or contact the maintainer through the repository
profile.

---

## Security

Karmanti is a client-side protection layer. It raises the cost of reverse
engineering and casual copying; it is **not** a substitute for server-side
entitlement or hardware-backed key custody.

Do not publish licence keys, customer data, private signing material or
distributable builds in this repository. Evaluation and release binaries are
provided out of band.

---

## Licence

**Proprietary — all rights reserved.** See [LICENSE](LICENSE). No permission is
granted to use, copy, modify, distribute or disclose any part of this software
without prior written permission.
