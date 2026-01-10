:mod:`umsgpack` -- MessagePack encoding and decoding
====================================================

.. module:: umsgpack
   :synopsis: MessagePack encoding and decoding

:Version: 2.8.0
:Author: vsergeev / Ivan (Vanya) A. Sergeev
:Source: https://github.com/vsergeev/u-msgpack-python
:License: MIT

Overview
--------

u-msgpack-python is a lightweight MessagePack serializer and deserializer
module that is:

* Compatible with both Python 2 and 3
* Compatible with CPython and PyPy implementations
* Fully compliant with the latest MessagePack specification
* Supports binary, UTF-8 string, and application extension types

The module provides a simple API for serializing Python objects to MessagePack
format and deserializing MessagePack data back to Python objects.

Quick Start
-----------

.. code-block:: python

    import umsgpack
    
    # Serialize a Python object to bytes
    packed = umsgpack.packb({"compact": True, "schema": 0})
    
    # Deserialize MessagePack bytes back to Python object
    unpacked = umsgpack.unpackb(packed)
    
    # Work with files
    with open('data.msgpack', 'wb') as f:
        umsgpack.pack({"hello": "world"}, f)
    
    with open('data.msgpack', 'rb') as f:
        data = umsgpack.unpack(f)

Installation
------------

The module is a single Python file. Simply download ``umsgpack.py`` and
import it in your project.

API Reference
-------------

Main Functions
~~~~~~~~~~~~~~

.. function:: pack(obj, fp, **options)

    Serialize a Python object into MessagePack format and write to a file-like object.
    
    :param obj: Python object to serialize
    :param fp: File-like object with a ``.write()`` method
    :param \*\*options: Additional packing options (see below)
    :raises UnsupportedTypeException: Object type not supported for packing

.. function:: packb(obj, **options)

    Serialize a Python object into MessagePack bytes.
    
    :param obj: Python object to serialize
    :param \*\*options: Additional packing options (see below)
    :returns: Serialized MessagePack bytes
    :raises UnsupportedTypeException: Object type not supported for packing

.. function:: unpack(fp, **options)

    Deserialize MessagePack data from a file-like object into a Python object.
    
    :param fp: File-like object with a ``.read()`` method
    :param \*\*options: Additional unpacking options (see below)
    :returns: Deserialized Python object
    :raises UnpackException: Various unpacking errors (see Exceptions section)

.. function:: unpackb(s, **options)

    Deserialize MessagePack bytes into a Python object.
    
    :param s: Serialized MessagePack bytes (``bytes`` or ``bytearray``)
    :param \*\*options: Additional unpacking options (see below)
    :returns: Deserialized Python object
    :raises UnpackException: Various unpacking errors (see Exceptions section)

Aliases
~~~~~~~

For convenience, the module provides aliases that match the ``pickle`` module API:

* ``dump`` = ``pack``
* ``dumps`` = ``packb``
* ``load`` = ``unpack``
* ``loads`` = ``unpackb``

Configuration Variables
-----------------------

.. data:: compatibility

    Compatibility mode boolean.
    
    When enabled, u-msgpack-python will serialize both unicode strings and bytes
    into the old "raw" MessagePack type, and deserialize the "raw" MessagePack
    type into bytes. This provides backwards compatibility with the old
    MessagePack specification.
    
    Default: ``False``
    
    .. code-block:: python
    
        umsgpack.compatibility = True
        packed = umsgpack.packb(["some string", b"some bytes"])
        # Both strings and bytes are packed as raw type
        unpacked = umsgpack.unpackb(packed)
        # Both are returned as bytes

.. data:: version

    Module version tuple (2, 8, 0)

.. data:: __version__

    Module version string "2.8.0"

Extension Types
---------------

Extension Types System
~~~~~~~~~~~~~~~~~~~~~~

u-msgpack-python supports MessagePack's extension types through two mechanisms:

1. Manual handling using the ``Ext`` class
2. Automatic registration using the ``ext_serializable`` decorator

.. class:: Ext(type, data)

    Container for MessagePack extension type objects.
    
    :param type: Application-defined type integer (-128 to 127)
    :param data: Application-defined data byte array
    :raises TypeError: Type is not integer or data has wrong type
    :raises ValueError: Type is out of range
    
    .. method:: __init__(type, data)
    
        Construct a new Ext object.
    
    .. method:: __eq__(other)
    
        Compare Ext objects for equality.
    
    .. method:: __str__()
    
        String representation of the Ext object.
    
    .. method:: __hash__()
    
        Provide a hash of the Ext object.

.. function:: ext_serializable(ext_type)

    Decorator to register a class for automatic packing and unpacking.
    
    :param ext_type: Application-defined Ext type code (-128 to 127)
    :returns: Decorator function
    :raises TypeError: Ext type is not integer
    :raises ValueError: Ext type out of range or already registered
    
    The decorated class must implement:
    
    * A ``packb()`` method that returns serialized bytes
    * An ``unpackb()`` class/static method that accepts bytes and returns an instance
    
    .. code-block:: python
    
        @umsgpack.ext_serializable(5)
        class MyClass:
            def __init__(self, value):
                self.value = value
            
            def packb(self):
                return str(self.value).encode('utf-8')
            
            @staticmethod
            def unpackb(data):
                return MyClass(int(data.decode('utf-8')))
        
        # Now MyClass instances are automatically serialized/deserialized
        obj = MyClass(42)
        packed = umsgpack.packb(obj)
        unpacked = umsgpack.unpackb(packed)

Packing Options
---------------

The following keyword arguments can be passed to packing functions:

