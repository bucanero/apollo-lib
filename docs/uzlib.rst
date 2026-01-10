:mod:`uzlib` -- zlib compression functions
==================================

.. module:: uzlib
   :synopsis: zlib compression functions

The ``uzlib`` module provides compression and decompression functions using the zlib library. It supports standard zlib compression as well as specialized functions for working with embedded compressed data in files.

Functions
---------

.. py:function:: decompress(data, wbits=zlib.MAX_WBITS)

   Decompresses zlib-compressed data.

   :param bytes data: Compressed data to decompress
   :param int wbits: Window size bits (default: zlib.MAX_WBITS)
   
     - Positive values: Base two logarithm of the window size (8..15)
     - Add 16 to decode gzip format
     - Add 32 to decode zlib and gzip formats (automatic header detection)
   
   :return: Decompressed data
   :rtype: bytes
   :raises ValueError: If decompression fails

.. py:function:: compress(data, wbits=zlib.MAX_WBITS, level=zlib.Z_BEST_COMPRESSION)

   Compresses data using zlib.

   :param bytes data: Data to compress
   :param int wbits: Window size bits (default: zlib.MAX_WBITS)
   
     - Positive values: Base two logarithm of the window size (8..15)
     - Add 16 to encode gzip format
   
   :param int level: Compression level (0-9, default: zlib.Z_BEST_COMPRESSION)
   
     - 0: No compression
     - 1: Best speed
     - 9: Best compression
   
   :return: Compressed data
   :rtype: bytes
   :raises ValueError: If compression fails

.. py:function:: offzip(data, wbits=zlib.MAX_WBITS)

   Scans data for embedded zlib-compressed blocks (similar to offzip tool).

   This function searches through binary data to find and verify embedded
   zlib-compressed blocks, useful for reverse engineering game files or
   other binary formats with embedded compression.

   :param bytes data: Binary data to scan for compressed blocks
   :param int wbits: Window size bits to use for decompression attempts
     (default: zlib.MAX_WBITS)
   :return: List of tuples, each containing information about found
     compressed blocks. Each tuple has format:
     ``(offset, compressed_size, decompressed_size, wbits)``
   :rtype: list of tuples

.. py:function:: packzip(data, block_info, new_data)

   Replaces a compressed block in data with newly compressed content.

   This function is useful for patching files that contain embedded
   compressed blocks. It takes the original data, information about a
   compressed block (as returned by ``offzip``), and new data to compress
   and insert at the specified location.

   :param bytes data: Original binary data containing compressed block
   :param tuple block_info: Information about the block to replace
     (as returned by ``offzip``). Format: ``(offset, compressed_size, decompressed_size, wbits)``
   :param bytes new_data: New data to compress and insert
   :return: Modified data with replaced compressed block
   :rtype: bytes
   :raises ValueError: If compression fails or invalid parameters provided

Usage Notes
-----------

- The module uses zlib's standard compression/decompression algorithms
- For gzip format support, add 16 to the wbits parameter
- The ``offzip`` and ``packzip`` functions are particularly useful for
  working with game files and other binary formats that embed compressed data
- All functions work with ``bytes`` objects; use ``bytearray`` for mutable data
- Compression levels range from 0 (no compression) to 9 (maximum compression)

Examples
--------

Basic compression and decompression:

.. code-block:: python

   import uzlib
   
   # Compress data
   data = b"Hello, World! " * 100
   compressed = uzlib.compress(data, level=6)
   
   # Decompress data
   decompressed = uzlib.decompress(compressed)
   
   # Use gzip format
   gzip_compressed = uzlib.compress(data, wbits=uzlib.MAX_WBITS + 16)
   gzip_decompressed = uzlib.decompress(gzip_compressed)

Working with embedded compressed blocks:

.. code-block:: python

   import uzlib
   
   # Read a file that may contain embedded compressed blocks
   with open("game_data.bin", "rb") as f:
       data = f.read()
   
   # Find all compressed blocks
   blocks = uzlib.offzip(data)
   
   if blocks:
       # Examine the first found block
       offset, comp_size, decomp_size, wbits = blocks[0]
       print(f"Found block at 0x{offset:08x}: {comp_size} -> {decomp_size} bytes")
       
       # Extract and decompress the block
       compressed_block = data[offset:offset + comp_size]
       decompressed_data = uzlib.decompress(compressed_block, wbits)
       
       # Modify the decompressed data
       modified_data = decompressed_data.replace(b"old", b"new")
       
       # Repack with modified data
       new_data = uzlib.packzip(data, blocks[0], modified_data)
       
       # Save the modified file
       with open("game_data_modified.bin", "wb") as f:
           f.write(new_data)

Error Handling
--------------

All functions raise ``ValueError`` on failure. Common failure reasons include:

- Invalid or corrupted compressed data
- Insufficient memory for decompression
- Invalid window bits parameter
- Invalid block information in ``packzip``

Notes on Window Bits (wbits)
----------------------------

The wbits parameter controls the compression window size and format:

===============  ============================================================
Value            Format
===============  ============================================================
8..15            Raw deflate format with specified window size
16 + (8..15)     Gzip format with specified window size
32 + (8..15)     Auto-detect between zlib or gzip format
===============  ============================================================

Larger window sizes generally provide better compression but use more memory.
The default ``MAX_WBITS`` is typically 15, which corresponds to a 32KB window.
