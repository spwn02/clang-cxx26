#ifndef MSVC_LIBC_STDLIB_H
#define MSVC_LIBC_STDLIB_H

// The part of <stdlib.h> that <__cstdlib/callbacks.h> needs, declared as the C library of Windows does.
typedef unsigned long long size_t;
extern "C" {
int atexit(void (*)());
void qsort(void*, size_t, size_t, int (*)(const void*, const void*));
void* bsearch(const void*, const void*, size_t, size_t, int (*)(const void*, const void*));
}

#endif
