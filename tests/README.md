# Apollo test suite

Characterization tests that **freeze the behavior of the patch engine**, so a
change to it has to be a deliberate one rather than a surprise.

Endianness is a **runtime** choice: `apollo_apply_sw_code()` takes the save-data
byte order from the code entry's `APOLLO_CODE_FLAG_ORDER_*` flags, falling back
to `apollo_set_endianness()` (`source/patches.c`). Nothing in the library
branches on it at compile time.

So there is **one binary**, `test_apollo`, and it runs the whole vector suite
twice in a single process — a little-endian pass, then a big-endian one —
printing a header and totals for each. Vectors whose expected bytes depend on
the mode branch on `apollo_test_be()`; vectors that are mode-invariant write
their expected bytes once, which asserts that invariance.

> These vectors and the golden manifests were originally captured from the two
> binaries this suite used to build (`test_apollo_le` and `test_apollo_be`,
> the latter compiled with `-D__PS3_PC__`) to prove the compile-time-to-runtime
> refactor byte-for-byte behavior-preserving. They still reproduce identically.

## Layout

| File | Purpose |
|------|---------|
| `test_savewizard.c` | Hand-authored Save Wizard opcode vectors, expected bytes computed by hand from `docs/savewizard.rst`, with per-endian expectations where the `MEM*` flag matters. |
| `test_sw_endian_gaps.c` | The endian-critical Save Wizard opcodes: type 3 (8-byte `MEM64` add + pointer-relative form), type 4 (32-bit `MEM32` multi-write), type 6 (pointer "mega code" — `MEM16` read and `MEM32` write), type 7 (conditional no-less/no-more-than, `MEM16`/`MEM32`), type 9 (pointer add/sub, end-pointer set), and type D's explicit 16-bit BE vs LE reads. |
| `test_bsd.c` | BSD script vectors: verbatim write/insert/delete/repeat, `left`/`mid`/`right`, `carry`-based truncation (the `HOST_LSB`/`HOST_MSB` fixes), `read()` at int16/int32/int64 widths, and hash smoke tests (`crc32big`, `sha1` against known vectors; `jhash` characterised). Fletcher-16/32 get full known-answer coverage: the published check values, the deferred-modulo block boundary (against exact arbitrary-precision reference values, not a second copy of the blocked algorithm), odd-length zero padding, an empty range, and both BSD commands. Fletcher-32's little-endian word order is fixed in the algorithm rather than taken from the host, so every one of those vectors holds identically in the LE and BE builds. |
| `test_search.c` | Search / conditional-skip behavior: Save Wizard types 8 (forward), B (backward), C (address-byte), D (byte-test skip), and the BSD `search` command — each covering found / not-found / occurrence-count paths. |
| `test_parse.c` | Savepatch parsing (`apollo_load_code_list`): code count, name extraction, Save-Wizard-vs-BSD type detection, file association, `DEFAULT`/`INFO`/`PYTHON`/`GROUP` flags, `(REQUIRED)`, `EMPTY`, and comment stripping. |
| `test_samples.c` | **Opt-in** known-answer vectors against real game saves from the `save-decrypters` repo — every tool there that ships an `.enc`/`.dec` pair. Algorithms with a non-trivial range are driven by the **actual script from the shipped `.savepatch`**, so engine/patch coupling is covered — including `search`-derived ranges, `{TAG}` option branches, and the multi-code chains a front-end applies in file order. Covers BSD ciphers, the MGS5 PS3/PS4 key set, and the **Python** patches (the only coverage MicroPython has here — `test_corpus.c` skips every Python code). Run with `make check-samples SAMPLES=... PATCHES=...`. |
| `test_mgspw.c` | MGS Peace Walker bounds vectors using synthetic buffers: undersized buffer refused, minimum size accepted, out-of-range data-derived salt offset refused, and the PSP size guard held independent of the (larger) PS3 one — a shared guard rejects every real PSP save. Plus an **opt-in** correctness round-trip against a real PS3 save via `make check-mgspw MGSPW_SAVE=...`. |
| `test_crypt_bsd.c` | BSD `encrypt`/`decrypt` command vectors: encrypt-then-decrypt round-trips for every cipher with an inverse (AES ECB/CBC, Camellia, 3-DES ECB/CBC, Blowfish ECB/CBC, Diablo 3, Silent Hill 3, NFS Undercover, MGS, FFXIII, Borderlands 3, Monster Hunter), twice-applied checks for the self-inverse streams (AES CTR, RGG Studio, DW8XL, MGS5 TPP), case-insensitive keyword matching, and unknown-algorithm inertness. |
| `test_offzip.c` | offZip session vectors: planted-stream discovery (offset / zip / unzip lengths), `offzip_util` geometry plus inflated payload, `offzip_free(NULL)` safety, sub-`g_minzip` blocks ignored, and — the point of the handle — two concurrent sessions advancing independently. |
| `test_corpus.c` | Golden regression: applies every code from a tree of real `.savepatch` files to a fixed synthetic buffer and emits a stable manifest line per code. |
| `test_common.[ch]` | Tiny assertion framework, deterministic data helpers, host-callback + log stubs, code builders, and the runtime endian mode (`apollo_test_be()`). |
| `fixtures/` | A curated, vendored set of `.savepatch` files so the committed goldens are reproducible from this repo alone. |
| `golden/` | Committed reference manifests `corpus_le.txt` / `corpus_be.txt` (the two corpus modes). |

