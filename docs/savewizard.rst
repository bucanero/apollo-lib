=================================
Save Wizard code format Reference
=================================

.. contents::
   :depth: 3
   :local:

Overview
========

The Save Wizard code format is a proprietary cheat/editing system used to modify game save files. This document outlines the code structure and available code types based on the provided C implementation.

Code Structure
==============

Each code line follows a specific format:

::

   TXXXXXX YYYYYYYY

Where:

- ``T`` = Code type identifier (0-9, A-D)
- ``XXXXXX`` = 6-character hexadecimal offset/address
- ``YYYYYYYY`` = 8-character hexadecimal value/parameter

Some codes span multiple lines with additional data.

Code Types
==========

Type 0-2: Direct Write Operations
----------------------------------

**Format:** ``BTXXXXXX YYYYYYYY``

**Parameters:**

- ``B`` = Byte size (0=8-bit, 1=16-bit, 2=32-bit)
- ``T`` = Address type (0=Normal, 8=Offset From Pointer)
- ``XXXXXX`` = Address/offset
- ``YYYYYYYY`` = Value to write

**Description:** Direct memory write operations. Writes the specified value to the given address.

Type 3: Increase/Decrease Write
--------------------------------

**Format:** ``3BYYYYYY XXXXXXXX``

**Parameters:**

- ``B`` = Operation type and size:

  +--------+---------------------------------------+
  | Value  | Operation                             |
  +========+=======================================+
  | 0      | Add 1 byte                            |
  +--------+---------------------------------------+
  | 1      | Add 2 bytes                           |
  +--------+---------------------------------------+
  | 2      | Add 4 bytes                           |
  +--------+---------------------------------------+
  | 3      | Add 8 bytes                           |
  +--------+---------------------------------------+
  | 4      | Subtract 1 byte                       |
  +--------+---------------------------------------+
  | 5      | Subtract 2 bytes                      |
  +--------+---------------------------------------+
  | 6      | Subtract 4 bytes                      |
  +--------+---------------------------------------+
  | 7      | Subtract 8 bytes                      |
  +--------+---------------------------------------+
  | 8      | Offset from pointer; Add 1 byte       |
  +--------+---------------------------------------+
  | 9      | Offset from pointer; Add 2 bytes      |
  +--------+---------------------------------------+
  | A      | Offset from pointer; Add 4 bytes      |
  +--------+---------------------------------------+
  | B      | Offset from pointer; Add 8 bytes      |
  +--------+---------------------------------------+
  | C      | Offset from pointer; Subtract 1 byte  |
  +--------+---------------------------------------+
  | D      | Offset from pointer; Subtract 2 bytes |
  +--------+---------------------------------------+
  | E      | Offset from pointer; Subtract 4 bytes |
  +--------+---------------------------------------+
  | F      | Offset from pointer; Subtract 8 bytes |
  +--------+---------------------------------------+

- ``YYYYYY`` = Address
- ``XXXXXXXX`` = Value to add/subtract

**Description:** Increments or decrements the value at the specified address by the given amount.

Type 4: Multi-Write
-------------------

**Format:** ``4TXXXXXX YYYYYYYY`` (First line)
Additional lines vary by subtype

**Parameters:**

- ``T`` = Type and size:

  +--------+---------------------------+------------------+
  | Value  | Operation                 | Address Type     |
  +========+===========================+==================+
  | 0      | 8-bit                     | Normal           |
  +--------+---------------------------+------------------+
  | 8      | 8-bit                     | Offset from      |
  |        |                           | Pointer          |
  +--------+---------------------------+------------------+
  | 1      | 16-bit                    | Normal           |
  +--------+---------------------------+------------------+
  | 9      | 16-bit                    | Offset from      |
  |        |                           | Pointer          |
  +--------+---------------------------+------------------+
  | 2      | 32-bit                    | Normal           |
  +--------+---------------------------+------------------+
  | A      | 32-bit                    | Offset from      |
  |        |                           | Pointer          |
  +--------+---------------------------+------------------+
  | 4      | 1-byte incremental        | Normal           |
  +--------+---------------------------+------------------+
  | C      | 1-byte incremental        | Offset from      |
  |        |                           | Pointer          |
  +--------+---------------------------+------------------+
  | 5      | 2-byte incremental        | Normal           |
  +--------+---------------------------+------------------+
  | D      | 2-byte incremental        | Offset from      |
  |        |                           | Pointer          |
  +--------+---------------------------+------------------+
  | 6      | 4-byte incremental        | Normal           |
  +--------+---------------------------+------------------+
  | E      | 4-byte incremental        | Offset from      |
  |        |                           | Pointer          |
  +--------+---------------------------+------------------+

