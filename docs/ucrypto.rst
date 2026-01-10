:mod:`ucrypto` -- encryption related functions
=====================================

.. module:: ucrypto
   :synopsis: encryption related functions

The ``ucrypto`` module provides cryptographic functions for both generic algorithms and game-specific custom encryption schemes. All functions operate in-place on bytearray/memoryview objects.

Constants
---------

.. py:data:: DECRYPT
   :value: 0

   Constant representing decryption mode for functions that support encryption/decryption.

.. py:data:: ENCRYPT
   :value: 1

   Constant representing encryption mode for functions that support encryption/decryption.

Game-Specific Cryptography Functions
------------------------------------

These functions implement custom encryption schemes used by various video games.

.. py:function:: diablo3(enc_mode, data)

   Encrypts or decrypts data using Diablo 3's encryption algorithm.

   :param int enc_mode: Operation mode (``ENCRYPT`` or ``DECRYPT``)
   :param bytearray data: Data buffer to process (modified in-place)
   :return: The enc_mode parameter
   :rtype: int

.. py:function:: dw8xl(data)

   Encodes data using Dynasty Warriors 8: Xtreme Legends' encoding scheme.

   :param bytearray data: Data buffer to encode (modified in-place)
   :return: The data parameter
   :rtype: bytearray

.. py:function:: silent_hill3(enc_mode, data)

   Encrypts or decrypts data using Silent Hill 3's encryption algorithm.

   :param int enc_mode: Operation mode (``ENCRYPT`` or ``DECRYPT``)
   :param bytearray data: Data buffer to process (modified in-place)
   :return: The enc_mode parameter
   :rtype: int

.. py:function:: nfs_undercover(enc_mode, data)

   Encrypts or decrypts data using Need for Speed: Undercover's encryption algorithm.

   :param int enc_mode: Operation mode (``ENCRYPT`` or ``DECRYPT``)
   :param bytearray data: Data buffer to process (modified in-place)
   :return: The enc_mode parameter
   :rtype: int

.. py:function:: final_fantasy13(enc_mode, data, key, version)

   Encrypts or decrypts data using Final Fantasy XIII's encryption algorithm.

   :param int enc_mode: Operation mode (``ENCRYPT`` or ``DECRYPT``)
   :param bytearray data: Data buffer to process (modified in-place)
   :param bytearray key: Encryption key
   :param int version: Algorithm version parameter
   :return: The enc_mode parameter
   :rtype: int

.. py:function:: borderlands3(enc_mode, data, type)

   Encrypts or decrypts data using Borderlands 3's encryption algorithm.

   :param int enc_mode: Operation mode (``ENCRYPT`` or ``DECRYPT``)
   :param bytearray data: Data buffer to process (modified in-place)
   :param int type: Encryption type parameter
   :return: The enc_mode parameter
   :rtype: int

.. py:function:: mgs_pw(enc_mode, data)

   Encrypts or decrypts data using Metal Gear Solid: Peace Walker's encryption algorithm.

   :param int enc_mode: Operation mode (``ENCRYPT`` or ``DECRYPT``)
   :param bytearray data: Data buffer to process (modified in-place)
   :return: The enc_mode parameter
   :rtype: int

.. py:function:: mgs_base64(enc_mode, data)

   Encodes or decodes data using Metal Gear Solid's custom Base64 algorithm.

   :param int enc_mode: Operation mode (``ENCRYPT`` for encode, ``DECRYPT`` for decode)
   :param bytearray data: Data buffer to process (modified in-place)
   :return: The enc_mode parameter
   :rtype: int

.. py:function:: mgs(enc_mode, data, key)

   Encrypts or decrypts data using a generic Metal Gear Solid encryption algorithm.

   :param int enc_mode: Operation mode (``ENCRYPT`` or ``DECRYPT``)
   :param bytearray data: Data buffer to process (modified in-place)
   :param bytearray key: Encryption key
   :return: The data parameter
   :rtype: bytearray

.. py:function:: mgs5_tpp(data, key)

   Encodes data using Metal Gear Solid V: The Phantom Pain's encoding scheme.

   :param bytearray data: Data buffer to encode (modified in-place)
   :param int key: Encoding key parameter
   :return: The data parameter
   :rtype: bytearray

.. py:function:: monster_hunter(enc_mode, data, game_version)

   Encrypts or decrypts data using Monster Hunter's encryption algorithm.

   :param int enc_mode: Operation mode (``ENCRYPT`` or ``DECRYPT``)
   :param bytearray data: Data buffer to process (modified in-place)
   :param int game_version: Game version (2 or 3)
   :return: The enc_mode parameter
   :rtype: int
   :raises ValueError: If game_version is not 2 or 3