## Running

```bash
cd tests
make check           # hand-authored opcode vectors, LE + BE passes (fast, no external deps)
make check-corpus    # re-apply fixtures and diff against committed goldens
make bsd-invariance  # assert BSD output is identical in the LE and BE passes
```

Broader correctness needs real game saves, which are likewise not vendored.
Point the check at a clone of
[save-decrypters](https://github.com/bucanero/save-decrypters):

```bash
make check-samples SAMPLES=/path/to/save-decrypters \
                   PATCHES=/path/to/apollo-patches
```

A round-trip only proves a cipher is reversible; these prove libapollo speaks
the real format. The NFS Undercover off-by-one fixed in `63f334a` round-tripped
perfectly and still produced the wrong bytes.

`PATCHES` is only needed for the Python vectors, whose patches `import` helper
modules from `apollo-patches/python`. Left at its `fixtures` default those skip
with a message and everything else still runs.

Three things these vectors pin down that are easy to get wrong from the outside:

- **Byte order is the host's job on the BSD path.** `apollo_apply_bsd_code()`
  ignores the per-code `APOLLO_CODE_FLAG_ORDER_*` flags — only
  `apollo_apply_sw_code()` reads those — and `apollo_apply_code()` does not set
  it either. It comes from `apollo_set_endianness()` alone, and
  `apollo_free_var_list()` resets it, so a host has to re-assert it **before
  every apply** (which is what `apctl_apply()` in `apollo-patcher` does). The
  MGS5 PS3 vectors fail if either half of that is missing.
- **Chained Python codes are not the same as concatenated ones.** Each code
  gets `savedata` as a fresh bytearray built from the file; a code that
  reassigns it (`savedata = uzlib.compress(...)`) leaves immutable `bytes`
  behind, which is fine at a code boundary and a `TypeError` mid-body. The FF
  Pixel Remaster vector applies its four codes separately for that reason.
- **Compression is not canonical.** Re-compressing Max Payne 3's plaintext
  yields 17202 bytes where the sample's own compressor produced 18108, and
  both decompress to the same save — so those vectors assert
  `decrypt(encrypt(x)) == x` rather than comparing against the stored
  ciphertext, and skip the header fields computed over the compressed stream.

**The Python vectors are why the wasm build works at all.** They were the first
thing in this suite to execute MicroPython — `test_corpus.c` skips every Python
code — and they turned up two real defects:

- **`micropy_mpz_as_bytes()` did not zero-fill.** It emitted one byte per digit
  the bignum had and returned, so `struct.pack_into('>I', buf, 0, x)` wrote
  only the low two bytes of a four-byte field when `x` needed fewer, and the
  high two kept whatever the buffer held. `MP_SMALL_INT` is 31 bits on a 32-bit
  target and 63 on a 64-bit one, so the values in question are bignums on
  wasm32, **PS3, PSP and PS Vita** and plain small ints on x86_64 — this was a
  32-bit bug on every console build, not a wasm one, and the native suite could
  never have caught it. It corrupted ~80% of a Dead or Alive 5 save.
  `python_mpz_pack_into_zero_fills` pins it, and needs no fixtures, so it runs
  in a plain `make check` (where, being 64-bit, it will pass either way — it
  earns its keep on the wasm and console builds).

- **The conservative GC is blind under wasm.** `micropy_gc_collect()` scans from
  `&regs` up to `vm.stack_top`, but wasm keeps locals in wasm locals rather
  than addressable memory: a measured run had **1276 bytes** of shadow stack for
  the entire live VM call chain. Live objects went unseen, were swept, and the
  next free of one tripped `assert(!"bad free")` on an `AT_FREE` block. The fix
  is Binaryen's `--spill-pointers`, which `Makefile.wasm` now applies as a
  post-link step — see the note at the top of that file for why passing it via
  `-sBINARYEN_EXTRA_PASSES` silently does nothing.

One vector still fails under wasm: `sample_mhworld`, the 8MB save, raises
`MemoryError`. Spilling makes the scan retain aggressively, and it exhausts the
MicroPython heap at `PY_HEAP_SIZE` and at 4x it. It is a clean failure, not
corruption, and the other fourteen Python vectors pass.

Two known divergences the vectors deliberately do **not** paper over:Two known divergences the vectors deliberately do **not** paper over:

- **Monster Hunter World** — `python/mhworld.py` and
  `monsterhunter-world-decrypter` disagree about where 3128 bytes sit in the
  decrypted file (the C tool rotates the `0x600488` window, the patch splices
  the block out and leaves it in place). Both round-trip, and their ciphertext
  agrees, but their plaintexts differ by ~2.4KB. The vector asserts the
  round-trip only; a save editor written against one layout will misread the
  other, so the two want reconciling upstream.
- **NFS Rivals** — the shipped `USR-DATA` pair is not self-consistent: it is
  716800 bytes but only the first 358404 were ever encrypted, so neither the
  savepatch nor the C tool (which agree with each other) can reproduce the
  `.enc` from the `.dec`. The vector pins the range the fixture actually
  covers and still proves the cipher and the custom-CRC parameters.

Correctness for MGS Peace Walker needs a real save, which is deliberately not
vendored (~300 KB of binary, and it is somebody's game data). Point the opt-in
check at an encrypted save with its decrypted twin alongside as `<file>.dec`:

```bash
make check-mgspw MGSPW_SAVE=/path/to/00000000.000
```

`check-samples` covers all three MGS PW save types from the `save-decrypters`
samples. Note libapollo leaves the decrypted header byte-swapped for PSP saves
too, where the reference tool swaps the first 17 words back — one convention for
both platforms, so a savepatch reads the header fields the same way on PS3 and
PSP, and PS3 output stays identical to previous releases. The PSP vectors
therefore compare the payload past `0x44` against the reference, assert the
header is exactly the word-swapped reference header, and prove the round-trip by
re-encrypting libapollo's *own* plaintext back to the original file.

`make golden` regenerates the committed manifests — only run it deliberately
(pre-refactor, or when fixtures change), then commit the result.

Point the corpus at the full patch repository for a broad sweep (goldens for a
full sweep are intentionally **not** committed — they depend on that repo's
state):

```bash
make check-corpus PATCHES=/path/to/apollo-patches   # diff vs committed goldens (fixtures only)
./test_apollo --corpus /path/to/apollo-patches > /tmp/le.txt        # ad-hoc manifest
./test_apollo --corpus /path/to/apollo-patches --be > /tmp/be.txt   # ...big-endian
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
integer. With big-endian save data on a little-endian PC (then the
`-D__PS3_PC__` build, now the BE pass) this kept the **high** half of the
accumulator instead of the low half — e.g. `wadd = 0x000068AC` was written as
`00 00` instead of `68 AC`. Both a real PS3 (`__PPU__`) and a real PS4/PC write
`68 AC`, so that path was simply wrong.

The fix introduces `HOST_LSB()` (in `include/types.h`), which follows the **real
host** byte order — `carry` only on a genuinely big-endian host (`__PPU__`), `0`
everywhere else, whatever the save-data order — and switches the four BSD host-integer
truncation sites to it. The save-wizard path keeps `PADDING` (it slices a value
already arranged in target-endian order). Result: BSD output is now identical in
the LE and BE passes (`make bsd-invariance`), the LE golden manifest is
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
