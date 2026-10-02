// RUN: %clang_cc1 -std=c++26 -freflection -fexpansion-statements -fsyntax-only -verify=err %s
// RUN: %clang_cc1 -std=c++26 -freflection -fexpansion-statements -fsyntax-only -Wno-error=expansion-stmt-non-compound-body -verify=warn %s
// RUN: %clang_cc1 -std=c++26 -freflection -fexpansion-statements -fsyntax-only -Wno-expansion-stmt-non-compound-body -verify=none %s
// none-no-diagnostics

// P1306R5 [stmt.expand]: the body is a compound-statement. Other statements are
// accepted as an extension that is an error by default.
void f() {
  template for (auto x : {1, 2}) ; // err-error {{body of an expansion statement should be a compound-statement}} warn-warning {{body of an expansion statement should be a compound-statement}}
  template for (auto x : {1, 2}) (void)x; // err-error {{body of an expansion statement should be a compound-statement}} warn-warning {{body of an expansion statement should be a compound-statement}}
  template for (auto x : {1, 2}) { (void)x; }
}