.. py:function:: rgg_studio(data, key)

   Applies XOR encryption to data using RGG Studio's algorithm.

   :param bytearray data: Data buffer to process (modified in-place)
   :param bytearray key: XOR key
   :return: The data parameter
   :rtype: bytearray

Generic Cryptography Functions
------------------------------

These functions implement standard cryptographic algorithms.

.. py:function:: aes_ecb(enc_mode, data, key)

   Encrypts or decrypts data using AES in ECB mode.

   :param int enc_mode: Operation mode (``ENCRYPT`` or ``DECRYPT``)
   :param bytearray data: Data buffer to process (modified in-place)
   :param bytearray key: Encryption key (16, 24, or 32 bytes for AES-128, AES-192, or AES-256)
   :return: The data parameter
   :rtype: bytearray

.. py:function:: aes_cbc(enc_mode, data, key, iv)

   Encrypts or decrypts data using AES in CBC mode.

   :param int enc_mode: Operation mode (``ENCRYPT`` or ``DECRYPT``)
   :param bytearray data: Data buffer to process (modified in-place)
   :param bytearray key: Encryption key (16, 24, or 32 bytes for AES-128, AES-192, or AES-256)
   :param bytearray iv: Initialization vector (16 bytes)
   :return: The enc_mode parameter
   :rtype: int

.. py:function:: aes_ctr(data, key, iv)

   Encrypts or decrypts data using AES in CTR mode.

   :param bytearray data: Data buffer to process (modified in-place)
   :param bytearray key: Encryption key (16, 24, or 32 bytes for AES-128, AES-192, or AES-256)
   :param bytearray iv: Counter/nonce (16 bytes)
   :return: The data parameter
   :rtype: bytearray

.. py:function:: des_ecb(enc_mode, data, key)

   Encrypts or decrypts data using DES in ECB mode.

   :param int enc_mode: Operation mode (``ENCRYPT`` or ``DECRYPT``)
   :param bytearray data: Data buffer to process (modified in-place)
   :param bytearray key: Encryption key (8 bytes)
   :return: The data parameter
   :rtype: bytearray

.. py:function:: des3_cbc(enc_mode, data, key, iv)

   Encrypts or decrypts data using Triple DES in CBC mode.

   :param int enc_mode: Operation mode (``ENCRYPT`` or ``DECRYPT``)
   :param bytearray data: Data buffer to process (modified in-place)
   :param bytearray key: Encryption key (24 bytes for 3-key DES)
   :param bytearray iv: Initialization vector (8 bytes)
   :return: The enc_mode parameter
   :rtype: int

.. py:function:: blowfish_ecb(enc_mode, data, key)

   Encrypts or decrypts data using Blowfish in ECB mode.

   :param int enc_mode: Operation mode (``ENCRYPT`` or ``DECRYPT``)
   :param bytearray data: Data buffer to process (modified in-place)
   :param bytearray key: Encryption key (1 to 56 bytes)
   :return: The data parameter
   :rtype: bytearray

.. py:function:: blowfish_cbc(enc_mode, data, key, iv)

   Encrypts or decrypts data using Blowfish in CBC mode.

   :param int enc_mode: Operation mode (``ENCRYPT`` or ``DECRYPT``)
   :param bytearray data: Data buffer to process (modified in-place)
   :param bytearray key: Encryption key (1 to 56 bytes)
   :param bytearray iv: Initialization vector (8 bytes)
   :return: The enc_mode parameter
   :rtype: int

.. py:function:: camellia_ecb(enc_mode, data, key)

   Encrypts or decrypts data using Camellia in ECB mode.

   :param int enc_mode: Operation mode (``ENCRYPT`` or ``DECRYPT``)
   :param bytearray data: Data buffer to process (modified in-place)
   :param bytearray key: Encryption key (16, 24, or 32 bytes)
   :return: The data parameter
   :rtype: bytearray

Usage Notes
-----------

- All functions operate **in-place** on the provided data buffer
- Input data should be provided as ``bytearray`` or ``memoryview`` objects
- For functions with ``enc_mode`` parameter, use the module constants ``ENCRYPT`` (1) or ``DECRYPT`` (0)
- Buffer lengths must be appropriate for each algorithm (e.g., multiples of block size for block ciphers)
- Keys and IVs must be the correct size for each algorithm

Example
-------

.. code-block:: python

   import ucrypto
   from ucrypto import ENCRYPT, DECRYPT
   
   # Example using AES-ECB
   data = bytearray(b"Hello World!1234")  # Must be 16-byte aligned for AES
   key = bytearray(16)  # 16-byte key for AES-128
   
   # Encrypt
   ucrypto.aes_ecb(ENCRYPT, data, key)
   
   # Decrypt
   ucrypto.aes_ecb(DECRYPT, data, key)
   
   # Example using game-specific encryption
   save_data = bytearray(...)  # Game save data
   ucrypto.mgs_pw(DECRYPT, save_data)