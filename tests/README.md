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
| `test_sw_endian_gaps.c` | The endian-critical Save Wizard opcodes: type 3 (8-byte `MEM64` add + pointer-relative form), type 4 (32-bit `MEM32` multi-write), type 6 (pointer "mega code" — `MEM16` read and `MEM32` write), type 7 (conditional no-less/no-more-than, `MEM16`/`MEM32`), type 9 (pointer add/sub, end-pointer set), and type D's explicit 16-bit BE vs LE reads. |
| `test_bsd.c` | BSD script vectors: verbatim write/insert/delete/repeat, `left`/`mid`/`right`, `carry`-based truncation (the `HOST_LSB`/`HOST_MSB` fixes), `read()` at int16/int32/int64 widths, and hash smoke tests (`crc32big`, `sha1` against known vectors; `jhash` characterised). |
| `test_search.c` | Search / conditional-skip behavior: Save Wizard types 8 (forward), B (backward), C (address-byte), D (byte-test skip), and the BSD `search` command — each covering found / not-found / occurrence-count paths. |
| `test_parse.c` | Savepatch parsing (`apollo_load_code_list`): code count, name extraction, Save-Wizard-vs-BSD type detection, file association, `DEFAULT`/`INFO`/`PYTHON`/`GROUP` flags, `(REQUIRED)`, `EMPTY`, and comment stripping. |
| `test_offzip.c` | offZip session vectors: planted-stream discovery (offset / zip / unzip lengths), `offzip_util` geometry plus inflated payload, `offzip_free(NULL)` safety, sub-`g_minzip` blocks ignored, and — the point of the handle — two concurrent sessions advancing independently. |
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

## Coverage matrix (hand vectors)

### Save Wizard opcodes

| Type | Operation | Sub-cases covered | Tests |
|------|-----------|-------------------|-------|
| 0 | 8-bit write | normal; pointer-relative (`08…`) | `sw_write8`, (`08…` used across search tests) |
| 1 | 16-bit write | normal | `sw_write16` |
| 2 | 32-bit write | normal | `sw_write32` |
| 3 | inc/dec write | add-32, sub-16, add-64, pointer-relative add-32 | `sw_add32`, `sw_sub16`, `sw3_add64`, `sw3_pointer_add32` |
| 4 | multi-write | 16-bit incremental, 32-bit incremental | `sw_multiwrite16`, `sw4_multiwrite32` |
| 5 | copy bytes | normal | `sw_copy` |
| 6 | pointer mega code | READ (w=0, 16-bit), MOVE-from-obtained (w=1), MOVE pointer (w=2), WRITE (w=4, 32-bit) | `sw6_read16`, `sw6_move_write32` |
| 7 | conditional write | no-less-than 16-bit, no-more-than 32-bit | `sw7_no_less_than16`, `sw7_no_more_than32` |
| 8 | forward search | found, not-found→skip, skip→resume, occurrence count | `sw_search_then_write`, `sw8_forward_found`, `…_not_found_skips`, `…_skip_resumes_at_next_search`, `…_count_second` |
| 9 | pointer manip | set BE (0), set LE (1), add (2), sub (3), eof−X (4), set X (5), end-ptr (D), end-ptr from ptr (E) | `sw_ptr_from_be_value`, `sw_ptr_from_le_value`, `sw9_pointer_add_sub`, `sw_ptr_from_eof`, `sw_ptr_set_direct`, `sw9_end_pointer_D`, `sw9_end_pointer_E` |
| A | bulk write | normal | `sw_bulk_write` |
| B | backward search | found (last), occurrence count, not-found→skip | `swB_backward_found_last`, `swB_backward_count_second`, `swB_backward_not_found_skips` |
| C | address-byte search | forward found, not-found→skip | `swC_addr_search_found`, `swC_addr_search_not_found_skips` |
| D | conditional skip | 8-bit equal (pass/skip), 16-bit BE (Z=0), 16-bit LE (Z=2) | `swD_test_pass_no_skip`, `swD_test_fail_skips`, `swD_test_16bit_be`, `swD_test_16bit_le` |

### BSD commands / functions

