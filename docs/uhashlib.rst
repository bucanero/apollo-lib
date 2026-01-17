:mod:`uhashlib` -- hashing algorithm
====================================

.. module:: uhashlib
   :synopsis: hashing algorithm

The ``uhashlib`` module provides hashing and checksum functions, including both standard cryptographic hashes and game-specific checksum algorithms. All functions return byte arrays containing the hash/checksum result.

Constants
---------

.. py:data:: CRC_16_BITS
   :value: 16

   Constant representing 16-bit CRC width for custom CRC calculations.

.. py:data:: CRC_32_BITS
   :value: 32

   Constant representing 32-bit CRC width for custom CRC calculations.

.. py:data:: CRC_64_BITS
   :value: 64

   Constant representing 64-bit CRC width for custom CRC calculations.

Standard Cryptographic Hash Functions
-------------------------------------

These functions implement standard cryptographic hash algorithms.

.. py:function:: md5(data)

   Computes the MD5 hash (128-bit) of the input data.

   :param bytes data: Input data to hash
   :return: 16-byte MD5 hash
   :rtype: bytes

.. py:function:: sha1(data)

   Computes the SHA-1 hash (160-bit) of the input data.

   :param bytes data: Input data to hash
   :return: 20-byte SHA-1 hash
   :rtype: bytes

.. py:function:: sha224(data)

   Computes the SHA-224 hash (224-bit, truncated SHA-256) of the input data.

   :param bytes data: Input data to hash
   :return: 28-byte SHA-224 hash
   :rtype: bytes

.. py:function:: sha256(data)

   Computes the SHA-256 hash (256-bit) of the input data.

   :param bytes data: Input data to hash
   :return: 32-byte SHA-256 hash
   :rtype: bytes

.. py:function:: sha384(data)

   Computes the SHA-384 hash (384-bit, truncated SHA-512) of the input data.

   :param bytes data: Input data to hash
   :return: 48-byte SHA-384 hash
   :rtype: bytes

.. py:function:: sha512(data)

   Computes the SHA-512 hash (512-bit) of the input data.

   :param bytes data: Input data to hash
   :return: 64-byte SHA-512 hash
   :rtype: bytes

.. py:function:: hmac_sha1(key, data)

   Computes HMAC-SHA1 (Hash-based Message Authentication Code using SHA-1).

   :param bytes key: HMAC key
   :param bytes data: Input data to authenticate
   :return: 20-byte HMAC-SHA1 result
   :rtype: bytes

Password-Based Key Derivation Functions (PBKDF2)
------------------------------------------------

The PBKDF2 (Password-Based Key Derivation Function 2) functions implement key derivation from passwords using HMAC with SHA-1 or SHA-256. These functions are commonly used for password hashing and key derivation in cryptographic applications.

.. py:function:: pbkdf2_sha1(password, salt, iterations, dklen)

   Derives a cryptographic key from a password using PBKDF2 with HMAC-SHA1.

   PBKDF2 applies a pseudorandom function (HMAC-SHA1) to the input password along with a salt value and repeats the process many times to produce a derived key. This "key stretching" technique makes brute-force attacks more difficult.

   :param bytes password: Password or passphrase from which to derive the key
   :param bytes salt: Cryptographic salt (should be random and unique for each password)
   :param int iterations: Number of iterations (higher values increase security but slow down computation)
   :param int dklen: Desired length of the derived key in bytes
   :return: Derived key as bytes
   :rtype: bytes

   **Security Considerations**:
   
   - Use a cryptographically random salt of at least 16 bytes
   - Use a high iteration count (typically 100,000 to 1,000,000)
   - SHA-1 is considered cryptographically weak; prefer ``pbkdf2_sha256`` when possible
   - The derived key length should match the requirements of the cryptographic algorithm using it

   **Example**::

      import uhashlib
      import os
      
      password = b"mySecretPassword123"
      salt = os.urandom(16)  # Generate random salt
      derived_key = uhashlib.pbkdf2_sha1(password, salt, 100000, 32)
      # Returns 32-byte derived key

