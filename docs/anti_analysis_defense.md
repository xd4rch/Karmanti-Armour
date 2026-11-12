# In-Memory Anti-Analysis & Memory Defense Engine

Karmanti implements multi-tiered countermeasures designed to raise the cost of memory dumping, instrumentation hooks, and dynamic debugger attachment.

---

## 1. Active PE Header Erasure (4096-Byte Nulling)

Conventional unpackers (such as Scylla, x64dbg OllyDumpEx, and PE-sieve) rely on traversing in-memory Section Tables via `IMAGE_DOS_HEADER` and `IMAGE_NT_HEADERS` to locate virtual boundaries and rebuild the Import Address Table (IAT).

* **Execution Protocol:**
  1. The client process resolves its module base via `GetModuleHandleW(NULL)`.
  2. Memory protection is relaxed to `PAGE_EXECUTE_READWRITE` via `VirtualProtect`.
  3. The first 4096 bytes—encompassing the DOS Stub, PE Signature, File Header, Optional Header, and Section Tables—are securely erased with `SecureZeroMemory`.
  4. CPU instruction caches are flushed (`FlushInstructionCache`) and the original memory protection is restored.
* **Result:** Memory dumpers fail to reconstruct valid PE layouts, triggering structural parse exceptions.

---

## 2. Process Kernel DACL Hardening

Karmanti rewrites the security descriptor of the process kernel object
(`SetKernelObjectSecurity`) before the child executes its first instruction.

The resulting DACL is a **protected deny-first descriptor**: all memory-access
rights (`PROCESS_VM_READ`, `PROCESS_VM_WRITE`, `PROCESS_VM_OPERATION`) and
handle duplication are denied to the interactive user, the owner keeps only
enough access to query the security status, and `SYSTEM` keeps full access for
OS services.

Practical effect: any unprivileged utility — dumper, debugger or instrumentation
tool — attempting `OpenProcess(PROCESS_VM_READ, …)` against the protected
process fails immediately with Win32 `ERROR_ACCESS_DENIED` (5), even though it
runs under the same user account.

*Exact SDDL string and right masks are part of the closed-source implementation
and are not published.*

---

## 3. Asynchronous Anti-Dump Watchdog

An independent background worker thread periodically evaluates the integrity of
the running process:

* **Debugger attachment** — local and remote debugger presence, including
  debug-object and debug-port queries.
* **Memory layout** — the image's committed pages are re-checked for unexpected
  page-protection changes (RWX conversions, injected code pages).
* **Security descriptor integrity** — the active process DACL is re-read to
  detect permission modifications or rollback attempts.
* **Concealment of staging** — no staging artefact may survive past the run.

On a positive detection the watchdog does **not** raise a helpful error: it
terminates the process with a decoy exit code (`0xC0000005`,
`STATUS_ACCESS_VIOLATION`) after a randomized delay, so automated triage cannot
distinguish the reaction from a genuine crash.

*Exact check list, thresholds and API mix are part of the closed-source
implementation and are not published.*