| Command / function | Coverage | Tests |
|--------------------|----------|-------|
| `carry(n)` | drives wadd/add truncation | `bsd_carry_padding_truncation`, `bsd_add_carry_truncation` |
| `set pointer:` | absolute address | `bsd_write_next_pointer` |
| `set range:` | range for hashes | `bsd_hash_*` |
| `set [v]:read(o,n)` | int16 / int32 / int64 widths | `bsd_read_int16`, `bsd_read_int32`, `bsd_read_int64` |
| `set [v]:wadd` | carry truncation (HOST_LSB) | `bsd_carry_padding_truncation` |
| `set [v]:add` | carry truncation (HOST_LSB) | `bsd_add_carry_truncation` |
| `set [v]:right` | rightmost bytes (HOST_LSB) | `bsd_right_truncation` |
| `set [v]:left` | leftmost bytes (HOST_MSB) | `bsd_left` |
| `set [v]:mid` | byte substring | `bsd_mid`, `bsd_mid_offset` |
| `set [v]:endian_swap` | byte reversal (existing var) | `bsd_update_existing_variable` |
| `set [v]:crc32big` | CRC-32/BZIP2 (known vector) | `bsd_hash_crc32big` |
| `set [v]:sha1` | SHA-1 (known vector) | `bsd_hash_sha1` |
| `set [v]:jhash` | Jenkins hash (characterised) | `bsd_hash_jhash` |
| `set [v]:md5_xor` | folded MD5 (characterised) | `bsd_hash_md5_xor` |
| `set [v]:sha1_xor64` | folded SHA-1 (characterised) | `bsd_hash_sha1_xor64` |
| existing-variable update | re-fetch value (HOST_LSB @796) | `bsd_update_existing_variable` |
| `write at` | verbatim hex | `bsd_write_hex` |
| `write next` | pointer-relative | `bsd_write_next_pointer` |
| `write …:repeat(c,v)` | repeated value | `bsd_write_repeat` |
| `write …:[v]` | variable value | `bsd_carry_*`, `bsd_read_*` |
| `insert` | grow buffer | `bsd_insert` |
| `delete` | shrink buffer (length) | `bsd_delete` |
| `search` | found, not-found→abort, occurrence count | `bsd_search_found`, `bsd_search_not_found_aborts`, `bsd_search_count_second` |

(The golden corpus additionally exercises many other BSD functions — CRCs, other
checksums, encryption — as characterization over the vendored fixtures, without
per-function correctness assertions. See the gap list in the project history.)

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
- Codes run through the public `apollo_apply_code()` entry point via a temp
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

### Companion fix: `left()` and `mid()` host-consistency

The same class of bug affected the two other byte-extraction helpers:

- `left(value,len)` copied from offset 0 with no host adjustment, so on a
  little-endian host it returned the *low* bytes (identical to `right`) instead
  of the leftmost/most-significant bytes. It now uses a new `HOST_MSB()` macro
  (the complement of `HOST_LSB`), so `left(0x00012345,2)` yields `00 01` on every
  build.
- `mid(value,start,len)` extracts a slice of the value's big-endian byte view,
  but for 2/4/8-byte slices the write path byte-swapped it on little-endian
  builds. It now normalises the slice to host order (like `read()`), so the
  substring is emitted verbatim on every host: `mid(0x00012345,0,2)` → `00 01`.

Both are host-consistent (identical in the LE and BE builds) and match a real
PS3 (`__PPU__`). `left()` has no real-world corpus usage; `mid()` is used as
`mid([hash],n,4)` in three PS3 patch files (one, `NPUB31564`, is vendored as a
fixture) — the fix makes the PC/CLI tools reproduce real-PS3 output for them.
Guards: `bsd_left`, `bsd_mid`, `bsd_mid_offset`.

**Refactor implication:** BSD and Save Wizard answer to *different* notions of
endianness — Save Wizard `MEM*`/`PADDING` follow the target save-data order,
while BSD arithmetic truncation follows the host order via `HOST_LSB`. When
endianness becomes a runtime choice, keep these two concerns distinct; only the
save-data-order one should move under the runtime switch.
