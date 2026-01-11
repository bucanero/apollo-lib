:mod:`apollo` -- binary data manipulation functions
===================================================

.. module:: apollo
   :synopsis: binary data manipulation functions

The ``apollo`` module provides utility functions for binary data manipulation, particularly useful for working with game save files and binary patches. It includes search functions, endianness conversion, and Save Wizard code application.

Constants
---------

.. py:data:: version

   Returns the version string of the Apollo library.

   :type: str
   :value: Current Apollo library version (e.g., "1.0.0")

Functions
---------

.. py:function:: endian_swap(data, word_size=4)

   Performs endianness swapping on binary data in-place.

   This function modifies the input buffer by swapping byte order for
   multi-byte words. Useful for converting between little-endian and
   big-endian formats commonly found in different platforms.

   :param bytearray data: Binary data to modify (modified in-place)
   :param int word_size: Size of words to swap (2, 4, or 8 bytes)
   :return: The modified data buffer
   :rtype: bytearray
   :raises ValueError: If word_size is not 2, 4, or 8

   **Example**::

      # Swap 32-bit integers from little-endian to big-endian
      data = bytearray(b'\x01\x00\x00\x00\x02\x00\x00\x00')
      apollo.endian_swap(data, 4)
      # data now contains: b'\x00\x00\x00\x01\x00\x00\x00\x02'

.. py:function:: search(data, pattern, count=1)

   Finds the position of a pattern within binary data.

   Searches forward through the data for the specified pattern and returns
   the byte offset of the nth occurrence. Useful for locating specific
   structures or values in binary files.

   :param bytes data: Binary data to search
   :param bytes pattern: Pattern to search for
   :param int count: Occurrence number to find (1 = first occurrence)
   :return: Byte offset of the found pattern, or ``None`` if not found
   :rtype: int or None

   **Example**::

      data = b"HelloWorldHelloWorld"
      offset = apollo.search(data, b"World", 2)
      # Returns 13 (offset of second "World")

.. py:function:: reverse_search(data, pattern, count=1)

   Finds the position of a pattern searching backward from the end.

   Searches backward through the data for the specified pattern and returns
   the byte offset of the nth occurrence from the start. Useful for finding
   the last occurrence of a pattern in a file.

   :param bytes data: Binary data to search
   :param bytes pattern: Pattern to search for
   :param int count: Occurrence number to find from the end (1 = last occurrence)
   :return: Byte offset of the found pattern, or ``None`` if not found
   :rtype: int or None

   **Example**::

      data = b"HelloWorldHelloWorld"
      offset = apollo.reverse_search(data, b"World", 1)
      # Returns 13 (offset of last "World")

.. py:function:: apply_savewizard(data, code)

   Applies a Save Wizard code to binary data.

   Parses and applies Save Wizard (PS4 save editor) format cheat codes
   to modify binary data in-place. Supports various code types including
   byte, short, integer writes, and pointer chains.

   For more information on Save Wizard code format, refer to :doc:`savewizard`.

   :param bytearray data: Binary data to modify (modified in-place)
   :param str code: Save Wizard code string
   :return: Number of bytes written/modified
   :rtype: int

   **Save Wizard Code Format**:
   
   :doc:`Save Wizard codes</savewizard>` use a specific format::
   
      Example: "20001000 0000ABCD"
      
      First 32-bit value: Address/offset and code type
      Second 32-bit value: Value to write

   **Example**::

      # Write 0x0000ABCD at offset 0x1000
      data = bytearray(0x2000)  # 8KB buffer
      bytes_written = apollo.apply_savewizard(data, "20001000 0000ABCD")
      # Writes ABCD at offset 0x1000

Usage Notes
-----------

- The ``endian_swap`` and ``apply_savewizard`` functions modify data in-place
  and require ``bytearray`` or ``memoryview`` objects
- The search functions work with ``bytes`` objects and return offsets
- All offsets are zero-based (first byte is offset 0)
- Pattern searches are exact byte matches; no wildcards or regex support
- Save Wizard codes must be in the exact format used by Save Wizard software

Examples
--------

Working with game save files::

   import apollo
   
   # Read a save file
   with open("savegame.dat", "rb") as f:
       save_data = bytearray(f.read())
   
   # Search for a known value pattern
   money_offset = apollo.search(save_data, b"MONEY", 1)
   if money_offset:
       print(f"Found money at offset: 0x{money_offset:08x}")
   
   # Apply a cheat code to modify money
   code_applied = apollo.apply_savewizard(save_data, "0x210001234 0x00989680")
   print(f"Applied {code_applied} bytes of modifications")
   
   # Save modified file
   with open("savegame_modified.dat", "wb") as f:
       f.write(save_data)

Endianness conversion::

   import apollo
   
   # Read data that might be in wrong endianness
   data = bytearray(b'\x78\x56\x34\x12')  # Appears as 0x78563412 in little-endian
   
   # Convert 32-bit values from big-endian to little-endian
   apollo.endian_swap(data, 4)
   # data now contains: b'\x12\x34\x56\x78' (0x12345678)

Finding multiple occurrences::

   import apollo
   
   data = b"ABC123DEF123GHI123JKL"
   
   # Find first occurrence
   first = apollo.search(data, b"123", 1)  # Returns 3
   
   # Find second occurrence  
   second = apollo.search(data, b"123", 2)  # Returns 9
   
   # Find last occurrence
   last = apollo.reverse_search(data, b"123", 1)  # Returns 15

Error Handling
--------------

- ``endian_swap`` raises ``ValueError`` for invalid word sizes
- Search functions return ``None`` when pattern is not found
- ``apply_savewizard`` may fail silently for invalid codes; check return value
  for number of bytes actually modified