- ``XXXXXX`` = Starting address
- ``YYYYYYYY`` = Starting value

**For incremental types (4,5,6,C,D,E):**

Second line: ``NNNNWWWW VVVVVVVV``

- ``NNNN`` = Number of times to write
- ``WWWW`` = Address increment per write
- ``VVVVVVVV`` = Value increment per write

**Description:** Writes values to multiple addresses with optional increments.

Type 5: Copy Bytes
------------------

**Format (two lines):**

::

   5TXXXXXX ZZZZZZZZ
   5TYYYYYY 00000000

**Parameters:**

- ``T`` = Address type (0=From start, 8=From pointer)
- First line:
  - ``XXXXXX`` = Source offset
  - ``ZZZZZZZZ`` = Number of bytes to copy
- Second line:
  - ``YYYYYY`` = Destination offset

**Description:** Copies bytes from one memory location to another.

Type 6: Pointer Operations (Special Mega Code)
-----------------------------------------------

**Format:** ``6TWX0Y0Z VVVVVVVV``

**Parameters:**

- ``T`` = Data size:

  +--------+-------------------+
  | Value  | Data Size         |
  +========+===================+
  | 0      | 8-bit             |
  +--------+-------------------+
  | 1      | 16-bit            |
  +--------+-------------------+
  | 2      | 32-bit            |
  +--------+-------------------+
  | 8      | 8-bit (search)    |
  +--------+-------------------+
  | 9      | 16-bit (search)   |
  +--------+-------------------+
  | A      | 32-bit (search)   |
  +--------+-------------------+

- ``W`` = Main operation:

  +--------+-------------------------------------------+
  | Value  | Operation                                 |
  +========+===========================================+
  | 0X     | Read address from file                    |
  |        | (X=0:none, 1:add, 2:multiply)             |
  +--------+-------------------------------------------+
  | 1X     | Move pointer from obtained address        |
  |        | (X=0:add, 1:subtract, 2:multiply)         |
  +--------+-------------------------------------------+
  | 2X     | Move pointer directly                     |
  |        | (X=0:add, 1:subtract, 2:multiply)         |
  +--------+-------------------------------------------+
  | 4X     | Write value                               |
  |        | (X=0 at read address, 1 at pointer addr)  |
  +--------+-------------------------------------------+

- ``X`` = Operation modifier
- ``Y`` = Flag relative to read address (0=absolute, 1=pointer)
- ``Z`` = Flag relative to current pointer
- ``VVVVVVVV`` = Data value

**Description:** Complex pointer manipulation and conditional operations.

Type 7: Conditional Write (No More/Less Than)
----------------------------------------------

**Format:** ``7BYYYYYY XXXXXXXX``

**Parameters:**

