// FILE_DEPENDENCIES: annotation-module-serialization.cppm
//
// RUN: %{cxx} %{compile_flags} -std=c++26 -freflection-latest \
// RUN:     --precompile annotation-module-serialization.cppm -o %t.pcm
// RUN: %{cxx} %{compile_flags} -std=c++26 -freflection-latest \
// RUN:     -fmodule-file=AnnotationSerialization=%t.pcm -fsyntax-only %s

// expected-no-diagnostics
#include <meta>

import AnnotationSerialization;

constexpr Rename extracted_rename = extracted();
static_assert(extracted_rename.value[0] == 'n');
static_assert(extracted_rename.value[1] == 'e');
static_assert(extracted_rename.value[2] == 'w');
static_assert(extracted_rename.value[3] == '\0');
