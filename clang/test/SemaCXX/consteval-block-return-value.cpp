// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++29 -fsyntax-only -verify %s

// A consteval block is a void lambda-like scope: returning a value is an error.
// The diagnostic takes a select argument (block or lambda); a call site that
// forgot it made clang assert while formatting the message.
consteval { return 1; } // expected-error {{void lambda should not return a value}}

consteval { return; }
