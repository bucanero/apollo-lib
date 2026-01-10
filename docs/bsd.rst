====================
BSD Script Reference
====================

.. contents:: Table of Contents
   :depth: 3

Overview
========
BSD (Binary Scripting Definition) is a scripting language for binary file manipulation. It allows for searching, modifying, inserting, deleting, encrypting, decrypting, and computing checksums on binary data.

Basic Commands
==============

carry
-----
Sets the carry byte value for add() and wadd() operations.

**Syntax:**

.. code-block:: text

    carry(*)

**Parameters:**

- ``*`` : Integer value for carry bytes

**Example:**

.. code-block:: text

    carry(2)    ; Sets 2 byte carry

set
---
Sets values to variables, pointers, ranges, or CRC parameters.

**Subcommands:**

set pointer
^^^^^^^^^^^
Sets the file pointer to a specific address.

**Syntax:**

.. code-block:: text

    set pointer:*
    set pointer:eof*
    set pointer:lastbyte*
    set pointer:pointer*
    set pointer:read(*,*)*
    set pointer:[*]*

**Parameters:**

- ``*`` : Absolute address in hex (e.g., 0x100)
- ``eof`` : Address relative to end of file
- ``lastbyte`` : Address relative to end of file (same as eof)
- ``pointer`` : Address relative to current pointer
- ``read(offset, length)`` : Reads value from file at offset
- ``[*]`` : Value from variable

**Examples:**

.. code-block:: text

    set pointer:0x43           ; Absolute address
    set pointer:eof-10         ; 10 bytes before end of file
    set pointer:pointer+4      ; 4 bytes after current pointer
    set pointer:read(0x100,4)  ; Read 4 bytes from offset 0x100
    set pointer:[myvar]        ; Use value from variable

set range
^^^^^^^^^
Defines a range for operations.

**Syntax:**

.. code-block:: text

    set range:*,*

**Parameters:**

- ``*`` : Start offset (hex, variable, or expression)
- ``*`` : End offset (hex, variable, or expression)

**Example:**

.. code-block:: text

    set range:0x100,0x200      ; Range from 0x100 to 0x200

set crc_*
^^^^^^^^^
Configures custom CRC parameters.

**Syntax:**

.. code-block:: text

    set crc_bandwidth:*       ; CRC width in bits
    set crc_polynomial:*      ; CRC polynomial
    set crc_initial_value:*   ; Initial CRC value
    set crc_output_xor:*      ; Final XOR value
    set crc_reflection_input:* ; Input reflection (0/1)
    set crc_reflection_output:* ; Output reflection (0/1)

**Examples:**

.. code-block:: text

    set crc_bandwidth:32
    set crc_polynomial:0x04C11DB7
    set crc_initial_value:0xFFFFFFFF

set [variable]:*
^^^^^^^^^^^^^^^^
Sets a variable to a value or computed hash.

**General Syntax:**

.. code-block:: text

    set [variable_name]:value

**Supported Value Types:**

- Direct hex: ``set [var]:0x12345678``
- String: ``set [var]:"hello"``
- Variable: ``set [var2]:[var1]``
- Arithmetic: ``set [var]:[var]+0x10``
- Bitwise operations: ``set [var]:xor:0xFF``, ``set [var]:and:0xF0``, ``set [var]:or:0x01``
- ``endian_swap``: Swaps byte order (supports 16, 32, 64-bit)
- ``eof*``: EOF-relative value

**Hash Functions:**

- ``set [var]:md5`` - MD5 hash of current range
- ``set [var]:sha1`` - SHA-1 hash
- ``set [var]:sha256`` - SHA-256 hash
- ``set [var]:sha384`` - SHA-384 hash
- ``set [var]:sha512`` - SHA-512 hash
- ``set [var]:sha224`` - SHA-224 hash
- ``set [var]:crc32`` - CRC-32 (little-endian)
- ``set [var]:crc32big`` - CRC-32/BZIP (big-endian)
- ``set [var]:crc16`` - CRC-16/XMODEM
- ``set [var]:crc64`` - CRC-64 ISO
- ``set [var]:crc64_ecma`` - CRC-64 ECMA 182
- ``set [var]:adler32`` - Adler-32 checksum
- ``set [var]:adler16`` - Adler-16 checksum
- ``set [var]:crc`` - Custom CRC (configured via set ``crc_*``)
- ``set [var]:md5_xor`` - XOR of MD5 hash bytes
- ``set [var]:sha1_xor64`` - XOR of SHA-1 hash as 64-bit
- ``set [var]:hmac_sha1(key)`` - HMAC-SHA1 with key

**Checksum Functions:**