.. describe:: ext_handlers

    Dictionary mapping custom types to callables that pack instances into ``Ext`` objects.
    
    .. code-block:: python
    
        def pack_myobj(obj):
            return umsgpack.Ext(10, obj.serialize())
        
        umsgpack.packb(myobj, ext_handlers={MyClass: pack_myobj})

.. describe:: force_float_precision

    Force specific float precision:
    
    * ``"single"``: IEEE-754 single-precision (32-bit) floats
    * ``"double"``: IEEE-754 double-precision (64-bit) floats
    
    Default: Auto-detected based on system

Unpacking Options
-----------------

The following keyword arguments can be passed to unpacking functions:

.. describe:: ext_handlers

    Dictionary mapping integer Ext types to callables that unpack ``Ext`` instances.
    
    .. code-block:: python
    
        def unpack_myext(ext):
            return MyClass.deserialize(ext.data)
        
        umsgpack.unpackb(data, ext_handlers={10: unpack_myext})

.. describe:: use_ordered_dict

    Unpack maps into ``collections.OrderedDict`` instead of regular ``dict``.
    
    Default: ``False``

.. describe:: use_tuple

    Unpack arrays into tuples instead of lists.
    
    Default: ``False``

.. describe:: allow_invalid_utf8

    Unpack invalid UTF-8 strings into ``InvalidString`` instances (subclass of ``bytes``)
    instead of raising ``InvalidStringException``.
    
    Default: ``False``

Supported Data Types
--------------------

The following Python types are supported for serialization:

Basic Types
~~~~~~~~~~~

* ``None`` (serialized as nil)
* ``bool``
* ``int`` (including Python 2 ``long``)
* ``float``
* ``str`` (UTF-8 strings)
* ``bytes`` (binary data)

Collections
~~~~~~~~~~~

* ``list``
* ``tuple`` (serialized as array)
* ``dict``

Special Types
~~~~~~~~~~~~~

* ``datetime.datetime`` (serialized as timestamp extension)
* ``umsgpack.Ext`` (extension types)

Exceptions
----------

All exceptions inherit from either ``PackException`` or ``UnpackException``.

Packing Exceptions
~~~~~~~~~~~~~~~~~~

.. exception:: PackException

    Base class for packing exceptions.

.. exception:: UnsupportedTypeException(PackException)

    Object type not supported for packing.

Unpacking Exceptions
~~~~~~~~~~~~~~~~~~~~

.. exception:: UnpackException

    Base class for unpacking exceptions.

.. exception:: InsufficientDataException(UnpackException)

    Insufficient data to unpack the serialized object.

.. exception:: InvalidStringException(UnpackException)

    Invalid UTF-8 string encountered during unpacking.

.. exception:: UnsupportedTimestampException(UnpackException)

    Unsupported timestamp format encountered during unpacking.

.. exception:: ReservedCodeException(UnpackException)

    Reserved code encountered during unpacking.

.. exception:: UnhashableKeyException(UnpackException)

    Unhashable key encountered during map unpacking.

.. exception:: DuplicateKeyException(UnpackException)

    Duplicate key encountered during map unpacking.

Backwards Compatibility
~~~~~~~~~~~~~~~~~~~~~~~

For compatibility with older code:

* ``KeyNotPrimitiveException`` = ``UnhashableKeyException``
* ``KeyDuplicateException`` = ``DuplicateKeyException``

Examples
--------

Basic Usage
~~~~~~~~~~~

.. code-block:: python

    import umsgpack
    
    # Simple serialization
    data = {
        "name": "John Doe",
        "age": 30,
        "hobbies": ["reading", "hiking", "coding"],
        "metadata": {
            "created": datetime.datetime.now(),
            "version": 1.0
        }
    }
    
    # Serialize to bytes
    packed = umsgpack.packb(data)
    
    # Deserialize back
    unpacked = umsgpack.unpackb(packed)

Working with Files
~~~~~~~~~~~~~~~~~~

.. code-block:: python

    import umsgpack
    
    # Write to file
    with open('data.msgpack', 'wb') as f:
        umsgpack.pack({"test": [1, 2, 3]}, f)
    
    # Read from file
    with open('data.msgpack', 'rb') as f:
        data = umsgpack.unpack(f)

Custom Types with Extension System
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: python

    import umsgpack
    import datetime
    
    # Manual extension handling
    class Point:
        def __init__(self, x, y):
            self.x = x
            self.y = y
        
        def pack(self):
            return struct.pack('>ii', self.x, self.y)
        
        @classmethod
        def unpack(cls, data):
            x, y = struct.unpack('>ii', data)
            return cls(x, y)
    
    # Pack with manual handler
    def pack_point(obj):
        return umsgpack.Ext(1, obj.pack())
    
    point = Point(10, 20)
    packed = umsgpack.packb(point, ext_handlers={Point: pack_point})
    
    # Unpack with manual handler
    def unpack_point(ext):
        return Point.unpack(ext.data)
    
    unpacked = umsgpack.unpackb(packed, ext_handlers={1: unpack_point})

Notes
-----

* The module automatically handles timezone-aware and naive datetime objects
* When using compatibility mode, all strings are returned as bytes
* For maximum performance with large data, use the file-based API (``pack``/``unpack``)
* Extension type codes -128 to 127 are available for application use
* Type code -1 is reserved for timestamp extension

See Also
--------

* `MessagePack Specification <https://github.com/msgpack/msgpack/blob/master/spec.md>`_
* `Official MessagePack Python Implementation <https://github.com/msgpack/msgpack-python>`_
