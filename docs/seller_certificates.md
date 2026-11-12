# Seller certificates in the vendor desk

A seller certificate is the manufacturer's signed permission for one seller: how
long a licence may be, which tier it may carry, how many licences the seller may
issue, and until when the seller is allowed to sell at all. The desk reads those
ceilings from the certificate instead of keeping them as local settings.

This page describes what the desk does with the certificate. It is a security
property of the desk, not a convenience: a seller who can widen the ceilings has
no ceilings.

## What the manufacturer hands over

| Item | Purpose |
|---|---|
| The signed certificate | The ceilings, signed by the manufacturer |
| The manufacturer's public key | Lets the desk verify that signature |
| The licence pool | The finite set of one-time keys the certificate authorises |

The desk never carries the manufacturer's private material. It cannot mint a
certificate; it can only verify one and stay inside it.

## What the desk checks before it issues

1. **Signature.** The certificate is verified with the manufacturer's public key.
   If it does not verify, the desk refuses to issue. It does not fall back to
   local mode.
2. **Ceilings.** Maximum licence duration, maximum tier, maximum licence count
   and the authority window are read from the verified certificate.
3. **Seller identity.** The desk's own key must be the one named in the
   certificate; a certificate issued for another seller is refused.
4. **Pool.** The one-time pool must contain at least as many keys as the
   certificate authorises.
5. **TPM requirement.** A certificate can require TPM-bound builds. The packager
   refuses to produce a build without TPM binding under such a certificate, so
   the restriction is enforced where the build is made, not only at the desk.

## Failure behaviour

| Situation | Desk behaviour |
|---|---|
| No certificate installed | Local mode; the desk works without a chain. |
| Certificate installed, public key missing | Refuse; the desk asks for the manufacturer's key. |
| Certificate does not verify | Refuse. No fallback. |
| Seller authority expired | Refuse. |
| Pool exhausted | Refuse. |
| Pool material missing | Refuse; the pool is the material. |
| Certificate requires TPM, build without TPM requested | Packager refuses. |

A desk that has been given a certificate is therefore always in chain mode: the
only way to issue is to satisfy the certificate.

## What is not claimed

- The desk cannot stop a seller from *asking* for more; it can only refuse to
  issue outside the signed limits.
- Global counting of licences is impossible offline: the pool bounds a batch,
  not the world.
- If the manufacturer's signing material is compromised, certificates issued
  from it are compromised too. That is why the manufacturer side supports a
  threshold split of the signing key and optional sealing of the signing
  material.


## Pure C++ chain console

The seller can work without Python:

`
krm_desk.exe --menu --secret <product secret> --cert vendor_cert.txt ^
    --factory factory_pub.txt --pool pool --used used.sealed
`

The console verifies the manufacturer certificate with the pinned public key
(the Karmanti certificate uses the project's chaotic challenge), enforces the
signed ceilings, keeps a sealed usage ledger, generates the 57-character
licence and writes the one-time-signature ticket (KRM4|<index>|<h1>) whose
chains and Merkle path use the project's chaotic digest, not SHA-256.

The manufacturer builds the folder with 	ools/prepare_cpp_desk.py; the
factory secret stays outside it.