- ``set [var]:eachecksum`` - EA/MC02 checksum
- ``set [var]:ffx_checksum`` - Final Fantasy X checksum (16-bit LE)
- ``set [var]:ff13_checksum`` - Final Fantasy XIII checksum (32-bit LE)
- ``set [var]:castlevania_checksum`` - Castlevania LoS checksum
- ``set [var]:deadrising_checksum`` - Dead Rising checksum (updates blocks)
- ``set [var]:dbzxv2_checksum`` - Dragon Ball Z Xenoverse 2 checksum (64-bit)
- ``set [var]:rockstar_checksum`` - Rockstar CHKS checksum
- ``set [var]:kh25_checksum`` - Kingdom Hearts 2.5 checksum (32-bit LE)
- ``set [var]:khcom_checksum`` - Kingdom Hearts Chain of Memories checksum
- ``set [var]:mgs2_checksum`` - Metal Gear Solid 2 checksum
- ``set [var]:mgspw_checksum`` - Metal Gear Solid Peace Walker checksum
- ``set [var]:sw4_checksum`` - Samurai Warriors 4 checksum (4x32-bit)
- ``set [var]:toz_checksum`` - Tales of Zestiria SHA1 checksum
- ``set [var]:tiara2_checksum`` - Tears to Tiara 2 checksum
- ``set [var]:checksum32`` - Generic 32-bit checksum
- ``set [var]:force_crc32:value`` - Forces CRC32 to specific value

**Hash Algorithms:**

- ``set [var]:murmur3_32[:seed]`` - MurmurHash3 32-bit
- ``set [var]:jhash[:seed]`` - Jenkins hash
- ``set [var]:jenkins_oaat[:seed]`` - Jenkins one-at-a-time hash
- ``set [var]:lookup3_little2(init1,init2)`` - lookup3 hash (2x32-bit)
- ``set [var]:sdbm[:init]`` - SDBM hash
- ``set [var]:djb2`` - DJB2 hash
- ``set [var]:fnv1[:init]`` - FNV-1 hash

**Arithmetic Functions:**

- ``set [var]:add(start,end)`` - 8-bit sum of bytes
- ``set [var]:wadd(start,end)`` - 16-bit sum of words (respects carry)
- ``set [var]:wadd_le(start,end)`` - 16-bit LE sum
- ``set [var]:dwadd(start,end)`` - 32-bit sum of dwords
- ``set [var]:dwadd_le(start,end)`` - 32-bit LE sum
- ``set [var]:qwadd(start,end)`` - 64-bit sum of qwords
- ``set [var]:wsub(start,end)`` - 16-bit subtraction
- ``set [var]:xor(start,end,increment)`` - XOR of bytes

**Data Reading Functions:**

- ``set [var]:read(offset,length)`` - Read bytes from file
- ``set [var]:right(value,length)`` - Rightmost bytes of value
- ``set [var]:left(value,length)`` - Leftmost bytes of value
- ``set [var]:mid(value,start,length)`` - Middle bytes of value

**Host Information:**

- ``set [var]:host_lan_addr`` - Host LAN MAC address
- ``set [var]:host_wlan_addr`` - Host WLAN MAC address
- ``set [var]:host_account_id`` - Host account ID
- ``set [var]:host_psid`` - Host PSID
- ``set [var]:host_username`` - Host username
- ``set [var]:host_sysname`` - Host system name

write
-----
Writes data to the file at specified location.

**Syntax:**

.. code-block:: text

    write at|next offset:data
    write at|next offset:xor:data
    write at|next offset:repeat(count,value)
    write at|next offset:[variable]

**Parameters:**

- ``at`` : Absolute address
- ``next`` : Address relative to current pointer
- ``offset`` : Offset in hex (0x100) or decimal ((100))
- ``data`` : Hex string, quoted string, or variable
- ``xor:data`` : XOR data with existing bytes before writing
- ``repeat(count,value)`` : Repeat value count times
- ``[variable]`` : Write variable contents

**Examples:**

.. code-block:: text

    write at 0x100: "Difficulty"
    write next 0:0x446966666963756C7479
    write at 0x43:[myvar]
    write next 0:xor:0xAA

insert
------
Inserts data into the file.

**Syntax:**

.. code-block:: text

    insert at|next offset:data

**Parameters:**

- ``at`` : Absolute insertion point
- ``next`` : Insertion point relative to current pointer
- ``offset`` : Offset in hex or decimal
- ``data`` : Data to insert (hex string, quoted string, or variable)

**Example:**

.. code-block:: text

    insert at 0x100: "NEW_DATA"
    insert next 0:[insert_data]

delete
------
Deletes data from the file.

**Syntax:**

.. code-block:: text

    delete at|next offset:length
    delete at|next offset:until pattern

**Parameters:**

- ``at`` : Absolute start address
- ``next`` : Start address relative to current pointer
- ``offset`` : Offset in hex or decimal
- ``length`` : Number of bytes to delete
- ``until pattern`` : Delete until pattern is found

**Examples:**

.. code-block:: text

    delete at 0x100:10
    delete next 0:until "END"

search
------
Searches for data and sets pointer to match location.

**Syntax:**

.. code-block:: text

    search pattern[:count]
    search next pattern[:count]

**Parameters:**

- ``next`` : Start search from current pointer (optional)
- ``pattern`` : Hex string or quoted string to search for
- ``count`` : Occurrence number (default: 1)

