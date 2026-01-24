===================================
Python scripting with Apollo
===================================

Apollo includes a built-in MicroPython interpreter that allows you to write custom scripts
to manipulate save data. The following global variables are available to your scripts:

Global variables
----------------

.. py:data:: savedata

   The raw binary save data being modified. You can read from and write to this
   bytearray to manipulate the save data.

   :type: bytearray
   :value: A byte array containing the raw binary save data.

.. py:data:: host_file_path

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
-----------------------------

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