.. py:function:: pbkdf2_sha256(password, salt, iterations, dklen)

   Derives a cryptographic key from a password using PBKDF2 with HMAC-SHA256.

   This is a more secure variant using SHA-256 as the underlying hash function. It provides better cryptographic security than SHA-1 and is recommended for new applications.

   :param bytes password: Password or passphrase from which to derive the key
   :param bytes salt: Cryptographic salt (should be random and unique for each password)
   :param int iterations: Number of iterations (higher values increase security but slow down computation)
   :param int dklen: Desired length of the derived key in bytes
   :return: Derived key as bytes
   :rtype: bytes

   **Security Considerations**:
   
   - Use a cryptographically random salt of at least 16 bytes
   - Use a high iteration count (typically 100,000 to 1,000,000)
   - SHA-256 is currently considered cryptographically secure
   - Consider using even higher iteration counts for very sensitive applications

   **Example**::

      import uhashlib
      import os
      
      password = b"mySecretPassword123"
      salt = os.urandom(16)  # Generate random salt
      derived_key = uhashlib.pbkdf2_sha256(password, salt, 100000, 32)
      # Returns 32-byte derived key

Standard Checksum Functions
---------------------------

These functions implement common checksum and non-cryptographic hash algorithms.

.. py:function:: adler16(data)

   Computes Adler-16 checksum (16-bit) of the input data.

   :param bytes data: Input data to checksum
   :return: 2-byte Adler-16 checksum (big-endian)
   :rtype: bytes

.. py:function:: adler32(data, init=0)

   Computes Adler-32 checksum (32-bit) of the input data.

   :param bytes data: Input data to checksum
   :param int init: Optional initial value (default: 0)
   :return: 4-byte Adler-32 checksum (big-endian)
   :rtype: bytes

.. py:function:: crc16(data)

   Computes standard CRC-16 (CRC-16-IBM) checksum.

   :param bytes data: Input data to checksum
   :return: 2-byte CRC-16 checksum (big-endian)
   :rtype: bytes

.. py:function:: crc32(data)

   Computes standard CRC-32 (CRC-32/ISO-HDLC) checksum with reflected input/output.

   :param bytes data: Input data to checksum
   :return: 4-byte CRC-32 checksum (big-endian)
   :rtype: bytes

.. py:function:: crc32big(data)

   Computes CRC-32/BZIP checksum without reflection (big-endian).

   :param bytes data: Input data to checksum
   :return: 4-byte CRC-32 checksum (big-endian)
   :rtype: bytes

.. py:function:: crc64_ecma(data)

   Computes CRC-64-ECMA checksum.

   :param bytes data: Input data to checksum
   :return: 8-byte CRC-64-ECMA checksum (big-endian)
   :rtype: bytes

.. py:function:: crc64_iso(data)

   Computes CRC-64-ISO checksum.

   :param bytes data: Input data to checksum
   :return: 8-byte CRC-64-ISO checksum (big-endian)
   :rtype: bytes

.. py:function:: crc(width, data, poly, init, xor, refIn, refOut)

   Computes custom CRC with user-defined parameters.

   :param int width: CRC width in bits (16, 32, or 64)
   :param bytes data: Input data to checksum
   :param int poly: Polynomial value
   :param int init: Initial value
   :param int xor: Final XOR value
   :param int refIn: Input reflection (0=no, 1=yes)
   :param int refOut: Output reflection (0=no, 1=yes)
   :return: CRC result (2, 4, or 8 bytes depending on width)
   :rtype: bytes
   :raises ValueError: If width is not 16, 32, or 64

.. py:function:: force_crc32(data, offset, newcrc)

   Forces a specific CRC-32 value at a given offset in the data.

   :param bytes data: Input data (modified to have target CRC)
   :param int offset: Byte offset where CRC should be placed
   :param int newcrc: Desired CRC-32 value
   :return: 4-byte modified CRC value
   :rtype: bytes

Game-Specific Checksum Functions
--------------------------------

These functions implement checksum algorithms used by specific video games.

.. py:function:: ea_checksum(data)

   Computes checksum used by Electronic Arts games (MC02 algorithm).

   :param bytes data: Input data to checksum
   :return: 4-byte EA checksum (big-endian)
   :rtype: bytes

.. py:function:: ffx_checksum(data)

   Computes checksum used by Final Fantasy X.

   :param bytes data: Input data to checksum
   :return: 2-byte FFX checksum (little-endian)
   :rtype: bytes

