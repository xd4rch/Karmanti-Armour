# Karmanti Architectural Specification

Karmanti 7.4 is an enterprise binary protection and cryptographic licensing system engineered for Windows PE32/PE32+ binaries, supporting native C/C++, managed .NET environments, and Python executables. It provides runtime anti-tamper, active memory protection, and zero-disk execution primitives without hypervisor dependencies.

```
       [ Developer Machine ]
        |  (protector.py)
        |-- Raw PE / Python Payload
        |-- NTDLL LZNT1 Compression
        |-- Chaotic Keystream Cipher (iterated-tangent core, IEEE-754)
        |-- 1024-Bit Schnorr Public Sealing
        v
    [ Sealed Distribution (.krm) ]
        |
        +---> Dual-Engine Compilation
                 |-- Native C++ Stub (Zero-Python Dependency)
                 \-- Vendor Desk Management Console
                         |
                         v
       [ Client Target Machine ]
        |-- Direct Win32 Hardware Probes (CPU, SMBIOS, Disk Serial)
        |-- GF(999,999,999,989) Lagrange 2/3 Linear Interpolation
        |-- Verification via 1024-Bit Schnorr Identity
        |-- Stage into Hidden 0-Byte NTFS Alternate Data Stream (ADS)
        |-- Apply Strict Kernel DACL (Deny VM_READ / VM_WRITE)
        |-- Perform In-Memory PE Header Erasure (4096-Byte Zeroing)
        \-- ResumeThread Execution (stub) / NtResumeProcess (Python launcher)
```

---

## 1. Cryptographic Engine & Mathematical Primitives

### 1.1 Asymmetric Schnorr Identity ($g^s \cdot Y^e \pmod P$)
Licensing authenticity is verified mathematically via a 1024-bit discrete logarithm group:
* **Group:** a Schnorr group of order $Q$ inside $\mathbb{Z}_P^*$ (safe-prime construction, $P = k \cdot Q + 1$).
* **Generator ($G$):** Element of order $Q$.
* **Private Key ($x$):** Known strictly to the software owner; mathematically absent from all client stubs.
* **Public Key ($Y = G^x \bmod P$):** Embedded into the client runtime for verification:
  $$R_v = (G^s \cdot Y^e) \bmod P$$

### 1.2 Lagrange 2/3 Hardware Attestation
The master decryption seed is reconstructed by **first-degree polynomial
interpolation** over a 12-digit prime field, evaluated at three independent
hardware coordinates:
* $x_1$: CPU Processor ID (`CentralProcessor\0`)
* $x_2$: Motherboard / SMBIOS RSMB firmware table + `MachineGuid`
* $x_3$: System volume serial (`GetVolumeInformationW`)

Each hardware factor is masked independently and embedded into the licence; the
client can therefore evaluate any **two of the three** sample points and recover
the same seed. This gives transparent tolerance to a single hardware change
(board, disk or CPU upgrade) while a cloned HWID set fails interpolation.

*Exact field, sampling coordinates and interpolation coefficients are part of the
closed-source implementation and are not published.*

### 1.3 Parametric Chaotic Day-Ladder
Time validity is enforced monotonically: the ladder advances by repeated
application of the chaotic core $f$ (a deterministic iterated-tangent map,
`chaotic_core` in the implementation) and reduction modulo $P_M$:

$$cur_{i+1} = f(cur_i) \pmod{P_M}$$

License durations are bounded at compilation. If client duration units $D$ exceed
vendor limits, the resulting root candidate fails to match the Schnorr identity
$R_v$, permanently disabling runtime decryption.