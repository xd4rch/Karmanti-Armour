# Zero-Disk NTFS ADS Execution & .NET Interoperability

Karmanti eliminates dropped binaries by staging protected PE files into transient Alternate Data Streams (ADS) backed by zero-byte hidden carriers.

---

## 1. NTFS Stream Staging Workflow

1. **Carrier Allocation:** The runtime allocates a zero-byte host carrier file (`node_<8>.dat`) inside the execution directory, applying `FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM`.
2. **Stream Encapsulation:** The decrypted buffer is written directly to the unnamed stream extension:
   `node_<8>.dat:s_<24>.exe`
3. **Suspended Execution:** The process is spawned via Win32 `CreateProcessA` using `CREATE_SUSPENDED (0x00000004)`.
4. **Kernel Hardening:** Before the entry point executes:
   * A restrictive Kernel DACL denies `PROCESS_VM_READ`, `PROCESS_VM_WRITE`,
     `PROCESS_VM_OPERATION` and `PROCESS_DUP_HANDLE`.
   * A keep-open handle on the stream is held with full sharing
     (`FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE`) so the image
     cannot be swapped out from under the loader.
5. **Sanitization:** Upon child process termination, the first 4096 bytes of the
   staging stream are overwritten with zeros and the carrier file is unlinked.

---

## 2. The .NET Framework ADS Path Dilemma

### Symptom & Analysis
Under .NET Framework (mscorlib WinForms runtime), binaries executed from ADS collapsed with unhandled CLR exceptions:

```text
System.NotSupportedException: The given path's format is not supported.
   at System.Security.Permissions.FileIOPermission.EmulateFileIOPermissionChecks(String fullPath)
   at System.IO.Path.GetFullPath(String path)
   at System.Diagnostics.FileVersionInfo.GetVersionInfo(String fileName)
   at System.Windows.Forms.ThreadExceptionDialog..ctor(Exception t)
```

The CLR path validation utility (`PathInternal.HasInvalidVolumeSeparator`) explicitly treats the `:` stream delimiter as an illegal volume specifier. When an application encounters a managed exception, the dialog initializer crashes while attempting to query process version metadata, escalating directly into WER Fault `0xE0434352 / 0xC000041D`.

### The Karmanti Device-Path Resolution
In Version 7.4, Karmanti dynamically qualifies ADS target binaries using the Win32 Extended Device Path prefix:

```cpp
// Transforms "C:\app\node.dat:s_xyz.exe" -> "\\?\C:\app\node.dat:s_xyz.exe"
std::string exec_launch = (using_ads && device_launch_enabled)
                              ? qualify_device_path(exec_target)
                              : exec_target;
CreateProcessA(exec_launch.c_str(), lp_cmd, ...);
```

Under extended device path formatting, `PathInternal.IsDevice(fullPath)` evaluates to `true`, forcing the CLR to bypass legacy volume parsing checks. The managed `ThreadExceptionDialog` renders normally with full **Continue / Exit** functionality intact, while execution remains isolated inside NTFS streams.

Note: the decision is taken at packaging time. The extended path is only used
when the payload is a managed PE **and** has no sibling managed assemblies to
probe for — device paths break the CLR's `ApplicationBase` probing, so native
payloads keep the plain stream path. The behaviour can also be overridden at run
time through a documented launcher switch.

---

## 3. Deliberate exceptions to "zero-disk"

Two short-lived, payload-free artefacts exist by design:

* **Loader-error dialog.** If a required import DLL is missing, the payload is
  never executed. A minimal, import-only **empty stub** (carrying no payload
  bytes, named after the original executable) is started so that Windows' own
  loader error is displayed with the correct application name, and it is
  destroyed as soon as the dialog is dismissed — including when the launcher is
  killed while the dialog is open.
  * **No filesystem fallback.** Named-stream support is required for the ADS
    path. A volume that cannot host a named stream is rejected; the launcher
    never stages the payload in a flat `~krm_` file.

Neither artifact survives normal execution, and neither contains decrypted
application code after the run.

*Implementation details (size, location, flags) are part of the closed-source
implementation and are not published.*

---

## Execution model

The execution engine now has **two deliberate paths**:

1. **In-process x64 loader.** For relocatable console PEs, the image is mapped
   with `VirtualAlloc`, `.pdata` is registered via `RtlAddFunctionTable`,
   IAT/ILT and section headers are wiped, and the entry point is called directly.
   No disk or ADS write occurs.
2. **NTFS ADS + entry-lock.** For GUI, .NET, Go and resource-bearing PEs, the
   image is written only to a named ADS stream. The entry point is locked
   (`0xEB 0xFE`) and a CLR `BSJB` header is masked before launch; the process is
   created suspended, a kernel DACL denies read/write/duplicate rights, and the
   headers are restored inside the child before the first instruction.

**There is no flat-file fallback.** A volume that cannot host a
named stream is rejected; no `~krm_` staging file is ever created. The ADS
stream is delete-on-close, overwritten and removed on normal or abnormal exit.
