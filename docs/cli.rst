===================================
Apollo Command-line Tools Reference
===================================

CLI Tools
---------

`Apollo command-line
tools <https://github.com/bucanero/apollo-lib/releases/latest>`__ are
useful for code creators and developers, to test SW codes and BSD
scripts locally on a computer.

-  `dumper <#dumper>`__
-  `patcher <#patcher>`__

patcher
~~~~~~~

The ``patcher`` command-line tool reads a ``.savepatch`` file and a
comma-separated list of patches, and apply the selected cheat codes to
the target file. It defaults to little-endian data mode and also
supports ``-b/--big-endian`` and ``-l/--little-endian`` to select the
mode at run-time.

::

   Apollo Cheat Patcher v2.1.0 - (c) 2022-2026 by Bucanero

   Patching:
    USAGE: ./patcher [-b|--big-endian] [-l|--little-endian] file.savepatch 1,2,7-10,18 [data-file.bin]

     -b,--big-endian:    Use big-endian data mode
     -l,--little-endian: Use little-endian data mode
     file.savepatch: The cheat patch file to apply
     1,2,7-10,18:    The list of codes to apply
     data-file.bin:  The target file to patch

   Listing:
    USAGE: ./patcher file.savepatch [-c 1,2,7-10,18]

     file.savepatch: The cheat patch file to list
     -c:             Display code details (Optional)
     1,2,7-10,18:    The list of codes to display (Optional)

dumper
~~~~~~

The ``dumper`` command-line tool reads a binary file and generates a
``.savepatch`` file with a SW code that writes the raw content of the
binary data.

::

   Apollo binary file SW dumper v1.4.0 - (c) 2023-2025 by Bucanero

   USAGE: ./dumper filename.ext
