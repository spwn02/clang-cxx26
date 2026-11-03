.. _implementation-defined-behavior:

===============================
Implementation-defined behavior
===============================

This document contains the implementation details of the implementation-defined behavior in libc++.
The C++ standard mandates that implementation-defined behavior is documented.

.. note:
   This page is far from complete.


Implementation-defined behavior
===============================

Updating the Time Zone Database
-------------------------------

The C++ standard allows implementations to automatically update the
*remote time zone database*. Libc++ opts not to do that. Instead calling

 - ``std::chrono::remote_version()`` will update the version information of the
   *remote time zone database*,
 - ``std::chrono::reload_tzdb()``, if needed, will update the entire
   *remote time zone database*.

This offers a way for users to update the *remote time zone database* and
give them full control over the process.


`[ostream.formatted.print]/3 <http://eel.is/c++draft/ostream.formatted.print#3>`_ A terminal capable of displaying Unicode
--------------------------------------------------------------------------------------------------------------------------

The C++ standard specifies that the manner in which a stream is determined to refer
to a terminal capable of displaying Unicode is implementation-defined. This is
used for ``std::print`` and similar functions taking an ``ostream&`` argument.

Libc++ determines that a stream is Unicode-capable terminal by:

* First it determines whether the stream's ``rdbuf()`` has an underlying
  ``FILE*``. This is ``true`` in the following cases:

  * The stream is ``std::cout``, ``std::cerr``, or ``std::clog``.

  * A ``std::basic_filebuf<CharT, Traits>`` derived from ``std::filebuf``.

* The way to determine whether this ``FILE*`` refers to a terminal capable of
  displaying Unicode is the same as specified for `void vprint_unicode(FILE*
  stream, string_view fmt, format_args args);
  <http://eel.is/c++draft/print.fun#7>`_. This function is used for other
  ``std::print`` overloads that don't take an ``ostream&`` argument.

`[sf.cmath] <https://wg21.link/sf.cmath>`_ Mathematical Special Functions: Large indices
----------------------------------------------------------------------------------------

Most functions within the Mathematical Special Functions section contain integral indices.
The C++ standard specifies the result for larger indices as implementation-defined.
Libc++ pursuits reasonable results by choosing the same formulas as for indices below that threshold.
E.g.,

- ``std::hermite(unsigned n, T x)`` for ``n >= 128``
- ``std::laguerre(unsigned n, T x)`` and ``std::assoc_laguerre(unsigned n, unsigned m, T x)`` for ``n >= 128``
- ``std::legendre(unsigned l, T x)`` and ``std::assoc_legendre(unsigned l, unsigned m, T x)`` for ``l >= 128``

Additionally, ``std::comp_ellint_1(T k)``, ``std::comp_ellint_2(T k)``, ``std::comp_ellint_3(T k, T nu)``,
and their incomplete counterparts return a quiet NaN, rather than an infinity, when ``k`` sits exactly
on the domain boundary (``k == +-1``): the Carlson symmetric-form evaluation used internally has no
well-defined limit there.


`[filebuf.virtuals] <https://eel.is/c++draft/filebuf.virtual>`_ Effect of calling ``basic_filebuf::setbuf`` with nonzero arguments
----------------------------------------------------------------------------------------------------------------------------------

Libc++ uses the provided buffer as the underlying buffer for input and output, and
does not discard that buffer even when the stream is closed.


`[stringbuf.cons] <http://eel.is/c++draft/stringbuf.cons>`_ Whether sequence pointers are initialized to null pointers
----------------------------------------------------------------------------------------------------------------------

Libc++ does not initialize the pointers to null pointers. It resizes the buffer
to its capacity and uses that size. This means the SSO buffer of
``std::string`` is used as initial output buffer.


`[syserr.fmt] <https://eel.is/c++draft/syserr.fmt>`_ Encoding of an error message during formatting (C++29)
------------------------------------------------------------------------------------------------------------

``std::formatter<std::error_code, charT>`` with the ``s`` option formats ``ec.message()``, which is in the
execution character set of the current C locale. For ``char`` when the ordinary literal encoding is UTF-8 it is
transcoded to UTF-8, as the Standard requires; that is the only case the Standard specifies. In every other case
libc++ chooses the following encoding:

 - ``char`` when the ordinary literal encoding is not UTF-8 (``_LIBCPP_HAS_UNICODE`` is off): the message unchanged.
 - ``wchar_t``: each multibyte character of the current C locale converted with ``mbrtowc``; a byte that is not a
   valid character is replaced by U+FFFD.

A program that never calls ``setlocale`` runs in the "C" locale. On platforms where that locale only accepts ASCII
(glibc), every byte of a message that is not ASCII is replaced by U+FFFD; a message in UTF-8 is passed through
unchanged only after the program selects a UTF-8 locale.


`[ptrtag.bits] <https://eel.is/c++draft/ptrtag.bits>`_ Pointer tagging limits (C++29)
-------------------------------------------------------------------------------------

``std::max_pointer_bits_available`` is 12 on every target: a tag never reaches into the page number of a 4 KiB page.
``std::pointer_bits_available(alignment)`` returns ``min(countr_zero(alignment), 12)``. Because the value is not zero,
libc++ guarantees what the remark of ``max_pointer_bits_available`` asks for: a tagged pointer is the untagged pointer
plus the tag as a byte offset (``getelementptr``), so every operation that keeps an invalid pointer value as it is
(copying, storing, passing, comparing) keeps the tag, and ``from_tagged`` of the result gives back the same
``pointer_tag_pair``. In constant expressions a use needs an object aligned to at least ``2^bits`` bytes, and a tagged
pointer that is not turned back with ``pointer()`` is not diagnosed when it is dereferenced.


Listed in the index of implementation-defined behavior
======================================================

The order of the entries matches the entries in the
`draft of the Standard <http://eel.is/c++draft/impldefindex>`_.
