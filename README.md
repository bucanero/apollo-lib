# Apollo Save Tool Core library (PS2/PS3/PS4/PSP/PS Vita)

[![Downloads][img_downloads]][app_downloads] [![Release][img_latest]][app_latest] [![License][img_license]][app_license]
[![macOS Linux binaries](https://github.com/bucanero/apollo-lib/actions/workflows/build.yml/badge.svg)](https://github.com/bucanero/apollo-lib/actions/workflows/build.yml)
[![Windows binaries](https://github.com/bucanero/apollo-lib/actions/workflows/build-win.yml/badge.svg)](https://github.com/bucanero/apollo-lib/actions/workflows/build-win.yml)
[![Twitter](https://img.shields.io/twitter/follow/dparrino?label=Follow)](https://twitter.com/dparrino)

This library and command-line tools implement a save-data patch engine that supports Save Wizard/Game Genie codes and Bruteforce Save Data scripts.

The library is cross-platform and is required to build:
- [Apollo Save Tool PS2](https://github.com/bucanero/apollo-ps2)
- [Apollo Save Tool PS3](https://github.com/bucanero/apollo-ps3)
- [Apollo Save Tool PS4](https://github.com/bucanero/apollo-ps4)
- [Apollo Save Tool PSP](https://github.com/bucanero/apollo-psp)
- [Apollo Save Tool PS Vita](https://github.com/bucanero/apollo-vita)

## Supported Code formats

- Save Wizard / Game Genie
- Bruteforce Save Data scripts

### Save Wizard / Game Genie

- Code Type 0: Standard 1 Byte Write
- Code Type 1: Standard 2 Byte Write
- Code Type 2: Standard 4 Byte Write
- Code Type 3: Increase / Decrease Write
- Code Type 4: Multi-Write (Repeater)
- Code Type 5: Copy and Paste
- Code Type 6: Special Mega-code
- Code Type 7: No More / No Less than Write
- Code Type 8: Forward Byte Search (Set Pointer)
- Code Type 9: Pointer Manipulator: (Set/Move Pointer)
- Code Type A: Mass Write
- Code Type B: Backward Byte Search (Set Pointer)
- Code Type C: Address Byte Search (Set Pointer)
- Code Type D: 2 Byte Test Commands (Code Skipper)

### Bruteforce Save Data (BSD)

- Commands: `set`, `write`, `search`, `insert`, `delete`, `copy`, `decrypt`, `encrypt`, `endian_swap`, `compress`, `decompress`
- Hashes: `crc16`, `crc32`, `crc32big`, `crc64_iso`, `crc64_ecma`, `md2`, `md4`, `md5`, `md5_xor`, `sha1`, `sha256`, `sha384`, `sha512`, `hmac_sha1`, `sha1_xor64`, `adler16`, `adler32`, `checksum32`, `sdbm`, `fnv1`, `add`, `wadd`, `dwadd`, `qwadd`, `wadd_le`, `dwadd_le`, `wsub`, `force_crc32`, `murmur3_32`, `jhash`, `jenkins_oaat`, `lookup3_little2`, `djb2`
- Custom hashes: `eachecksum`, `ffx_checksum`, `ff13_checksum`, `deadrising_checksum`, `kh25_checksum`, `khcom_checksum`, `mgs2_checksum`, `sw4_checksum`, `toz_checksum`, `tiara2_checksum`, `castlevania_checksum`, `rockstar_checksum`, `dbzxv2_checksum`
- Encryption: `aes_ecb`, `aes_cbc`, `aes_ctr`, `des_ecb`, `des3_cbc`, `blowfish_ecb`, `blowfish_cbc`, `camellia_ecb`
- Custom encryption: `diablo3`, `dw8xl`, `silent_hill3`, `nfs_undercover`, `ffxiii`, `borderlands3`, `mgs_pw`, `mgs_base64`, `mgs`, `mgs5_tpp`, `monster_hunter`, `rgg_studio`

## Apollo `savepatch` archive

You can find `.savepatch` files for many PlayStation games in the [apollo-patches](https://github.com/bucanero/apollo-patches/) repository.

## CLI Tools

[Apollo command-line tools](https://github.com/bucanero/apollo-lib/releases/latest) are useful for code creators and developers, to test SW codes and BSD scripts locally on a computer.

- [parser](#parser)
- [patcher](#patcher)
- [patcher-bigendian](#patcher-bigendian)
- [dumper](#dumper)
- [Apollo GUI](#apollo-gui)

### parser

The `parser` command-line tool reads a `.savepatch` file, and provides a numbered list of detected cheat patches.

```
Apollo .savepatch parser v0.1.0 - (c) 2021 by Bucanero

USAGE: ./parser filename.savepatch
```

### patcher

The `patcher` command-line tool reads a `.savepatch` file and a comma-separated list of patches, and apply the selected cheat codes to the target file.

```
Apollo cheat patcher v0.1.0 - (c) 2022 by Bucanero

USAGE: ./patcher file.savepatch <1,2,7,..,18> target.file
```

### patcher-bigendian

`patcher-bigendian` is provided to apply patches on PS3 (big-endian) save-game data files.

### dumper

The `dumper` command-line tool reads a binary file and generates a `.savepatch` file with a SW code that writes the raw content of the binary data.

```
Apollo binary file SW dumper v0.1.0 - (c) 2022 by Bucanero

USAGE: ./dumper file.bin
```

### Apollo GUI

Windows users that prefer a graphical interface can use [Apollo GUI](https://github.com/SkillerCMP/ApolloGUI/releases/latest) by [SkillerCMP](https://github.com/SkillerCMP). The GUI uses [Apollo CLI Tools](#cli-tools) to provide a user-friendly interface to apply save patches and scripts.

## Credits

* [Bucanero](http://www.bucanero.com.ar/): [Project developer](https://github.com/bucanero)

### Acknowledgments

* [aldostools](https://aldostools.org/): [Bruteforce Save Data](https://bruteforcesavedata.forumms.net/)
* [aluigi](http://aluigi.org): [offzip/packzip](http://aluigi.altervista.org/mytoolz.htm)
* [SkillerCMP](https://github.com/SkillerCMP): [Apollo GUI](https://github.com/SkillerCMP/ApolloGUI)

## Dependencies

This library requires:
* [PolarSSL](https://github.com/bucanero/oosdk_libraries/tree/master/polarssl-1.3.9)
* zlib

## License

[Apollo Save Tool](https://github.com/bucanero/apollo-lib/) library - Copyright (C) 2020-2024 [Damian Parrino](https://twitter.com/dparrino)

This program is free software: you can redistribute it and/or modify
it under the terms of the [GNU General Public License][app_license] as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

[app_license]: https://github.com/bucanero/apollo-lib/blob/master/LICENSE
[img_license]: https://img.shields.io/github/license/bucanero/apollo-lib.svg?maxAge=2592000
[app_downloads]: https://github.com/bucanero/apollo-lib/releases
[app_latest]: https://github.com/bucanero/apollo-lib/releases/latest
[img_downloads]: https://img.shields.io/github/downloads/bucanero/apollo-lib/total.svg?maxAge=3600
[img_latest]: https://img.shields.io/github/release/bucanero/apollo-lib.svg?maxAge=3600
