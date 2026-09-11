// RUN: %clang_cc1 -std=c++26 %s -verify -fsyntax-only

// Regression test: a malformed top-level declaration that fails to parse as
// a function declarator (here, a bogus parameter type referring to a
// namespace) previously crashed inside Sema::checkExposure at end-of-TU.
// Sema::ActOnEndOfTranslationUnit's exposure-checking pass walks every
// declaration to implement [basic.link]p14, including ones left behind by
// error recovery -- ExposureChecker::isTULocal(QualType) unconditionally
// dereferenced its argument without checking for a null QualType, unlike
// its two sibling overloads. A genuinely malformed declaration must still
// produce ordinary diagnostics, never a crash.

export module Foo;

namespace ns {}

// expected-error@+2 {{consteval can only be used in function declarations}}
// expected-error@+1 {{unexpected namespace name 'ns': expected expression}}
consteval auto diagnosticIsMessage(ns) // expected-error {{expected ';' after top level declarator}}
