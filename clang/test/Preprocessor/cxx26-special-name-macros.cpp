// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify=pedantic -pedantic %s
// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify=none %s
// none-no-diagnostics

// [cpp.replace.general]: a translation unit shall not #define or #undef a name
// identical to an identifier with special meaning ([lex.name.special]) or to an
// attribute-token ([dcl.attr]); 'likely' and 'unlikely' may be defined as
// function-like macros and may be undefined.
#define pre 1      // pedantic-warning {{defining the macro 'pre', an identifier with special meaning or an attribute name, is ill-formed}}
#define post 2     // pedantic-warning {{defining the macro 'post'}}
#define module 3   // pedantic-warning {{defining the macro 'module'}}
#define import 4   // pedantic-warning {{defining the macro 'import'}}
#define nodiscard  // pedantic-warning {{defining the macro 'nodiscard'}}
#define noreturn   // pedantic-warning {{defining the macro 'noreturn'}}
#define indeterminate // pedantic-warning {{defining the macro 'indeterminate'}}
#define no_unique_address // pedantic-warning {{defining the macro 'no_unique_address'}}
#undef pre         // pedantic-warning {{undefining the macro 'pre'}}

#define likely(x) (x)   // allowed: function-like
#undef likely           // allowed
#define unlikely(x) (x)
#undef unlikely
#define likely 1        // pedantic-warning {{defining the macro 'likely'}}
#undef likely

#define NOT_SPECIAL 1

// [lex.pptoken]: a character outside the basic character set that forms a
// preprocessing token of the last category makes the program ill-formed, even
// in a macro that is never expanded.
#define EURO € // pedantic-warning {{character not in the basic character set forms a preprocessing token that is not a valid token}}
