# Security Policy

## Scope

Karmanti-Armour is the public documentation and example repository of a
commercial software-protection product. In scope for a report:

- **Documentation errors** that misstate what the product does.
- **Leaks**: licence keys, master licences, customer hardware IDs, sealed
  packages or internal build notes accidentally published here.
- **Vulnerabilities in the example** (`examples/telemetry_demo.cpp`): memory
  safety, unsafe path handling, unbounded reads.
- **Design-level weaknesses** you can demonstrate against the documented
  model — e.g. a described protection layer that cannot work as described.

Out of scope:

- Anything that requires kernel-mode privileges, a hypervisor, physical access
  or administrative credential theft. The documented threat model already
  states that such an attacker can observe a running process.
- Generic antivirus false positives (see the README section on this).
- Requests for cracks, keygens, or help bypassing a licensed build.

## How to report

**Do not open a public issue for a leak or a vulnerability.**

Email <xd4rch@gmail.com> with:

1. a short description and the affected file/section,
2. reproduction steps or a proof of concept,
3. the impact you believe it has,
4. whether you want public credit.

You should receive an initial response within **5 business days**. If the report
is valid we will fix it, publish a note in `CHANGELOG.md`, and credit you unless
you ask otherwise.

## Handling of leaked material

If you find a published licence key, master licence or customer identifier in
this repository, treat it as **already compromised**: report it privately as
above, do not reuse it, and do not republish it.

## Supported versions

Only the latest published documentation set is supported. Version 7.4 is the
current baseline.
