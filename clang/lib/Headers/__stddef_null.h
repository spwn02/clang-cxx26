/*===---- __stddef_null.h - Definition of NULL -----------------------------===
 *
 * Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
 * See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *===-----------------------------------------------------------------------===
 */

#if !defined(NULL) || !__building_module(_Builtin_stddef)

/* linux/stddef.h will define NULL to 0. glibc (and other) headers then define
 * __need_NULL and rely on stddef.h to redefine NULL to the correct value again.
 * Modules don't support redefining macros like that, but support that pattern
 * in the non-modules case.
 */
#undef NULL

#ifdef __cplusplus
/* [support.types.nullptr]: NULL is an implementation-defined null pointer
 * constant that is a literal (LWG4182); __null is a keyword, not a literal.
 * The literal has the width of a pointer, like __null, so that it can still be
 * passed through a variable argument list as a sentinel. */
#if defined(__MINGW32__) || defined(_MSC_VER)
#define NULL 0
#elif __SIZEOF_POINTER__ == __SIZEOF_LONG__
#define NULL 0L
#elif __SIZEOF_POINTER__ == __SIZEOF_LONG_LONG__
#define NULL 0LL
#else
#define NULL 0
#endif
#else
#define NULL ((void*)0)
#endif

#endif