.. py:function:: ff13_checksum(data)

   Computes checksum used by Final Fantasy XIII.

   :param bytes data: Input data to checksum
   :return: 4-byte FFXIII checksum (little-endian)
   :rtype: bytes

.. py:function:: kh25_checksum(data)

   Computes checksum used by Kingdom Hearts 2.5.

   :param bytes data: Input data to checksum
   :return: 4-byte KH2.5 checksum (little-endian)
   :rtype: bytes

.. py:function:: khcom_checksum(data)

   Computes checksum used by Kingdom Hearts: Chain of Memories.

   :param bytes data: Input data to checksum
   :return: 4-byte KH:CoM checksum (big-endian)
   :rtype: bytes

.. py:function:: mgs2_checksum(data)

   Computes checksum used by Metal Gear Solid 2.

   :param bytes data: Input data to checksum
   :return: 4-byte MGS2 checksum (big-endian)
   :rtype: bytes

.. py:function:: mgspw_checksum(data)

   Computes checksum used by Metal Gear Solid: Peace Walker.

   :param bytes data: Input data to checksum
   :return: 4-byte MGSPW checksum (big-endian)
   :rtype: bytes

.. py:function:: sw4_checksum(data)

   Computes 4-part checksum used by Samurai Warriors 4 (Sengoku Musou 4).

   :param bytes data: Input data to checksum
   :return: Tuple of 4 4-byte checksums (big-endian)
   :rtype: tuple

.. py:function:: toz_checksum(data)

   Computes 20-byte checksum used by Tales of Zestiria.

   :param bytes data: Input data to checksum
   :return: 20-byte ToZ checksum
   :rtype: bytes

.. py:function:: tiara2_checksum(data)

   Computes checksum used by Tears to Tiara 2 games.

   :param bytes data: Input data to checksum
   :return: 4-byte Tiara 2 checksum (big-endian)
   :rtype: bytes

.. py:function:: castlevania_checksum(data)

   Computes checksum used by Castlevania: Lords of Shadow 1 & 2.

   :param bytes data: Input data to checksum
   :return: 4-byte Castlevania checksum (little-endian)
   :rtype: bytes

.. py:function:: rockstar_checksum(data)

   Computes and updates Rockstar Games CHKS checksums in data.

   :param bytes data: Input data containing CHKS blocks
   :return: 4-byte final CHKS value (big-endian)
   :rtype: bytes

.. py:function:: dbzxv2_checksum(data)

   Computes checksum used by Dragon Ball Z: Xenoverse 2.

   :param bytes data: Input data to checksum
   :return: 8-byte DBZXV2 checksum (big-endian)
   :rtype: bytes

.. py:function:: deadrising_checksum(data)

   Computes checksum used by Dead Rising games.

   :param bytes data: Input data to checksum
   :return: 4-byte Dead Rising checksum (big-endian)
   :rtype: bytes

Custom Hash Functions
---------------------

These functions implement various custom or modified hash algorithms.

.. py:function:: md5_xor(data)

   Computes MD5 hash and XORs the four 32-bit words together.

   :param bytes data: Input data to hash
   :return: 4-byte XOR result of MD5 words (big-endian)
   :rtype: bytes

.. py:function:: sha1_xor64(data)

   Computes SHA-1 hash and XORs to produce a 64-bit value.

   :param bytes data: Input data to hash
   :return: 8-byte XOR result of SHA-1 (big-endian)
   :rtype: bytes

.. py:function:: checksum32(data)

   Computes a simple 32-bit checksum.

   :param bytes data: Input data to checksum
   :return: 4-byte checksum (big-endian)
   :rtype: bytes

.. py:function:: add(data, carry=0)

   Computes additive checksum (sum of bytes).

   :param bytes data: Input data to checksum
   :param int carry: Carry value (must be 2 if provided)
   :return: 2 or 4-byte additive checksum (big-endian)
   :rtype: bytes
   :raises ValueError: If carry is specified and not 2

.. py:function:: wadd(data, carry=0)

   Computes word-additive checksum (sum of 16-bit words, big-endian).

   :param bytes data: Input data to checksum
   :param int carry: Carry value (must be 2 if provided)
   :return: 2 or 4-byte word-additive checksum (big-endian)
   :rtype: bytes
   :raises ValueError: If carry is specified and not 2

