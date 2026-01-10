:mod:`utime` -- time related functions
======================================

.. module:: utime
   :synopsis: time related functions

The ``utime`` module provides functions for getting the current time and date,
measuring time intervals, and for delays.

**Time Epoch**: Apollo uses standard for POSIX systems epoch of
1970-01-01 00:00:00 UTC.

Functions
---------

.. function:: gmtime([secs])
.. function:: localtime([secs])

   Convert the time *secs* expressed in seconds since the Epoch (see above) into an
   9-tuple which contains: ``(year, month, mday, hour, minute, second, weekday, yearday, dst)``
   If *secs* is not provided or None, then the current time from the RTC is used.

   The `gmtime()` function returns a date-time tuple in UTC, and `localtime()` returns a
   date-time tuple in local time.

   * year includes the century (for example 2014).
   * month   is 1-12
   * mday    is 1-31
   * hour    is 0-23
   * minute  is 0-59
   * second  is 0-59
   * weekday is 0-6 for Mon-Sun
   * yearday is 1-366

.. function:: mktime(time_tuple)

   This is inverse function of localtime. It's argument is a full 8-tuple
   which expresses a time as per localtime. It returns an integer which is
   the number of seconds since Jan 1, 1970.

.. function:: time()

   Returns the number of seconds, as an integer, since the Epoch, assuming that underlying
   RTC is set and maintained as described above. If an RTC is not set, this function returns
   number of seconds since a port-specific reference point in time.
   If you need calendar time, ``localtime()`` without an argument is a better choice.

   .. admonition:: Difference to CPython
      :class: attention

      In CPython, this function returns number of
      seconds since Unix epoch, 1970-01-01 00:00 UTC, as a floating-point,
      usually having microsecond precision.

.. function:: strftime(format[, time_tuple])

   This method converts a given time tuple (time_tuple) into a formatted string based on the
   specified format. If ``time_tuple`` is not provided, the function uses the current local time.
   The format must be a string.
