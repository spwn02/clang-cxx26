// RUN: %clang_cc1 -std=c++26 -freflection -fsyntax-only -verify %s

using Function = int();
template<class T> [: ^^Function :] function requires true;
template<class T> Function alias_control requires true;

template<class T> [: ^^int :] object requires true; // expected-error {{trailing requires clause can only be used when declaring a function}}
