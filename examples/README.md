# Examples

## `runtime_attestation.cpp`

A small, self-contained Windows program that **measures** what protection is
actually active in its own process — it does not assume anything. It is the same
application shown in the repository demo recording.

It reports three sections:

| Section | Measured with |
|---|---|
| **Authorization** | `KARMANTI_TIER_ID`, injected into the payload by the container |
| **Storage** | its own image path (NTFS ADS stream), the carrier's real size (`GetFileSizeEx`) and attributes (`GetFileAttributesW`) |
| **Memory** | a real `PROCESS_VM_READ` open attempt against itself **and** against its parent container — denied access proves the kernel DACL is in place |

It ends with a single verdict line:

```
VERDICT: PROTECTED   (license=on, dacl=on, zero-disk=on)
```

Run it **unprotected** and you should get `UNPROTECTED` — no licence context, a
flat file on disk, and its memory readable. Wrap it with Karmanti and the same
binary reports `PROTECTED` without a single line of code changing.

### Build

```bash
zig c++ -O2 -std=c++17 runtime_attestation.cpp -o runtime_attestation.exe
# or, with MinGW:
g++ -O2 -std=c++17 runtime_attestation.cpp -o runtime_attestation.exe
```

### Run

```bash
runtime_attestation.exe             # human-readable report
runtime_attestation.exe --json      # one-line JSON, for CI
runtime_attestation.exe --no-wait   # no final "Press [Enter]"
```

`--json` output can be asserted directly, for example:

```bash
runtime_attestation.exe --json --no-wait | findstr /C:"\"verdict\":\"PROTECTED\""
```

### Notes

- This is a *reporting* example. It contains no protection logic of its own — the
  protection lives in the Karmanti-generated launcher, which is a separate,
  licence-bound artifact.
- The executable carries no Karmanti code, so it is safe to build and inspect.
- Windows only (x64); no third-party libraries.
