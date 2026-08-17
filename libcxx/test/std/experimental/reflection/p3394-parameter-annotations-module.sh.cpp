// FILE_DEPENDENCIES: p3394-parameter-annotations-module.cppm
//
// RUN: %{cxx} %{compile_flags} -std=c++26 -freflection-latest -fparameter-reflection \
// RUN:     --precompile p3394-parameter-annotations-module.cppm -o %t.pcm
// RUN: %{cxx} %{compile_flags} -std=c++26 -freflection-latest -fparameter-reflection \
// RUN:     -fmodule-file=ParameterAnnotations=%t.pcm -fsyntax-only %s

// expected-no-diagnostics
#include <meta>

import ParameterAnnotations;

constexpr auto parameter = std::meta::parameters_of(^^configure)[0];
static_assert(std::meta::extract<Inject>(std::meta::annotations_of(parameter)[0]).slot == 7);
