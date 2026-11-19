// A module file whose dependency was rebuilt differently (C++20 named-module
// imports record no signature of what they import) names declarations that
// no longer exist. The compiler reports that the module file is out of date
// and stops; it must not assert or crash (#486).
//
// RUN: rm -rf %t && mkdir -p %t && split-file %s %t
// RUN: %clang_cc1 -std=c++20 -emit-reduced-module-interface %t/a1.cppm -o %t/a.pcm
// RUN: %clang_cc1 -std=c++20 -fmodule-file=A=%t/a.pcm -emit-reduced-module-interface %t/b.cppm -o %t/b.pcm
// RUN: %clang_cc1 -std=c++20 -fmodule-file=A=%t/a.pcm -fmodule-file=B=%t/b.pcm -fsyntax-only %t/u.cpp
// RUN: %clang_cc1 -std=c++20 -emit-reduced-module-interface %t/a2.cppm -o %t/a.pcm
// RUN: not %clang_cc1 -std=c++20 -fmodule-file=A=%t/a.pcm -fmodule-file=B=%t/b.pcm -fsyntax-only %t/u.cpp 2>&1 | FileCheck %s
//
// CHECK: fatal error: malformed or corrupted precompiled file: 'declaration of unexpected kind '{{.*}}' read from a module file; the module file is out of date with the module files it imports'

//--- a1.cppm
export module A;
export namespace ns { namespace inner { template <class T> inline constexpr bool v = true; } }
export template <class T> concept C = ns::inner::v<T>;

//--- a2.cppm
export module A;
typedef int t1; typedef int t2; typedef int t3; typedef int t4; typedef int t5; using t6 = int;
export namespace ns { namespace inner { template <class T> inline constexpr bool v = true; } }
export template <class T> concept C = ns::inner::v<T>;
export typedef int tt;

//--- b.cppm
export module B;
import A;
export template <class T> struct S { };
template <class T> requires (!C<T>) struct S<T> { static constexpr int v = 1; };
template <class T> requires C<T> struct S<T> { static constexpr int v = 2; };

//--- u.cpp
import B;
static_assert(S<int>::v == 2);