.. py:function:: wadd_le(data)

   Computes word-additive checksum (sum of 16-bit words, little-endian).

   :param bytes data: Input data to checksum
   :return: 4-byte word-additive checksum (big-endian)
   :rtype: bytes

.. py:function:: dwadd(data)

   Computes double-word additive checksum (sum of 32-bit words, big-endian).

   :param bytes data: Input data to checksum
   :return: 4-byte double-word additive checksum (big-endian)
   :rtype: bytes

.. py:function:: dwadd_le(data)

   Computes double-word additive checksum (sum of 32-bit words, little-endian).

   :param bytes data: Input data to checksum
   :return: 4-byte double-word additive checksum (big-endian)
   :rtype: bytes

.. py:function:: qwadd(data)

   Computes quad-word additive checksum (sum of 64-bit words).

   :param bytes data: Input data to checksum
   :return: 4-byte quad-word additive checksum (big-endian)
   :rtype: bytes

.. py:function:: wsub(data)

   Computes word-subtractive checksum.

   :param bytes data: Input data to checksum
   :return: 4-byte word-subtractive checksum (big-endian)
   :rtype: bytes

General-Purpose Hash Functions
------------------------------

These functions implement various general-purpose non-cryptographic hash algorithms.

.. py:function:: sdbm(data, init=0)

   Computes SDBM hash.

   :param bytes data: Input data to hash
   :param int init: Optional initial value (default: 0)
   :return: 4-byte SDBM hash (big-endian)
   :rtype: bytes

.. py:function:: fnv1(data, init=FNV1_INIT_VALUE)

   Computes FNV-1 (Fowler-Noll-Vo) hash.

   :param bytes data: Input data to hash
   :param int init: Optional initial value (default: ``0x811c9dc5``)
   :return: 4-byte FNV-1 hash (big-endian)
   :rtype: bytes

.. py:function:: djb2(data)

   Computes DJB2 hash (Daniel J. Bernstein hash).

   :param bytes data: Input data to hash
   :return: 4-byte DJB2 hash (big-endian)
   :rtype: bytes

.. py:function:: murmur3_32(data, init=0)

   Computes MurmurHash3 32-bit hash.

   :param bytes data: Input data to hash
   :param int init: Optional initial value (default: 0)
   :return: 4-byte MurmurHash3 hash (big-endian)
   :rtype: bytes

.. py:function:: jhash(data, init=0)

   Computes jhash (Jenkins hash).

   :param bytes data: Input data to hash
   :param int init: Optional initial value (default: 0)
   :return: 4-byte jhash (big-endian)
   :rtype: bytes

.. py:function:: jenkins_oaat(data, init=0)

   Computes Jenkins one-at-a-time hash.

   :param bytes data: Input data to hash
   :param int init: Optional initial value (default: 0)
   :return: 4-byte Jenkins OAAT hash (big-endian)
   :rtype: bytes

.. py:function:: lookup3_little2(data, iv1, iv2)

   Computes lookup3 little-endian hash producing two 32-bit values.

   :param bytes data: Input data to hash
   :param int iv1: First initialization vector
   :param int iv2: Second initialization vector
   :return: Tuple of two 4-byte hash values (big-endian)
   :rtype: tuple

Usage Notes
-----------

- All functions return the hash/checksum as a ``bytes`` object
- Most checksums are returned in big-endian format unless otherwise specified
- For functions with optional ``init`` parameters, these provide initial hash values
- The ``rockstar_checksum`` function modifies CHKS blocks in-place within the input data
- Input data should be provided as ``bytes`` or ``bytearray`` objects

Example
-------

.. code-block:: python

   import uhashlib
   
   # Compute standard hashes
   data = b"Hello World"
   md5_hash = uhashlib.md5(data)
   sha256_hash = uhashlib.sha256(data)
   
   # Compute game-specific checksums
   save_data = b"..."
   ffx_checksum = uhashlib.ffx_checksum(save_data)
   mgs2_checksum = uhashlib.mgs2_checksum(save_data)
   
   # Compute custom CRC
   crc_result = uhashlib.crc(
       width=uhashlib.CRC_32_BITS,
       data=data,
       poly=0x04C11DB7,
       init=0xFFFFFFFF,
       xor=0xFFFFFFFF,
       refIn=1,
       refOut=1
   )