**Examples:**

.. code-block:: text

    search "difficulty"
    search 0x646966666963756C7479:1
    search next 0x010e

copy
----
Copies data within the file.

**Syntax:**

.. code-block:: text

    copy from:to:size

**Parameters:**

- ``from`` : Source address
- ``to`` : Destination address
- ``size`` : Number of bytes to copy

**Example:**

.. code-block:: text

    copy 0x100:0x200:0x50

msgbox
------
Displays message box with variable contents (debugging).

**Syntax:**

.. code-block:: text

    msgbox [variable]

**Example:**

.. code-block:: text

    msgbox [checksum]

endian_swap
-----------
Swaps byte order of data in current range.

**Syntax:**

.. code-block:: text

    endian_swap(mode)

**Parameters:**

- ``mode`` : 2 for 16-bit, 4 for 32-bit, 8 for 64-bit

**Example:**

.. code-block:: text

    endian_swap(4)    ; Swap 32-bit values

decompress
----------
Decompresses data using zlib/deflate.

**Syntax:**

.. code-block:: text

    decompress(offset, wbits)
    decompress(#count, wbits)
    decompress(*, wbits)

**Parameters:**

- ``offset`` : Start offset of compressed data
- ``#count`` : Number of compressed blocks to try
- ``*`` : Try all offsets (with #count)
- ``wbits`` : Window bits (0=auto, 15=zlib, -15=raw deflate)

**Example:**

.. code-block:: text

    decompress(0x100, 0)

compress
--------
Compresses previously decompressed data.

**Syntax:**

.. code-block:: text

    compress(offset)

**Parameters:**

- ``offset`` : Original offset of compressed data

**Example:**

.. code-block:: text

    compress(0x100)

Encryption/Decryption Commands
==============================

Standard Encryption Algorithms
------------------------------

**AES-ECB:**

.. code-block:: text

    decrypt aes_ecb(key)
    encrypt aes_ecb(key)

**AES-CBC:**

.. code-block:: text

    decrypt aes_cbc(key,iv)
    encrypt aes_cbc(key,iv)

**AES-CTR:**

.. code-block:: text

    decrypt aes_ctr(key,iv)
    encrypt aes_ctr(key,iv)

**Camellia:**

.. code-block:: text

    decrypt camellia_ecb(key)
    encrypt camellia_ecb(key)

**DES:**

.. code-block:: text

    decrypt des_ecb(key)
    encrypt des_ecb(key)

**Triple DES-CBC:**

.. code-block:: text

    decrypt des3_cbc(key,iv)
    encrypt des3_cbc(key,iv)

**Blowfish-ECB:**

.. code-block:: text

    decrypt blowfish_ecb(key)
    encrypt blowfish_ecb(key)

**Blowfish-CBC:**

.. code-block:: text

    decrypt blowfish_cbc(key,iv)
    encrypt blowfish_cbc(key,iv)

Custom Encryption Algorithms
----------------------------
**Diablo 3:**

.. code-block:: text

    decrypt diablo3
    encrypt diablo3

**Dynasty Warriors 8 XL:**

.. code-block:: text

    decrypt dw8xl
    encrypt dw8xl

**Silent Hill 3:**

.. code-block:: text

    decrypt silent_hill3
    encrypt silent_hill3

**Need for Speed Undercover:**

.. code-block:: text

    decrypt nfs_undercover
    encrypt nfs_undercover

**Final Fantasy XIII:**

.. code-block:: text

    decrypt ffxiii(mode,key)
    encrypt ffxiii(mode,key)

**RGG Studio (Yakuza):**

.. code-block:: text

    decrypt rgg_studio(key)
    encrypt rgg_studio(key)

**Borderlands 3:**

.. code-block:: text

    decrypt borderlands3(type)
    encrypt borderlands3(type)

**Monster Hunter PSP:**

.. code-block:: text

    decrypt monster_hunter(type)
    encrypt monster_hunter(type)

**Metal Gear Solid 5:**

.. code-block:: text

    decrypt mgs5_tpp(xor_key)
    encrypt mgs5_tpp(xor_key)

**Metal Gear Solid Peace Walker:**

.. code-block:: text

    decrypt mgs_pw
    encrypt mgs_pw

**Metal Gear Solid Base64:**

.. code-block:: text

    decrypt mgs_base64
    encrypt mgs_base64

**Metal Gear Solid HD:**

.. code-block:: text

    decrypt mgs(key)
    encrypt mgs(key)

Notes
=====

- Addresses in parentheses are treated as decimal: ``(100)`` = decimal 100
- ``0x`` prefix indicates hexadecimal: ``0x100`` = hex 100
- Variables are enclosed in brackets: ``[variable_name]``
- Strings are enclosed in double quotes: ``"text"``
- Hex data is written as continuous hex string: ``446966666963756C7479``
- The ``range`` must be set before range-based operations
- ``carry`` setting affects ``add()`` and ``wadd()`` operations
- Most commands accept variables in place of literal values
