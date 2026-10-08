# Changelog

## 3.0.0 — 2026-10-08

Changes since [v2.1.0](https://github.com/bucanero/apollo-lib/releases/tag/v2.1.0).

### ⚠️ Breaking changes

- **New C API naming conventions** ([#69](https://github.com/bucanero/apollo-lib/pull/69)). The public functions in `include/apollo.h` are now namespaced:
  - Patch engine: `apollo_apply_*` / `apollo_load_*`
    - `apply_cheat_patch_code` → `apollo_apply_code`
    - `apply_sw_patch_code` / `apply_bsd_patch_code` / `apply_py_script_code` → `apollo_apply_sw_code` / `apollo_apply_bsd_code` / `apollo_apply_py_code`
    - `load_patch_code_list` → `apollo_load_code_list`
    - `free_patch_var_list` → `apollo_free_var_list`
    - `apollo_get_data_endianness` → `apollo_get_endianness`
  - Checksums: `apollo_hash_<algorithm>` (e.g. `crc32_hash` → `apollo_hash_crc32`, `mgspw_Checksum` → `apollo_hash_mgspw`, `force_crc32` → `apollo_hash_force_crc32`)
  - Crypto: `apollo_crypt_<cipher>(mode, ...)`. Each encrypt/decrypt pair is now one function that takes `APOLLO_ENCRYPT` or `APOLLO_DECRYPT` (new `apollo_crypt_mode_t`) as its first argument, matching the `ucrypto` Python module. AES CTR, DW8XL, RGG Studio and MGS5 TPP have no inverse, so they take no mode.
- **`offzip_free()` takes a handle.** offzip scans are now truly session-based. The handle `offzip_init()` returns now owns all of its scan state; in 2.x it pointed at one shared global. `offzip_free(handle)` releases it, and independent scans no longer interfere with each other. `offzip_util()` manages its own handle.
- **MGS Peace Walker save type is now passed by the caller.** It's `apollo_crypt_mgs_pw(mode, data, len, type)` in C, `ucrypto.mgs_pw(mode, data, type)` in Python, and `encrypt/decrypt mgs_pw(type)` in BSD. `type` is `0` for PS3 HD Edition, `1` for PSP US/EU and `2` for PSP JP digital.
- The PC GUI moved out of this repo into [bucanero/apollo-patcher](https://github.com/bucanero/apollo-patcher) ([#71](https://github.com/bucanero/apollo-lib/pull/71)).

BSD script commands stay backward-compatible. The only exception is the new `mgs_pw` save-type argument.

### ✨ New features

- **WebAssembly build** ([#71](https://github.com/bucanero/apollo-lib/pull/71), [#73](https://github.com/bucanero/apollo-lib/pull/73)): `Makefile.wasm` builds `libapollo.a` with Emscripten, along with an out-of-tree wasm mbedcrypto. `make -f Makefile.wasm test` runs the full test suite under Node, and the build has its own CI workflow.
- **MGS Peace Walker PSP support**: saves from PSP US/EU (ULUS10509, ULES01372) and PSP JP digital (NPJH50045) can now be decrypted and encrypted, alongside PS3 saves.
- **Fletcher checksums**: `fletcher16` and `fletcher32` are available in BSD (`set [var]:fletcher16`), in Python (`uhashlib.fletcher16/32`) and in C (`apollo_hash_fletcher16/32`). Fletcher-32 always sums 16-bit little-endian words. An odd-length input is zero-padded.
- **Explicit code-type prefixes** ([#72](https://github.com/bucanero/apollo-lib/pull/72)): `[SW:...]` and `[BSD:...]` set a code's type so the body shape no longer decides it. A Save Wizard code with one malformed line no longer silently turns into a BSD script.
- **Combinable title prefixes** ([#75](https://github.com/bucanero/apollo-lib/pull/75)): prefixes can be stacked in any order, e.g. `[BE:SW:Max Money]` or `[DEFAULT:PYTHON:Name]`. Before, only one prefix was recognised per title. When two prefixes say the same kind of thing, the last one wins, so `[LE:BE:...]` is big-endian instead of having both byte-order flags set.
- **Save Wizard lines inside BSD scripts**: a BSD script can contain `XXXXXXXX YYYYYYYY` lines directly, with no keyword around them. Each run of consecutive Save Wizard lines is applied as one Save Wizard code to the data as the script has left it, then the script continues. A run starts from its own pointer at `0` and follows the code's `[LE:]`/`[BE:]` byte order. Only valid hex lines count: anything else, such as an `XX` placeholder, is skipped like any line BSD doesn't recognise. BSD commands are matched first, so existing scripts behave the same; no patch in apollo-patches has Save Wizard lines inside a BSD code today.
- **Code-list teardown helpers** ([#72](https://github.com/bucanero/apollo-lib/pull/72)): `apollo_free_code_list()` and `apollo_free_code_entry()` free what `apollo_load_code_list()` allocated. Any header entries the caller put in the list first are left alone.
- **CI**: macOS CLI and GUI artifacts are now built as Universal2 (x86_64 + arm64, macOS 11.0+), and a verification step checks them with `lipo`/`otool` ([#70](https://github.com/bucanero/apollo-lib/pull/70)).

### 🐛 Fixes

- **MGS Peace Walker**: fixed two out-of-bounds reads in the crypto, which a malformed save could trigger. Added known-answer tests that use real game saves.
- **Big-endian correctness for BSD** ([#74](https://github.com/bucanero/apollo-lib/pull/74)):
  - `left()`, `right()` and `carry()` slices now write the right bytes at every width. Before, 3-byte slices came out reversed on little-endian hosts.
  - On little-endian hosts the 8-byte account ID was written byte-reversed.
  - The `md5_xor`-style 16-byte hash variables were reversed on little-endian hosts.
  - The DBZ Xenoverse 2 checksum wrote its eight bytes backwards on big-endian hosts.
- **Python on 32-bit targets** ([#73](https://github.com/bucanero/apollo-lib/pull/73)): `struct.pack_into` with a bignum value smaller than the field (e.g. `'>I'` with `0xFFFFFFFF ^ 0xFFFFFFFB`) didn't write the leading zero/sign bytes. This affected PS3, PSP, Vita and wasm32.
- **wasm MicroPython GC** ([#73](https://github.com/bucanero/apollo-lib/pull/73)): the test binary now runs `wasm-opt --spill-pointers` after linking. Without it the conservative GC scan missed live objects and crashed with `bad free`.
- **Parser** ([#72](https://github.com/bucanero/apollo-lib/pull/72)):
  - Trailing tabs are now trimmed. Before, `[Name]\t` produced no code, and a Save Wizard line ending in a tab was read as BSD.
  - A line containing only blanks no longer reads `buffer[-1]`.
- **BSD `set pointer`** ([#75](https://github.com/bucanero/apollo-lib/pull/75)):
  - `set pointer:read(0x20)` and other unreadable offsets no longer pick up uninitialised stack values. They now default to 0 and log a warning.
  - Bad `read()` arguments are reported instead of being silently used.
- **BSD `set [a]:[a]`**: a variable that references itself no longer ends up with an empty value.
- `encrypt`/`decrypt` mode keywords are handled case-insensitively, and the BSD encrypt and decrypt paths were merged into one.

### 📚 Docs and tests

- The README has a new C API section with a 2.x → 3.0 migration note, plus WebAssembly build instructions.
- `savepatch.rst` now documents the `[SW:]`, `[BSD:]`, `[LE:]` and `[BE:]` prefixes, how prefixes combine, and how the code type is detected.
- `bsd.rst`, `ucrypto.rst` and `uhashlib.rst` now cover Fletcher and the `mgs_pw` save type.
- The test suite was greatly expanded:
  - BSD crypto, MGS PW, offzip and parser tests
  - a large real-sample suite (`test_samples.c`)
  - endian-gap coverage
  - the full suite now also runs under Node/wasm in both LE and BE modes.

## 2.1.0 — 2026-08-26

Changes since [v2.0.4](https://github.com/bucanero/apollo-lib/releases/tag/v2.0.4).

### ⚠️ Breaking changes

- **The byte order of save data is now chosen at runtime** ([#64](https://github.com/bucanero/apollo-lib/pull/64)). The compile-time `__PS3_PC__` / `BIGENDIAN=1` build is gone, and with it the separate `patcher-bigendian` binary. Hosts pass `APOLLO_DATA_MODE_LITTLE` or `APOLLO_DATA_MODE_BIG` to `apollo_set_endianness()` and read the setting back with `apollo_get_data_endianness()`. `APOLLO_DATA_MODE_DEFAULT` follows the byte order of the machine running the library.
- `custom_crc_t.xor` was renamed to `xorOut`, because `xor` is a reserved token in C++ ([#65](https://github.com/bucanero/apollo-lib/pull/65)).
- `mgspw_Encrypt`/`mgspw_Decrypt` and `mgs5tpp_encode_data` now take `uint8_t*` instead of `uint32_t*`, so they can be passed unaligned buffers ([#67](https://github.com/bucanero/apollo-lib/pull/67)).

### ✨ New features

- **`patcher -b/--big-endian` and `-l/--little-endian`** select the data byte order at runtime. Little-endian is the default ([#64](https://github.com/bucanero/apollo-lib/pull/64)).
- **`[LE:...]` / `[BE:...]` title prefixes** set the byte order for one Save Wizard code, overriding the host setting ([#64](https://github.com/bucanero/apollo-lib/pull/64)). New flags: `APOLLO_CODE_FLAG_ORDER_LE` / `APOLLO_CODE_FLAG_ORDER_BE`.
- **Apollo Patcher GUI** ([#65](https://github.com/bucanero/apollo-lib/pull/65)): a Dear ImGui + GLFW/OpenGL3 desktop front-end for Windows, macOS and Linux, under `gui/`. It's built on a stdio-free `apollo_ctrl` facade over libapollo, and CI builds it, including a 32-bit Windows (mingw-i686) target. It moved to its own repo in 3.0.
- **Test suite** ([#66](https://github.com/bucanero/apollo-lib/pull/66)): `tests/` covers the parser, BSD, Save Wizard and search, plus a golden-output corpus of real `.savepatch` fixtures that runs in both LE and BE modes. A new `tests.yml` workflow runs it in CI. Later releases added bounds and NULL-safety tests ([#67](https://github.com/bucanero/apollo-lib/pull/67), [#68](https://github.com/bucanero/apollo-lib/pull/68)).
- The PS3 build now uses mbedTLS (`-D_USE_MBEDTLS`).

### 🐛 Fixes

- **Hardening against malformed patches** ([#67](https://github.com/bucanero/apollo-lib/pull/67)):
  - Save Wizard and BSD operations now check offsets and lengths against the save buffer before they run. That covers writes, copies, inc/dec, pointer reads, type 6, conditional and bulk writes, search patterns, `read()` and the `xor` stride. Instead of writing out of bounds, they log a `SKIP out-of-bounds ...` message.
- **Unaligned memory access** ([#67](https://github.com/bucanero/apollo-lib/pull/67)): FF13, MGS Peace Walker, the SW4 and DBZ Xenoverse 2 checksums, and the Save Wizard byte swaps now use byte-wise copies instead of casting pointers to wider integers. On strict-alignment targets this fixes `SIGBUS` crashes, which the corpus caught on most BLUS30490 codes.
- **Allocation and NULL safety** ([#68](https://github.com/bucanero/apollo-lib/pull/68)):
  - Every allocation and parse step in the BSD and Save Wizard engines now rejects the code cleanly instead of dereferencing NULL. This applies to failed allocations, missing delimiters, and multi-line codes that run out of lines.
  - Save Wizard lines must be a full `XXXXXXXX YYYYYYYY`.
  - `carry()` values outside 0–4 are rejected instead of causing a huge allocation, and the 32-bit shift that was undefined at `carry(4)` was fixed.
  - Checksum ranges ending at `eof+1` are clamped. Before, they read one byte of heap and gave nondeterministic checksums.
  - BSD variables never point at stack memory any more.
- **Loader** ([#68](https://github.com/bucanero/apollo-lib/pull/68)):
  - A non-seekable file no longer turns into `malloc((size_t)-1)`.
  - Short reads no longer leave uninitialised data behind.
  - An unknown `{tag}` in a code body no longer writes past the options array.
  - Malformed `A=B=C;` and unterminated `=value` option lines no longer leak or misparse.
- **Byte-order fixes** ([#66](https://github.com/bucanero/apollo-lib/pull/66)): the `carry()`, `right()` and `left()` truncations and `mid()` slices picked the wrong bytes when big-endian data was processed on a little-endian host.
- **Windows**: the MicroPython 64-bit config is now chosen on `_WIN64`, not on any MinGW build, which fixes 32-bit builds ([#65](https://github.com/bucanero/apollo-lib/pull/65)).
- The `GROUP:` log line no longer prints the leading delimiter.

## 2.0.4 — 2026-04-22

Changes since [v2.0.0](https://github.com/bucanero/apollo-lib/releases/tag/v2.0.0). This release includes the untagged 2.0.2 changes.

### ⚠️ Breaking changes

- **DES-ECB was replaced by Triple-DES ECB** ([#62](https://github.com/bucanero/apollo-lib/pull/62)). The BSD command `des_ecb(key)` is now `des3_ecb(key)`, the Python call `ucrypto.des_ecb` is now `ucrypto.des3_ecb`, and the C functions `des_ecb_*` are now `des3_ecb_*`. All of them now take a 24-byte key.
- **The offzip API now takes a session handle** ([#61](https://github.com/bucanero/apollo-lib/pull/61)): `offzip_init(data, size, wbits)` returns a `void*` handle, which `offzip_search()` and `offzip_verify()` now take instead of the raw data pointer.
- `apollo_host_cb_t` now reports its size through `uint32_t*` instead of `int*` ([#58](https://github.com/bucanero/apollo-lib/pull/58)).
- **The `parser` CLI tool was removed** and its features folded into `patcher` ([#63](https://github.com/bucanero/apollo-lib/pull/63)). `patcher file.savepatch [-c 1,2,7-10]` lists the codes in a patch and can show the details of selected ones.

### ✨ New features

- **mbedTLS backend** ([#45](https://github.com/bucanero/apollo-lib/pull/45)): with `-D_USE_MBEDTLS` the library builds against mbedTLS 2.16.12 (`libmbedcrypto`) instead of PolarSSL 1.3.9. The PS4 and PSP builds, the PC tools and CI all use it now.
- **`uhashlib.hmac_sha256(key, data)`** is a new Python function ([#62](https://github.com/bucanero/apollo-lib/pull/62)).
- **Save Wizard code type D** gained data type `2`, which reads a 16-bit little-endian value. `0` stays 16-bit big-endian and `1` stays 8-bit ([#60](https://github.com/bucanero/apollo-lib/pull/60)).
- **The BSD `compress` command can now recompress an extracted offzip variable** after it was modified, because it reads the variable's current data and length by reference ([#58](https://github.com/bucanero/apollo-lib/pull/58)).

### 🐛 Fixes

- **offzip** ([#61](https://github.com/bucanero/apollo-lib/pull/61)):
  - The scan now reads through a bounded memory-file layer, so reads stop at the end of the input.
  - `offzip_init()` now cleans up properly when it fails.
  - The read buffer was cut from 256 KB to 64 KB.
- **packzip** ([#58](https://github.com/bucanero/apollo-lib/pull/58)):
  - The output buffer is now sized with zlib's `compressBound()` instead of a hand-written estimate.
  - An LZMA request now fails cleanly instead of falling through.
- **search** ([#58](https://github.com/bucanero/apollo-lib/pull/58)): forward and reverse searches no longer underflow when the pattern is longer than the data.
- **Python scripts**:
  - The save buffer only grows when the script's output is larger, rather than always being reallocated (0f7a0f8). After that it is replaced instead of reallocated ([#58](https://github.com/bucanero/apollo-lib/pull/58)).
  - The `host_file_path` global is now removed after each run ([#58](https://github.com/bucanero/apollo-lib/pull/58)).
  - `apollo.endian_swap(data)` now defaults to swapping 4-byte words. Before, it used the whole buffer length as the word size (583340c).
- `x_to_u8_buffer()` allocated and converted twice as many bytes as the hex string held (f04b256).
- Tales of Zestiria checksum: the SHA-1 context is now initialised before use (ba71fc3).
- `dumper` reads its input file directly and reports errors when it creates the output file ([#63](https://github.com/bucanero/apollo-lib/pull/63)).

### 📚 Docs

- Corrected the parameter order of `uhashlib.crc()` to `crc(data, width, ...)`. The full `uhashlib` reference was reformatted.
- Documented the new data types for Save Wizard type D.
- The README now lists Python script support and points to the merged `patcher` tool.

## 2.0.0 — 2026-01-25

Changes since [v1.4.0](https://github.com/bucanero/apollo-lib/releases/tag/v1.4.0).

### ⚠️ Breaking changes

- **The patch engine works on in-memory buffers** ([#46](https://github.com/bucanero/apollo-lib/pull/46)). `apply_cheat_patch_code()` loads the file once, runs the code against the buffer, and writes the file back only if the code produced output. The per-type entry points now take and return buffers instead of file paths:
  - `apply_sw_patch_code(data, dsize, code)` replaces `apply_ggenie_patch_code(path, code)`.
  - `apply_bsd_patch_code(&data, dsize, code)` now takes a buffer.
  - New: `apply_py_script_code(&data, dsize, code)`.
  - Each returns the new data size, and `0` means failure.
- **`apply_cheat_patch_code()` no longer takes `title_id`.** The signature is now `(file_path, code, host_cb)` ([#53](https://github.com/bucanero/apollo-lib/pull/53)).
- **offzip/packzip work in memory** ([#46](https://github.com/bucanero/apollo-lib/pull/46)):
  - `offzip_util()` now takes a buffer and returns an `offzip_t` list of the streams it finds. It no longer writes files to an output folder.
  - `packzip_util()` recompresses an `offzip_t` entry into a new buffer.
  - New lower-level calls: `offzip_init()`, `offzip_search()`, `offzip_verify()` and `offzip_free()`.
- **The `md2` and `md4` BSD hashes were removed** ([#46](https://github.com/bucanero/apollo-lib/pull/46)).
- The BSD host variable `host_sysname` was renamed to `host_sys_name` ([#53](https://github.com/bucanero/apollo-lib/pull/53)).
- `[DEFAULT:...]` codes are now pre-selected through `code->activated`. In 1.4.0 they were flagged as alerts ([#53](https://github.com/bucanero/apollo-lib/pull/53)).
- New host data ID `APOLLO_HOST_DATA_PATH`. The host callback must return the app data folder for it, because that's where Python imports come from ([#49](https://github.com/bucanero/apollo-lib/pull/49)).

### ✨ New features

- **Python script codes** ([#46](https://github.com/bucanero/apollo-lib/pull/46)): a `[PYTHON:...]` title runs its body as a MicroPython 1.8.1 (Python 3.4) script.
  - The script receives the save as the `savedata` bytearray and returns the patched data by reassigning `savedata`.
  - Variables set by earlier BSD codes are available as globals.
  - The host values are available too: `host_sys_name`, `host_username`, `host_psid`, `host_account_id`, `host_lan_addr`, `host_wlan_addr` and `host_file_path` ([#53](https://github.com/bucanero/apollo-lib/pull/53)).
  - `{tag}` code options are substituted into the script, as in other code types ([#52](https://github.com/bucanero/apollo-lib/pull/52)).
  - Modules can be imported from `<data path>/python` ([#49](https://github.com/bucanero/apollo-lib/pull/49)).
  - The interpreter is created once and reused, and garbage is collected between runs. The heap size depends on the platform: 64 MB by default, 32 MB on Vita, 16 MB on PS3 and 2 MB on PSP.
- **Python modules**:
  - `apollo`: search, reverse search, `endian_swap`, applying Save Wizard codes and the version string.
  - `ucrypto`: the save-data ciphers and the game-specific encryptions.
  - `uhashlib`: the BSD hashes and checksums.
  - `uzlib`: compression and decompression, including offzip.
  - `utime`, `math`, `uio` and `collections.OrderedDict` ([#49](https://github.com/bucanero/apollo-lib/pull/49)).
  - The standard MicroPython modules: `ubinascii`, `ujson`, `umsgpack`, `ure`, `ustruct`, `uheapq`, `sys` and `gc`.
- **PBKDF2**: `pbkdf2_sha1()`/`pbkdf2_sha256()` in C and `uhashlib.pbkdf2_sha1/sha256(pwd, salt, iterations, dklen)` in Python ([#52](https://github.com/bucanero/apollo-lib/pull/52)).
- **New BSD hash `sha224`** ([#46](https://github.com/bucanero/apollo-lib/pull/46)).
- **More BSD `set` values** ([#46](https://github.com/bucanero/apollo-lib/pull/46)): `set [var]:` accepts any value literal (quoted string, hex bytes or `[var]` reference). `0x...` values are stored as 32-bit integers.
- **In-memory `decompress`/`compress`** ([#46](https://github.com/bucanero/apollo-lib/pull/46)): a decompressed stream becomes a `~extracted\XXXXXXXX.dat` variable. Later codes can target it as their file, and `compress` writes it back into the save.
- **The checksum helpers are now public** in `apollo.h`: `md5_xor_hash`, `sha1_xor64_hash`, `add_hash`, `wadd_hash`, `dwadd_hash`, `qwadd_hash` and `wsub_hash`.
- **CLI tools**: `patcher` now pre-selects `[DEFAULT:]` codes. `parser` shows each code's type (Save Wizard, BSD or Python) and marks required codes with `[R]`.
- **Documentation site** ([#50](https://github.com/bucanero/apollo-lib/pull/50), [#51](https://github.com/bucanero/apollo-lib/pull/51)): a Sphinx reference under `docs/`, published by a new `documentation.yml` workflow. It covers the `.savepatch` format, BSD, Save Wizard codes, the CLI tools, the C library and every Python module.
- **CI** uploads the CLI tools as `apollo-cli-*` artifacts instead of a tarball ([#54](https://github.com/bucanero/apollo-lib/pull/54)).

### 🐛 Fixes

- **`ucrypto` functions return the processed data buffer.** Before, they returned the mode argument ([#52](https://github.com/bucanero/apollo-lib/pull/52)).
- **Code-type detection** now only reclassifies Save Wizard codes as BSD, so a Python code's body never changes its type. A hex-digit check that could never be true was removed ([#46](https://github.com/bucanero/apollo-lib/pull/46)).
- **BSD `set [var]:[other]`** now always stores a 32-bit value. Before, the variable length could be left unset (568fffb).
- **The crypto log lines** report key sizes in bits and moved into one place ([#52](https://github.com/bucanero/apollo-lib/pull/52)).