- ``B`` = Operation type:

  +--------+-----------------------------------+
  | Value  | Operation                         |
  +========+===================================+
  | 0      | No Less Than: 1 byte              |
  +--------+-----------------------------------+
  | 1      | No Less Than: 2 bytes             |
  +--------+-----------------------------------+
  | 2      | No Less Than: 4 bytes             |
  +--------+-----------------------------------+
  | 4      | No More Than: 1 byte              |
  +--------+-----------------------------------+
  | 5      | No More Than: 2 bytes             |
  +--------+-----------------------------------+
  | 6      | No More Than: 4 bytes             |
  +--------+-----------------------------------+
  | 8      | Offset from pointer; No Less      |
  |        | Than: 1 byte                      |
  +--------+-----------------------------------+
  | 9      | Offset from pointer; No Less      |
  |        | Than: 2 bytes                     |
  +--------+-----------------------------------+
  | A      | Offset from pointer; No Less      |
  |        | Than: 4 bytes                     |
  +--------+-----------------------------------+
  | C      | Offset from pointer; No More      |
  |        | Than: 1 byte                      |
  +--------+-----------------------------------+
  | D      | Offset from pointer; No More      |
  |        | Than: 2 bytes                     |
  +--------+-----------------------------------+
  | E      | Offset from pointer; No More      |
  |        | Than: 4 bytes                     |
  +--------+-----------------------------------+

- ``YYYYYY`` = Address
- ``XXXXXXXX`` = Value to write conditionally

**Description:** Writes only if current value meets condition (greater/less than threshold).

Type 8: Forward Search
----------------------

**Format:** ``8TZZXXXX YYYYYYYY`` + additional data lines

**Parameters:**

- ``T`` = Address type (0=Normal, 8=Offset From Pointer)
- ``ZZ`` = Number of occurrences to find before setting pointer
- ``XXXX`` = Length of data to match (in bytes)
- ``YYYYYYYY`` = Search pattern (first 4 bytes)

**Additional lines:** Continue search pattern data (8 bytes per line)

**Description:** Searches forward for a pattern and sets the pointer to its location.

Type 9: Pointer Manipulation
----------------------------

**Format:** ``9Y000000 XXXXXXXX``

**Parameters:**

- ``Y`` = Operation:

  +--------+-----------------------------------------+
  | Value  | Operation                               |
  +========+=========================================+
  | 0      | Set pointer to Big Endian value at      |
  |        | address                                 |
  +--------+-----------------------------------------+
  | 1      | Set pointer to Little Endian value at   |
  |        | address                                 |
  +--------+-----------------------------------------+
  | 2      | Add X to pointer                        |
  +--------+-----------------------------------------+
  | 3      | Subtract X from pointer                 |
  +--------+-----------------------------------------+
  | 4      | Set pointer to end of file minus X      |
  +--------+-----------------------------------------+
  | 5      | Set pointer to X                        |
  +--------+-----------------------------------------+
  | D      | Set end pointer to X                    |
  +--------+-----------------------------------------+
  | E      | Set end pointer to pointer + X          |
  +--------+-----------------------------------------+

- ``XXXXXXXX`` = Value/address

**Description:** Direct pointer manipulation operations.

Type A: Bulk Memory Write
-------------------------

**Format:** ``ATxxxxxx yyyyyyyy`` + data lines

**Parameters:**

- ``T`` = Address type (0=Normal, 8=Offset From Pointer)
- ``xxxxxx`` = Destination address
- ``yyyyyyyy`` = Size of data to write

**Additional lines:** Data to write (8 bytes per line)

**Description:** Writes a block of data to memory.

Type B: Backward Search
-----------------------

**Format:** ``BTCCYYYY XXXXXXXX`` + additional data lines

**Parameters:**

- ``T`` = Offset type (0=Default, 8=Offset from Pointer)
- ``CC`` = Number of occurrences to find
- ``YYYY`` = Length of bytes to search (1=1 byte, 2=2 bytes, etc.)
- ``XXXXXXXX`` = Search pattern (first 4 bytes)

**Additional lines:** Continue search pattern data

**Description:** Searches backward from end pointer for a pattern.

Type C: Address Byte Search
---------------------------

**Format:** ``CBFFYYYY XXXXXXXX``

**Parameters:**

- ``B`` = Search type:

  +--------+-----------------------------------------+
  | Value  | Search Type                             |
  +========+=========================================+
  | 0      | Search forwards from given address      |
  +--------+-----------------------------------------+
  | 4      | Search from 0x0 to given address        |
  +--------+-----------------------------------------+
  | 8      | Offset from pointer; search forwards    |
  +--------+-----------------------------------------+
  | C      | Offset from pointer; search from 0x0    |
  |        | to address                              |
  +--------+-----------------------------------------+

