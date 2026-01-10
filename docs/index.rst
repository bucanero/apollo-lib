MicroPython libraries
=====================

This chapter describes modules (function and class libraries) which are built
into MicroPython. There are a few categories of modules:

* Modules which implement a subset of standard Python functionality and are not
  intended to be extended by the user.
* Modules which implement a subset of Python functionality, with a provision
  for extension by the user (via Python code).
* Modules which implement MicroPython extensions to the Python standard libraries.
* Modules specific to a particular port and thus not portable.

Note about the availability of modules and their contents: This documentation
in general aspires to describe all modules and functions/classes which are
implemented in MicroPython. However, MicroPython is highly configurable, and
each port to a particular board/embedded system makes available only a subset
of MicroPython libraries. For officially supported ports, there is an effort
to either filter out non-applicable items, or mark individual descriptions
with "Availability:" clauses describing which ports provide a given feature.
With that in mind, please still be warned that some functions/classes
in a module (or even the entire module) described in this documentation may be
unavailable in a particular build of MicroPython on a particular board. The
best place to find general information of the availability/non-availability
of a particular feature is the "General Information" section which contains
information pertaining to a specific port.

Beyond the built-in libraries described in this documentation, many more
modules from the Python standard library, as well as further MicroPython
extensions to it, can be found in the `micropython-lib repository
<https://github.com/micropython/micropython-lib>`_.

.. toctree::
   :maxdepth: 2

   bsd.rst
   savewizard.rst

Python standard libraries and micro-libraries
---------------------------------------------

The following standard Python libraries have been "micro-ified" to fit in with
the philosophy of MicroPython.  They provide the core functionality of that
module and are intended to be a drop-in replacement for the standard Python
library.

.. only:: not port_unix

    The modules are available by their u-name, and also by their non-u-name.  The
    non-u-name can be overridden by a file of that name in your package path.
    For example, ``import json`` will first search for a file ``json.py`` or
    directory ``json`` and load that package if it is found.  If nothing is found,
    it will fallback to loading the built-in ``ujson`` module.

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
