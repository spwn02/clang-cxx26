// The filename of '__has_include', '__has_embed' (and '#include') may be a macro
// that looks for a filename itself: the lexer was asked twice at once for a
// filename and asserted ("reentered LexIncludeFilename").

// RUN: rm -rf %t && split-file %s %t
// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %t/nested.cpp
// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %t/eof.cpp

//--- nested.cpp
#if __has_include(__has_include(<x>)) // expected-error {{expected "FILENAME" or <FILENAME>}} expected-error {{expected end of line in preprocessor expression}}
#endif
#if __has_embed(__has_include(<x>)) // expected-error {{expected "FILENAME" or <FILENAME>}} expected-error {{expected end of line in preprocessor expression}}
#endif
#if __has_include(__has_embed("x")) // expected-error {{expected "FILENAME" or <FILENAME>}} expected-error {{expected end of line in preprocessor expression}}
#endif
//--- eof.cpp
// expected-error@+4 {{missing '(' after '__has_include'}}
// expected-error@+3 {{expected "FILENAME" or <FILENAME>}}
// expected-error@+2 {{expected value in expression}}
// expected-error@+1 {{unterminated conditional directive}}
#if __has_embed(__has_include