- ``FF`` = Number of occurrences to find
- ``YYYY`` = Length of bytes to use from search address
- ``XXXXXXXX`` = Address containing bytes to search for

**Description:** Uses bytes from a specific address as a search pattern.

Type D: Conditional Skip (Byte Test)
------------------------------------

**Format:** ``DBYYYYYY CCZDXXXX``

**Parameters:**

- ``B`` = Offset type (0=Normal, 8=Offset from Pointer)
- ``YYYYYY`` = Address to test
- ``CC`` = Number of code lines to skip if test fails
- ``Z`` = Data type (0=16-bit, 1=8-bit)
- ``D`` = Test operation:

  +--------+-------------------+
  | Value  | Operation         |
  +========+===================+
  | 0      | Equal             |
  +--------+-------------------+
  | 1      | Not equal         |
  +--------+-------------------+
  | 2      | Greater than      |
  +--------+-------------------+
  | 3      | Less than         |
  +--------+-------------------+

- ``XXXX`` = Value to test against

**Description:** Tests a memory location and skips following lines if condition fails.

Common Patterns
===============

Pointer Usage
-------------

- Types 8, B, C set the ``pointer`` variable
- Types 0-7, A can use pointer offsets when ``T`` = 8
- Type 9 manipulates pointers directly

Multi-line Codes
----------------

- Types 4, 5, 6, 8, A, B, C span multiple lines
- Each additional line contains 8 bytes of data or parameters

Endianness
----------

- Most operations use little-endian (host byte order)
- Search operations (8, B) use big-endian for patterns
- Type 9 supports both endianness modes

Notes
=====

1. All offsets and addresses are hexadecimal
2. Values are typically little-endian unless specified
3. The pointer variable persists between code executions
4. Search operations can fail, causing code skipping
5. Conditional operations (D) allow for branching logic




Save Wizard Code Examples
=========================

Type 0: 8-bit Direct Write
--------------------------

**Code:** 

::

   00A01234 000000FF 

**Description:** Write value ``0xFF`` to
address ``0xA01234``

**Code:** 

::

   08123456 00000042 

**Description:** Write value ``0x42`` to
pointer offset ``0x123456``

--------------

Type 1: 16-bit Direct Write
---------------------------

**Code:** 

::

   10B00000 00001234

**Description:** Write value ``0x1234``
to address ``0xB00000``

**Code:** 

::

   18550000 0000ABCD

**Description:** Write value ``0xABCD``
to pointer offset ``0x550000``

--------------

Type 2: 32-bit Direct Write
---------------------------

**Code:** 

::

   20080000 12345678

**Description:** Write value ``0x12345678`` to address ``0x080000``

**Code:** 

::

   28300000 87654321

**Description:** Write value ``0x87654321`` to pointer offset ``0x300000``


Type 3: Increase/Decrease Write
-------------------------------

**Code:** 

::
   
   30012345 0000000A
   
**Description:** Add ``0x0A`` (10) to
the 1-byte value at address ``0x012345``

**Code:**

::
   
   32A54321 00000064
   
**Description:** Add ``0x64`` (100) to
the 4-byte value at address ``0xA54321``

**Code:** 

::
   
   34800000 00000005
   
**Description:** Subtract ``0x05`` (5)
from the 1-byte value at address ``0x800000``

**Code:** 

::
   
   38876543 00002710
   
**Description:** (Pointer offset) Add
``0x2710`` (10000) to the 1-byte value

**Code:** 

::
   
   3F200000 00000001
   
**Description:** (Pointer offset)
Subtract ``0x01`` from the 8-byte value at offset ``0x200000``



Type 4: Multi-Write
-------------------

Simple Multi-Write (8-bit)
~~~~~~~~~~~~~~~~~~~~~~~~~~

