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
-  `parser <#parser>`__
-  `patcher <#patcher>`__
-  `patcher-bigendian <#patcher-bigendian>`__

parser
~~~~~~

The ``parser`` command-line tool reads a ``.savepatch`` file, and
provides a numbered list of detected cheat patches.

::

   Apollo .savepatch parser v1.4.0 - (c) 2021-2025 by Bucanero

   USAGE: ./parser file.savepatch [code #]

     file.savepatch: The cheat patch file to parse
     code #:         The code patch to display (Optional)

patcher
~~~~~~~

The ``patcher`` command-line tool reads a ``.savepatch`` file and a
comma-separated list of patches, and apply the selected cheat codes to
the target file.

::

   Apollo cheat patcher v1.4.0 - (c) 2022-2025 by Bucanero

   USAGE: ./patcher file.savepatch 1,2,7-10,18 [data-file.bin]

     file.savepatch: The cheat patch file to apply
     1,2,7-10,18:    The list of codes to apply
     data-file.bin:  The target file to patch

patcher-bigendian
~~~~~~~~~~~~~~~~~

``patcher-bigendian`` is provided to apply patches on PS3 (big-endian)
save-game data files.

::

   Apollo cheat patcher v1.4.0 PS3/big-endian - (c) 2022 by Bucanero

   USAGE: ./patcher-bigendian file.savepatch 1,2,7-10,18 [data-file.bin]

     file.savepatch: The cheat patch file to apply
     1,2,7-10,18:    The list of codes to apply
     data-file.bin:  The target file to patch

dumper
~~~~~~

The ``dumper`` command-line tool reads a binary file and generates a
``.savepatch`` file with a SW code that writes the raw content of the
binary data.

::

   Apollo binary file SW dumper v1.4.0 - (c) 2023-2025 by Bucanero

   USAGE: ./dumper filename.ext
