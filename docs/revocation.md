# Licence revocation (blacklist)

A licence can be withdrawn after it was issued: a refund, a chargeback, a leaked
key, or a seller contract that ended. Karmanti does this with **revocation
tags** — a 16-digit one-way digest derived from the licence itself.

## The tag

```
tag = digest("the licence-derived revocation tag" + h1)
```

`h1` is the first half of the 57-character licence key (the meta-bearing part),
so the tag identifies the licence without storing the key. The same tag is
derived anywhere: desk, guard, packager, protected launcher.

| Property | Effect |
|---|---|
| one-way | the tag does not reveal the licence |
| stable | same licence always gives the same tag |
| short | 16 digits, comparable with a plain string equality |
| side-effect free | deriving a tag never touches the vault or the clock |

## Where the list lives

Two places, on purpose:

1. **Inside the protected build.** The packager embeds the tags in the stub
   (`the embedded revocation table`). A launcher built after the revocation rejects the key even
   when the machine has no list file and no network.
2. **Next to the application.** `revocation_list.json` is read at run time:

```json
{
  "product_id": "karmanti-standart-urun",
  "updated_at": "2026-10-29 21:43:59",
  "revoked_tags": ["0691515591886871", "..."],
  "entries": {
    "0691515591886871": {
      "key_preview": "9uhN%Z#c1$...)L*XG87_",
      "reason": "refund",
      "revoked_at": "2026-10-29 15:06:15"
    }
  }
}
```

A build made **before** the revocation picks it up from this file; a build made
after it carries the tag anyway. Both paths are checked; either one refuses.

## Desk commands

```
vendor-desk --revoke <57-char key> [reason]
the vendor desk --unrevoke <key|16-digit tag>
the vendor desk --status --json        # revoked_count, revocation_file
```

The TUI has the same operation: `[4] Add license to blacklist`, plus a list mode
(`[6]`) that toggles a row in and out of the list, showing the tag, the reason
and the date. The C++ desk build supports the same flag and the same list mode,
so a seller who only has the native panel can still revoke.

## What a revoked key does

| Situation | Behaviour |
|---|---|
| Customer starts the app with a revoked key | the run ends; the message says the licence was revoked, not just "invalid" |
| Seller desk activated with a revoked master licence | the desk locks itself |
| Revoked key still in the DPAPI vault | the vault entry is not used; the run ends |
| `--unrevoke` | the key works again on the next start; nothing else changes |

## Limits, stated plainly

- Revocation is **local**: it reaches a machine when the list file is shipped
  there, or when a new build carrying the tag is shipped. There is no online
  call, so a machine that never receives either keeps working.
- Removing a tag from the file removes the revocation for builds that only
  read the file; builds with the tag embedded keep refusing.
- The tag covers the licence, not the machine: revoking one customer does not
  affect another customer's key, even on the same hardware model.

## Tests

`tests/test_karmanti.py` covers add/remove (`C5`), the runtime effect on
verification (`C5` verify path), and the desk lock on a revoked master
(`B5`). The revocation file format is asserted by the same tests, so a broken
serialiser fails the suite instead of silently disabling the feature.

---

## Revocation model

- The 16-digit `KRM-REVOKE` tag is derived from the licence H1 block without
  storing the key.
- Build-time tags are embedded in a masked `the embedded revocation table` table; root poisoning
  makes a revoked licence produce the wrong reconstruction root.
- The native launcher also reads `revocation_list.json` (and
  `data/revocation_list.json`) next to the executable on every start. Adding a
  tag there revokes a licence **without rebuilding** the application.
- Revocation is checked both in the outer licence gate and in the burn ladder;
  a revoked key cannot complete the day/credit burn.