::

   40000100 00000001

**Description:** Write ``0x01`` to address ``0x000100`` (8-bit)

Incremental Multi-Write (32-bit)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

::

   42001000 00000001
   00020000 00000001

**Description:** Write increasing values starting at ``0x00000001``: 

- Write to ``0x001000``, then ``0x001200``, then ``0x001400``\ … 
- Each write increases address by ``0x20000`` and value by ``0x01``

Complex Multi-Write with Pointer
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

::

   4C030000 00000010
   00050000 00000001

**Description:** (Pointer offset) Write starting value ``0x10``: 

- Write 1-byte values ``0x10``, ``0x11``, ``0x12``\ … 
- To addresses: pointer+\ ``0x030000``, pointer+\ ``0x035000``, pointer+\ ``0x03A000``\ …

--------------

Type 5: Copy Bytes
------------------

::

   50001000 00000010
   50002000 00000000

**Description:** Copy 16 bytes from address ``0x001000`` to address
``0x002000``

::

   58012345 00000100
   58654321 00000000

**Description:** Copy 256 bytes from pointer offset ``0x012345`` to
pointer offset ``0x654321``

--------------

Type 6: Pointer Operations
--------------------------

**Code:**

::
   
   60000001 00100000
   
**Description:** Read 32-bit value from
address ``0x100000`` into pointer

**Code:**

::
   
   61000000 00000004
   
**Description:** Add ``0x04`` to pointer
value from obtained address

**Code:**

::
   
   62000000 00001000
   
**Description:** Move pointer forward by
``0x1000`` bytes

**Code:**

::
   
   64010001 000000FF
   
**Description:** Write ``0xFF`` (8-bit)
to pointer address

**Complex Example:**

::

   60200001 00100000  // Read address from 0x100000
   61000000 00000004  // Add 4 to pointer
   64010001 000000FF  // Write 0xFF at pointer address

--------------

Type 7: Conditional Write
-------------------------

**Code:**

::
   
   70012345 00000064
   
**Description:** Write ``0x64`` to
address ``0x012345`` only if current value < ``0x64`` (No More Than, 1
byte)

**Code:**

::
   
   71A00000 00002710
   
**Description:** Write ``0x2710`` to
address ``0xA00000`` only if current value > ``0x2710`` (No Less Than, 2
bytes)

**Code:**

::
   
   78876543 3B9ACA00
   
**Description:** (Pointer offset) Write
``0x3B9ACA00`` (1,000,000,000) only if current value < that amount (No
More Than, 1 byte)

--------------

Type 8: Forward Search
----------------------

Simple Search
~~~~~~~~~~~~~

::

   80000104 41424344

**Description:** Search for pattern “ABCD” (ASCII) and set pointer to
first occurrence

Complex Search
~~~~~~~~~~~~~~

::

   81000208 48656C6C  // "Hell"
   6F2C576F 726C6421  // "o,World!" (spread across two lines)

**Description:** Search for “Hello,World!” (12 bytes), find 2nd
occurrence

Search with Pointer Offset
~~~~~~~~~~~~~~~~~~~~~~~~~~

::

   88000104 504C4159  // "PLAY"

**Description:** Search for “PLAY” starting from pointer location

--------------

Type 9: Pointer Manipulation
----------------------------

**Code:**

::
   
   90000000 00100000
   
**Description:** Set pointer to 32-bit
Big Endian value at address ``0x100000``

**Code:**

::
   
   92000000 00000004
   
**Description:** Move pointer forward by
4 bytes

**Code:**

::
   
   93000000 00001000
   
**Description:** Move pointer back by
4096 bytes

**Code:**

::
   
   94000000 00000100
   
**Description:** Set pointer to 256
bytes from end of file

**Code:**

::
   
   95000000 00C00000
   
**Description:** Set pointer directly to
``0xC00000``

**Code:**

::
   
   9D000000 00100000
   
**Description:** Set end pointer to
``0x100000``

**Code:**

::
   
   9E000000 00001000
   
