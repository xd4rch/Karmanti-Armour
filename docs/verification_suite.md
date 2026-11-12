# Karmanti Comprehensive Test & Verification Suite

All mathematical algorithms, storage primitives, and operating system bridges are covered by an automated test suite that ensures zero regressions across environments.

---

## Test Execution Matrix

### Test Runner 1: Full System Suite (`test_karmanti.py`)

```text
==============================================================================
Test Section                            Coverage                      Status
==============================================================================
Section A: Cryptographic Engine         Schnorr 1024, Lagrange 2/3   PASS
Section B: Guard Storage & Clock        DPAPI Vault, Monotonic ADS   PASS
Section C: Python Vendor Desk           Keygen, Zero-Key Lock        PASS
Section D: Native Vendor Desk (.exe)    Zig C++, UTF-8 TrueColor     PASS
Section E: Native Client Stub           PE Execution, ADS Staging    PASS
Section F: Protected Python Launcher    In-Memory RAM Exec, DPAPI    PASS
Section G: Integrated Self-Test         12-Phase Dual-Engine Verif.  PASS
Section H: Anti-Dump & Memory Defense   Header Wipe, DACL Security   PASS
Section I: Localization Engine          Turkish / English Parity     PASS
Section J: Credit & Time Enforcers      Ladder Erosion, Finite Pool  PASS
Section K: Terminal User Interface      Hover, Resizing, Hitboxes    PASS
==============================================================================
Final Verification Result: 80/80 PASS, 0 FAIL, 0 SKIP
```

### Test Runner 2: .NET ADS Regression Suite (`test_dotnet_ads.py`)
Verifies execution integrity using commercial .NET WinForms targets (`AutoLogin.exe`):
1. **ADS Process Identity:** Process image identified as `node_*.dat:s_*.exe`.
2. **Carrier State:** Host carrier file maintained strictly at 0 bytes with hidden attributes.
3. **Managed Dialog Recovery:** Confirms `Microsoft .NET Framework` dialog displays correctly.
4. **Continuity Proof:** Submits `BM_CLICK` to the "Continue" handle; confirms process continuity without WER escalation.

### Test Runner 3: 12-Stage Protector Self-Test (`Suite 3: 12-Phase Self-Test`)
Evaluates avalanche diffusion, forged signatures, clock rollbacks, binary hex-edits, and Vendor Desk isolation barriers:

```text
Result: 12/12 PHASES PASSED
```

### Test Runner 4: Evaluation Trial Integrity (`test_eval_*.py`)

The licence-free evaluation build is verified separately, because its failure mode
is different: it must stay usable on any machine while making tampering pointless.

| Area | Checks | Result |
|---|---|---|
| Trial state machine (day cap, launch cap, boundary) | 19 | PASS |
| State storage (three redundant stores, merge, forged records) | 9 | PASS |
| End-to-end build (ADS staging, DACL, watermarked tier) | 8 | PASS |
| Tamper response (state edit, clock rollback, frozen clock, snapshot) | 13 | PASS |
| Certificate chain (factory signature, caps, authority window) | 17 | PASS |
| One-time signature pool (N licences, N+1 impossible, tamper) | 11 | PASS |
| Chaotic core parity, Python vs native (20 seed lengths) | 20 | PASS |
| Verifiable delay function (proof, tamper, wrong step count) | 9 | PASS |
| Threshold key split (2-of-3, wrong share rejected) | 6 | PASS |
| Threshold signing (2 shares -> licence, verification) | 4 | PASS |
| TPM tier requirement (Corporate needs TPM) | 3 | PASS |
| Delay-proof credit renewal (one proof per use, replay refused) | 4 | PASS |
| Licence bound to the machine TPM (sealed digest) | 3 | PASS |
| Credit balance inside the licence (N renewals, then refused) | 3 | PASS |
| Trial record bound to the machine TPM | 3 | PASS |
| Single counter: index range bounded by credits | 3 | PASS |
| Counter sealed by the TPM (product-bound, tamper refused) | 3 | PASS |
| Legacy (v2) licence refused by chain packages | 2 | PASS |
| Seller desk in chain mode (leaf per licence, caps, pool end) | 8 | PASS |
| Signing material sealed by the TPM (tamper refused) | 3 | PASS |
| Pool consumption recorded inside the signing material (reset refused) | 2 | PASS |
| Hardware sealing (TPM seal/unseal, honest fallback) | 5 | PASS |

Observed behaviour: a valid trial runs normally; a single deleted store does not
reset the trial; an edited record, a rolled-back clock, a frozen clock or a
restored snapshot each end in a permanently disabled build.

---

## Verification model

The verification suite now runs real processes, not source-word checks:

- **Core/TUI:** 80/80.
- **Certificate chain:** 30/30.
- **C++/Python floating-point parity:** full seed sweep plus vendor-desk and
  native-stub cross-acceptance.
- **Live hook / TOCTOU lab:** `CREATE_SUSPENDED` + `NtResumeProcess`, real
  `WriteProcessMemory` patches on `ntdll`, `ucrtbase` and `advapi32`, plus a
  `.text` RAM flip with restored `PAGE_EXECUTE_READ`. All eight checks pass.
- **Full-system lab:** standard licence lifecycle, runtime revocation, eval
  encryption/run limit, C++ exception/arguments and multi-EXE compatibility.
- **x64dbg headless lab:** entry/anti-debug/hardware/ADS/licence/fuzz scenarios.
- **Build self-test:** `python Suite 3: 12-Phase Self-Test` — 12/12.
