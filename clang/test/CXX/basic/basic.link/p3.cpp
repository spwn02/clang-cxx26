// RUN: rm -rf %t
// RUN: split-file %s %t

// RUN: %clang_cc1 -std=c++20 -verify %t/M.cpp
// RUN: %clang_cc1 -std=c++20 -verify %t/ImportError1.cpp
// RUN: %clang_cc1 -std=c++20 -verify %t/ModuleInNamespace.cpp
// RUN: %clang_cc1 -std=c++20 -verify %t/ImportError2.cpp
// RUN: %clang_cc1 -std=c++20 -verify %t/ImportInNamespace.cpp
// RUN: %clang_cc1 -std=c++20 -verify %t/ImportSameLine.cpp

//--- M.cpp
module;

struct import { struct inner {}; };
struct module { struct inner {}; };
constexpr int n = 123;

export module m; // #1
module y = {}; // expected-error {{multiple module declarations}} expected-error 2{{}}
// expected-note@#1 {{previous module declaration}}

::import x = {};
::module y = {};

import::inner xi = {};
module::inner yi = {};

namespace N {
  // module a; at the start of a line is a module directive: see ModuleInNamespace.cpp
  // import b; is an import directive (it starts its line, [cpp.pre]): see ImportInNamespace.cpp
}

extern "C++" module cxxm;
extern "C++" import cxxi;

template<typename T> module module_var_template;

// This is a variable named 'import' that shadows the type 'import' above.
struct X {} import;

//--- ImportError1.cpp
module;

struct import { struct inner {}; };
struct module { struct inner {}; };

constexpr int n = 123;

export module m; // #1

import x = {}; // expected-error {{expected ';' after module name}}
               // expected-error@-1 {{module 'x' not found}}

//--- ImportError2.cpp
// expected-no-diagnostics
module;

struct module { struct inner {}; };

constexpr int n = 123;

export module m; // #1

struct X;
template<int> struct import;
template<> struct import<n> {
  static X y;
};

// The 'import' is not at the start of a logical source line, so (CWG3177) this is
// not a pp-import and no header-name is formed.
struct X {} import<n>::y;


//--- ImportInNamespace.cpp
// An import that starts its logical line introduces an import directive wherever it is, not only at the top level.
struct import { struct inner {}; };
namespace N {
import b; // expected-error {{module 'b' not found}}
}

//--- ImportSameLine.cpp
// expected-no-diagnostics
// 'import' is not the first token of its logical line: not a directive, so an ordinary identifier.
struct import {};
int a; import x;

//--- ModuleInNamespace.cpp
// A module declaration that starts its logical line is a directive wherever it is (and ill-formed here).
export module m;
namespace N {
module a; // expected-error {{module declaration can only appear at the top level}}
}