**Description:** Set end pointer to
current pointer + ``0x1000``

--------------

Type A: Bulk Memory Write
-------------------------

Write 8 bytes
~~~~~~~~~~~~~

::

   A0001000 00000008
   11223344 55667788

**Description:** Write 8 bytes of data to address ``0x001000``

Write 16 bytes with pointer
~~~~~~~~~~~~~~~~~~~~~~~~~~~

::

   A8300000 00000010
   41414141 42424242  // "AAAA", "BBBB"
   43434343 44444444  // "CCCC", "DDDD"

**Description:** Write 16 bytes “AAAABBBBCCCCDDDD” to pointer offset
``0x300000``

--------------

Type B: Backward Search
-----------------------

**Code:**

::
   
   B0010104 656E6421
   
**Description:** Search backward for “end!” from end of file

**Code:**

::
   
   B8010208 46494E21  // "FIN!"
   21544E45 4E490000  // "!TENI" (pattern spread)

**Description:** Search backward for 8-byte pattern starting from
pointer

--------------

Type C: Address Byte Search
---------------------------

**Code:**

::
   
   C0010404 00100000
   
**Description:** Use 4 bytes from
address ``0x100000`` as search pattern, find first occurrence

**Code:**

::
   
   C4010808 00200000
   
**Description:** Search from beginning
of file to address ``0x200000`` using 8 bytes from that address

**Code:**

::
   
   C8020410 00300000
   
**Description:** (Pointer offset) Search
forward using 4 bytes from pointer+\ ``0x300000``, find 2nd occurrence

--------------

Type D: Conditional Skip
------------------------

**Code:**

::
   
   D0123456 02003E80

**Description:** Test if 16-bit value at
``0x123456`` equals ``0x3E80``, skip 2 lines if false

**Code:**

::
   
   D8654321 0110FF00

**Description:** Test if 8-bit value at
pointer+\ ``0x654321`` is not equal to ``0x00``, skip 1 line if false

**Code:**

::
   
   D0A00000 03023E80

**Description:** Test if 16-bit value at
``0xA00000`` is greater than ``0x3E80``, skip 3 lines if false

**Code:**

::
   
   D8000000 01014000

**Description:** Test if 8-bit value at
pointer is less than ``0x40``, skip 1 line if false

--------------

Real-World Examples
-------------------

Example 1: Set Max Health
~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: text

   20012345 0000270F  // Write 9999 (0x270F) to health address

Example 2: Unlimited Ammo
~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: text

   000ABCDE 00000063  // Set ammo to 99 (0x63) at 8-bit address

Example 3: Unlock All Weapons
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: text

   40010000 00000001  // Start writing 0x01 (unlocked flag)
   00040000 00000000  // Write to 256 weapons, each 4 bytes apart

Example 4: Pointer-based Money Hack
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: text

   80000104 4D6F6E79  // Search for "Mony" (part of "Money")
   62000000 00000004  // Move pointer 4 bytes forward
   64020001 05F5E0FF  // Write 99,999,999 (0x05F5E0FF) at pointer

Example 5: Conditional God Mode
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: text

   D0123456 01010001  // If health != 1
   00012345 00000001  // Then set health to 1 (skip if already 1)

Example 6: Complex Inventory Editor
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: text

   80000208 496E7665  // Search for "Inve"
   6E746F72 79202020  // "ntory   " (complete the pattern)
   42000000 00000001  // Start writing item ID 1
   00000004 00000001  // Write to 256 slots, increment by 1

Example 7: Experience Multiplier
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: text

   30054321 0000000A  // Add 10 to current XP (4-byte)
   D0543210 02026400  // If XP < 100,000 (0x186A0)
   92000000 00000004  // Skip next 4 lines (don't cap)
   94000000 00000004  // Actually, just 4 lines from end
   000186A0 00000000  // This won't execute due to skip

These examples demonstrate practical applications of each code type for
game save modification. The actual addresses and values would need to be
determined for specific games through reverse engineering or
community-shared codes.
