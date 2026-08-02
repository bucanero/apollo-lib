# Apollo test suite

Characterization tests that **freeze the current behavior of the patch engine**
so the upcoming *"remove the compile-time big-endian flag"* refactor can be
proven byte-for-byte behavior-preserving.

Today endianness is a **compile-time** choice: the `BIGENDIAN` make flag defines
`__PS3_PC__`, which makes the `MEM*`/`PADDING` macros byte-swap
(`include/types.h`). This suite builds **twice** from the same sources —
`test_apollo_le` (default little-endian) and `test_apollo_be`
(`-D__PS3_PC__`) — exactly as `tools/Makefile` does, and captures both
behaviors as the reference. After endianness becomes a **runtime** choice, the
same vectors and golden manifests must still reproduce identically.

## Layout

| File | Purpose |
|------|---------|
| `test_savewizard.c` | Hand-authored Save Wizard opcode vectors, expected bytes computed by hand from `docs/savewizard.rst`, with per-endian expectations where the `MEM*` flag matters. |
| `test_bsd.c` | BSD script vectors: verbatim write/insert/delete/repeat (build-invariant) **plus** the one endian-sensitive BSD path (`carry`/`PADDING`, see below). |
| `test_search.c` | Search / conditional-skip behavior: Save Wizard types 8 (forward), B (backward), C (address-byte), D (byte-test skip), and the BSD `search` command — each covering found / not-found / occurrence-count paths. |
| `test_parse.c` | Savepatch parsing (`load_patch_code_list`): code count, name extraction, Save-Wizard-vs-BSD type detection, file association, `DEFAULT`/`INFO`/`PYTHON`/`GROUP` flags, `(REQUIRED)`, `EMPTY`, and comment stripping. |
| `test_corpus.c` | Golden regression: applies every code from a tree of real `.savepatch` files to a fixed synthetic buffer and emits a stable manifest line per code. |
| `test_common.[ch]` | Tiny assertion framework, deterministic data helpers, host-callback + log stubs, code builders. |
| `fixtures/` | A curated, vendored set of `.savepatch` files so the committed goldens are reproducible from this repo alone. |
| `golden/` | Committed reference manifests `corpus_le.txt` / `corpus_be.txt`. |

## Running

```bash
cd tests
make check           # hand-authored opcode vectors, LE + BE (fast, no external deps)
make check-corpus    # re-apply fixtures and diff against committed goldens
make bsd-invariance  # assert BSD output is identical in the LE and BE builds
```

`make golden` regenerates the committed manifests — only run it deliberately
(pre-refactor, or when fixtures change), then commit the result.

Point the corpus at the full patch repository for a broad sweep (goldens for a
full sweep are intentionally **not** committed — they depend on that repo's
state):

```bash
make check-corpus PATCHES=/path/to/apollo-patches   # diff vs committed goldens (fixtures only)
./test_apollo_le --corpus /path/to/apollo-patches > /tmp/le.txt   # ad-hoc manifest
```

## What the two layers guarantee

**Hand vectors** are an independent spec: expected bytes are derived from the
docs, not from the implementation, so a behavior change is *caught*, not
blessed. They cover the endian-critical Save Wizard writes (types 0–4), the
explicit-endian pointer ops (type 9), the always-big-endian search/bulk paths
(types 8/A), and BSD byte moves.

**Golden corpus** locks real-world code behavior. Each manifest line is:

```
<relpath>#<index>\t<code_type>\t<status>\t<out_size>\t<fnv1a_hash>
```

`code_type` is 1=Save Wizard, 2=BSD, 3=Python. The refactor must reproduce both
`golden/corpus_le.txt` and `golden/corpus_be.txt` exactly.

Robustness notes:

- Every code is applied in a **forked child**, so a code that dereferences a
  wild pointer on synthetic data yields one stable `CRASH(sig=N)` line instead
  of derailing the run. Crashes are deterministic and comparable across the
  refactor.
- Codes run through the public `apply_cheat_patch_code()` entry point via a temp
  file, exercising the real host callback and file path.
- Python codes and offzip-extracted targets are skipped (interpreter / external
  state, out of scope for endian testing).

### Known coverage limitation

The **corpus** buffer is fixed pseudo-random data, so real-world codes that
**search** for a pattern (or read a pointer from the file) usually miss and
record `noop` in the manifest. That still pins the search-miss path, but does
not exercise those codes' write logic through the corpus. Search find / skip /
count logic is instead covered directly and exhaustively by the hand vectors in
`test_search.c` (which place patterns at known offsets); direct-offset writes
are exercised by both. Seeding the corpus buffer with per-code search patterns
is a possible future improvement.

## Fixed bug: BSD `carry` truncation on the PS3-on-PC build

Building this suite surfaced a real bug. `carry()`-based checksum truncation in
the `wadd` / `dwadd` / `add` / `right` handlers sliced the accumulator with the
**target-endian** `PADDING` macro:

```c
var->len = BSD_VAR_INT32 - carry;
memcpy(var->data, (uint8_t*)&add + PADDING(carry), var->len);   // PADDING: carry on __PS3_PC__
```

`PADDING` follows the *target save-data* byte order, but `&add` is a **host**
integer. On the `__PS3_PC__` build (big-endian save data simulated on a
little-endian PC) this kept the **high** half of the accumulator instead of the
low half — e.g. `wadd = 0x000068AC` was written as `00 00` instead of `68 AC`.
Both a real PS3 (`__PPU__`) and a real PS4/PC write `68 AC`, so `__PS3_PC__` was
simply wrong.

The fix introduces `HOST_LSB()` (in `include/types.h`), which follows the **real
host** byte order — `carry` only on a genuinely big-endian host (`__PPU__`), `0`
everywhere else including `__PS3_PC__` — and switches the four BSD host-integer
truncation sites to it. The save-wizard path keeps `PADDING` (it slices a value
already arranged in target-endian order). Result: BSD output is now identical in
the LE and BE builds (`make bsd-invariance`), the LE golden manifest is
unchanged, and only the one affected BE line moved to match LE.

All four fixed sites have dedicated regression guards (each verified to fail
under the old semantics): `bsd_carry_padding_truncation` (wadd),
`bsd_add_carry_truncation` (add), `bsd_right_truncation` (`right()`), and
`bsd_update_existing_variable` (the existing-variable re-fetch at patches.c:796).

**Refactor implication:** BSD and Save Wizard answer to *different* notions of
endianness — Save Wizard `MEM*`/`PADDING` follow the target save-data order,
while BSD arithmetic truncation follows the host order via `HOST_LSB`. When
endianness becomes a runtime choice, keep these two concerns distinct; only the
save-data-order one should move under the runtime switch.
