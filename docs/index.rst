Apollo Save Tool documentation
==============================

|Downloads| |Release| |License| |macOS Linux binaries| |Windows
binaries| |Twitter|

`The Apollo Save Tool library <https://github.com/bucanero/apollo-lib>`__ and :doc:`command-line tools </cli>` implement a save-data patch engine
that supports:

- :doc:`Save Wizard / Game Genie codes </savewizard>`
- :doc:`Bruteforce Save Data scripts </bsd>`
- Python scripts (using MicroPython)

Reference documentation
-----------------------

.. toctree::
   :maxdepth: 2

   cli.rst
   bsd.rst
   savewizard.rst

Apollo Save Tool Core library (PS2/PS3/PS4/PSP/PS Vita)
-------------------------------------------------------

The library is cross-platform and is required to build: 

- `Apollo Save Tool PS2 <https://github.com/bucanero/apollo-ps2>`__ 
- `Apollo Save Tool PS3 <https://github.com/bucanero/apollo-ps3>`__ 
- `Apollo Save Tool PS4 <https://github.com/bucanero/apollo-ps4>`__ 
- `Apollo Save Tool PSP <https://github.com/bucanero/apollo-psp>`__ 
- `Apollo Save Tool PS Vita <https://github.com/bucanero/apollo-vita>`__

Python standard libraries and micro-libraries
---------------------------------------------

The following standard Python libraries have been "micro-ified" to fit in with
the philosophy of MicroPython.  They provide the core functionality of that
module and are intended to be a drop-in replacement for the standard Python
library.

.. toctree::
   :maxdepth: 1

   apollo.rst
   gc.rst
   math.rst
   sys.rst
   ubinascii.rst
   ucollections.rst
   ucrypto.rst
   uhashlib.rst
   uheapq.rst
   uio.rst
   ujson.rst
   umsgpack.rst
   ure.rst
   ustruct.rst
   utime.rst
   uzlib.rst



Save Wizard / Game Genie
~~~~~~~~~~~~~~~~~~~~~~~~

-  Code Type 0: Standard 1 Byte Write
-  Code Type 1: Standard 2 Byte Write
-  Code Type 2: Standard 4 Byte Write
-  Code Type 3: Increase / Decrease Write
-  Code Type 4: Multi-Write (Repeater)
-  Code Type 5: Copy and Paste
-  Code Type 6: Special Mega-code
-  Code Type 7: No More / No Less than Write
-  Code Type 8: Forward Byte Search (Set Pointer)
-  Code Type 9: Pointer Manipulator: (Set/Move Pointer)
-  Code Type A: Mass Write
-  Code Type B: Backward Byte Search (Set Pointer)
-  Code Type C: Address Byte Search (Set Pointer)
-  Code Type D: 2 Byte Test Commands (Code Skipper)

Bruteforce Save Data (BSD)
~~~~~~~~~~~~~~~~~~~~~~~~~~

-  Commands: ``set``, ``write``, ``search``, ``insert``, ``delete``,
   ``copy``, ``decrypt``, ``encrypt``, ``endian_swap``, ``compress``,
   ``decompress``
-  Hashes: ``crc16``, ``crc32``, ``crc32big``, ``crc64_iso``,
   ``crc64_ecma``, ``md5``, ``md5_xor``, ``sha1``, ``sha224``,
   ``sha256``, ``sha384``, ``sha512``, ``hmac_sha1``, ``sha1_xor64``,
   ``adler16``, ``adler32``, ``checksum32``, ``sdbm``, ``fnv1``,
   ``add``, ``wadd``, ``dwadd``, ``qwadd``, ``wadd_le``, ``dwadd_le``,
   ``wsub``, ``force_crc32``, ``murmur3_32``, ``jhash``,
   ``jenkins_oaat``, ``lookup3_little2``, ``djb2``
-  Custom hashes: ``eachecksum``, ``ffx_checksum``, ``ff13_checksum``,
   ``deadrising_checksum``, ``kh25_checksum``, ``khcom_checksum``,
   ``mgs2_checksum``, ``sw4_checksum``, ``toz_checksum``,
   ``tiara2_checksum``, ``castlevania_checksum``, ``rockstar_checksum``,
   ``dbzxv2_checksum``
-  Encryption: ``aes_ecb``, ``aes_cbc``, ``aes_ctr``, ``des_ecb``,
   ``des3_cbc``, ``blowfish_ecb``, ``blowfish_cbc``, ``camellia_ecb``
-  Custom encryption: ``diablo3``, ``dw8xl``, ``silent_hill3``,
   ``nfs_undercover``, ``ffxiii``, ``borderlands3``, ``mgs_pw``,
   ``mgs_base64``, ``mgs``, ``mgs5_tpp``, ``monster_hunter``,
   ``rgg_studio``

Apollo ``savepatch`` archive
----------------------------

You can find ``.savepatch`` files for many PlayStation games in the
`apollo-patches <https://github.com/bucanero/apollo-patches/>`__
repository.

Credits
-------

-  `Bucanero <https://github.com/bucanero/>`__: `Project
   developer <https://github.com/bucanero/apollo-lib/>`__

License
-------

`Apollo Save Tool <https://github.com/bucanero/apollo-lib/>`__ library -
Copyright (C) 2020-2026 `Damian
Parrino <https://x.com/dparrino>`__

This program is free software: you can redistribute it and/or modify it
under the terms of the `GNU General Public
License <https://github.com/bucanero/apollo-lib/blob/master/LICENSE>`__
as published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version.

.. |Downloads| image:: https://img.shields.io/github/downloads/bucanero/apollo-lib/total.svg?maxAge=3600
   :target: https://github.com/bucanero/apollo-lib/releases
.. |Release| image:: https://img.shields.io/github/release/bucanero/apollo-lib.svg?maxAge=3600
   :target: https://github.com/bucanero/apollo-lib/releases/latest
.. |License| image:: https://img.shields.io/github/license/bucanero/apollo-lib.svg?maxAge=2592000
   :target: https://github.com/bucanero/apollo-lib/blob/master/LICENSE
.. |macOS Linux binaries| image:: https://github.com/bucanero/apollo-lib/actions/workflows/build.yml/badge.svg
   :target: https://github.com/bucanero/apollo-lib/actions/workflows/build.yml
.. |Windows binaries| image:: https://github.com/bucanero/apollo-lib/actions/workflows/build-win.yml/badge.svg
   :target: https://github.com/bucanero/apollo-lib/actions/workflows/build-win.yml
.. |Twitter| image:: https://img.shields.io/twitter/follow/dparrino?label=Follow
   :target: https://x.com/dparrino
