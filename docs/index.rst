Apollo Save Tool documentation
==============================

|Downloads| |Release| |License| |Twitter|

`The Apollo Save Tool library <https://github.com/bucanero/apollo-lib>`__ and :doc:`command-line tools </cli>` implement a save-data patch engine
that supports:

- :doc:`Save Wizard / Game Genie codes </savewizard>`
- :doc:`Bruteforce Save Data scripts </bsd>`
- `Python scripts (using MicroPython) <#python-scripting>`__

Reference documentation
-----------------------

.. toctree::
   :maxdepth: 2

   cli.rst
   savepatch.rst
   bsd.rst
   savewizard.rst

Apollo Save Tool C library 
--------------------------

The library is cross-platform (PS2 / PS3 / PS4 / PSP / PS Vita / Win / Linux / macOS) and is required to build: 

- `Apollo Save Tool PS2 <https://github.com/bucanero/apollo-ps2>`__ 
- `Apollo Save Tool PS3 <https://github.com/bucanero/apollo-ps3>`__ 
- `Apollo Save Tool PS4 <https://github.com/bucanero/apollo-ps4>`__ 
- `Apollo Save Tool PSP <https://github.com/bucanero/apollo-psp>`__ 
- `Apollo Save Tool PS Vita <https://github.com/bucanero/apollo-vita>`__

Python scripting
----------------

Apollo includes a built-in MicroPython interpreter that allows you to write custom scripts
to manipulate save data. The following global variables are available to your scripts:

Global variables
~~~~~~~~~~~~~~~~

.. py:data:: savedata

   The raw binary save data being modified. You can read from and write to this
   bytearray to manipulate the save data.

   :type: bytearray
   :value: A byte array containing the raw binary save data.

.. py:data:: host_file_name

   :type: bytearray
   :value: The full path of the save data file being modified. (e.g., "/user/savegames/SAVEDATA.DAT")

.. py:data:: host_sys_name

   :type: bytearray
   :value: The user-defined system name. (e.g., "PS3-1234")

.. py:data:: host_username

   :type: bytearray
   :value: Current username. (e.g., "player1")

.. py:data:: host_psid

   :type: bytearray
   :value: The system's PSID value. (e.g., ``b"\x12\x34\x56\x78\x9A..."``)

.. py:data:: host_account_id

   :type: bytearray
   :value: The user's account ID. (e.g., ``b"\x12\x34\x56\x78\x9A\xBC\xDE\xF0"``)

.. py:data:: host_lan_addr

   :type: bytearray
   :value: The system's LAN MAC address. (e.g., ``b"\x12\x34\x56\x78\x9A\xBC"``)

.. py:data:: host_wlan_addr

   :type: bytearray
   :value: The system's WLAN MAC address. (e.g., ``b"\x12\x34\x56\x78\x9A\xBC"``)

Libraries and micro-libraries
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

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
.. |Twitter| image:: https://img.shields.io/twitter/follow/dparrino?label=Follow
   :target: https://x.com/dparrino
