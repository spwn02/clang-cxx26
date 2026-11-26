// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: gcc
// XFAIL: has-no-cxx-module-support

// RUN: mkdir %t
// RUN: %{cxx} %{compile_flags} -std=c++26 \
// RUN:     -Wno-reserved-module-identifier -Wno-reserved-user-defined-literal \
// RUN:     --precompile -o %t/std.pcm -c %{module-dir}/std.cppm
// RUN: %{cxx} %{compile_flags} %{link_flags} -std=c++26 \
// RUN:     -fmodule-file=std=%t/std.pcm %t/std.pcm %s -o %t/test.exe
// RUN: %{exec} %t/test.exe

import std;

static_assert(sizeof(decltype(std::contracts::controls::enforced)) == 1);

int main(int, char**) { return 0; }
