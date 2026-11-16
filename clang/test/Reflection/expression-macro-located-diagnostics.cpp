// RUN: %clang -std=c++29 -nostdinc++ -isystem %S/../../../libcxx/include -idirafter %S/../../../build-libcxx/include/c++/v1 -fsyntax-only -fcxx-exceptions -Xclang -verify -Wmacro-located-diagnostics %s
// RUN: not %clang -std=c++29 -nostdinc++ -isystem %S/../../../libcxx/include -idirafter %S/../../../build-libcxx/include/c++/v1 -fsyntax-only -fcxx-exceptions -Wmacro-located-diagnostics -fdiagnostics-parseable-fixits %s 2>&1 | FileCheck %s --check-prefix=FIXIT
// RUN: not %clang -std=c++29 -nostdinc++ -isystem %S/../../../libcxx/include -idirafter %S/../../../build-libcxx/include/c++/v1 -fsyntax-only -fcxx-exceptions -Wmacro-located-diagnostics %s 2>&1 | FileCheck %s --check-prefix=DIAG

#include <__meta/token_locations.h>
#include <debugging>

using std::meta::token_sequence;
namespace mx = std::meta::__x;

// A located error fails the expansion at its token, without a call-site
// macro-evaluation diagnostic.
__macro one_error(token_sequence ts) {
  (void)mx::__error(ts[0], "bad token");
  return ^^{ 1 };
}
int one = one_error!(bad_token); // expected-error@17 {{bad token}}

// Located errors record and continue; both are emitted and the malformed
// token sequence returned below must never reach parsing.
__macro two_errors(token_sequence ts) {
  (void)mx::__error(ts[0], ts[1], "first token error");
  (void)mx::__error(ts[1], "second token error");
  return ^^{ 1 + };
}
int two = two_errors!(first_token second_token); // expected-error@26 {{first token error}} expected-error@26 {{second token error}}

// Warnings and notes do not fail the expansion. The warning can be disabled.
__macro warning_and_note(token_sequence ts) {
  (void)mx::__warning(ts[0], "located warning");
  (void)mx::__note(ts[1], ts[1], "located note");
  return ^^{ 42 };
}
static_assert(warning_and_note!(warn_token note_token) == 42); // expected-warning@34 {{located warning}} expected-note@34 {{located note}}

// Invalid token locations fall back to the opening parenthesis of the call.
__macro invalid_location(token_sequence) {
  (void)mx::__error(std::meta::token{}, "invalid location fallback");
  return ^^{ 1 };
}
int fallback = invalid_location!(anything); // expected-error@41 {{invalid location fallback}}

// The fix-it endpoints are the first and last tokens of an inclusive range;
// insertion goes after the token, and removal covers its inclusive range.
__macro fixit_replace(token_sequence ts) {
  (void)mx::__error(ts[0], "replace fixit",
                    mx::__replace(ts[0], ts[1], "replacement"));
  return ^^{ 1 };
}
int replace = fixit_replace!(replace_first replace_last); // expected-error@50 {{replace fixit}}

__macro fixit_insert(token_sequence ts) {
  (void)mx::__warning(ts[0], "insert fixit",
                      mx::__insert_after(ts[0], " inserted"));
  return ^^{ 1 };
}
int insert = fixit_insert!(insert_here); // expected-warning@57 {{insert fixit}}

__macro fixit_remove(token_sequence ts) {
  (void)mx::__warning(ts[0], "remove fixit",
                      mx::__remove(ts[0], ts[1]));
  return ^^{ 1 };
}
int removed = fixit_remove!(remove_first remove_last); // expected-warning@64 {{remove fixit}}

// Macro-produced token storage is not a valid edit target. Keep the report,
// but discard its fix-it.
#define FROM_MACRO macro_storage
__macro macro_storage_fixit(token_sequence ts) {
  (void)mx::__warning(ts[0], "macro storage fixit",
                      mx::__replace(ts[0], ts[0], "ignored"));
  return ^^{ 1 };
}
int macro_storage = macro_storage_fixit!(FROM_MACRO); // expected-warning@74 {{macro storage fixit}}

__macro macro_storage_insert(token_sequence ts) {
  (void)mx::__warning(ts[0], "macro insertion storage",
                      mx::__insert_after(ts[0], " ignored"));
  return ^^{ 1 };
}
int macro_insert = macro_storage_insert!(FROM_MACRO); // expected-warning@81 {{macro insertion storage}}

// This explicit expand-at-instantiation case is deferred by existing Sema.
// Its preliminary evaluation is discarded, then instantiation reports once;
// this test makes no broader replay claim.
__macro deferred(token_sequence ts) {
  (void)mx::__error(ts[0], "deferred error");
  std::meta::expand_at_instantiation();
  return ^^{ 1 };
}
template <class T> int templated(T value) {
  return deferred!(template_token);
}
int instantiate = templated(0); // expected-error@92 {{deferred error}}

// The legacy API remains immediate and call-site-pinned.
__macro legacy_error() {
  std::constexpr_error_str("legacy", "legacy error");
  return ^^{ 1 };
}
int legacy = legacy_error!(); // expected-error@101 {{expression macro 'legacy_error' reported an error}} expected-note@debugging:105 {{constexpr message with tag 'legacy': legacy error}} expected-note@98 {{in call to}} expected-note@97 {{in call to 'legacy_error()'}} expected-note@94 {{in instantiation of function template specialization 'templated<int>' requested here}}

// FIXIT: fix-it:"{{.*}}":{50:30-50:56}:"replacement"
// FIXIT: fix-it:"{{.*}}":{57:39-57:39}:" inserted"
// FIXIT: fix-it:"{{.*}}":{64:29-64:53}:""
// FIXIT-NOT: ignored
// DIAG-DAG: {{.*}}expression-macro-located-diagnostics.cpp:17:22: error: bad token
// DIAG-DAG: {{.*}}expression-macro-located-diagnostics.cpp:26:23: error: first token error
// DIAG-DAG: {{.*}}expression-macro-located-diagnostics.cpp:26:35: error: second token error
// DIAG-DAG: {{.*}}expression-macro-located-diagnostics.cpp:34:33: warning: located warning
// DIAG-DAG: {{.*}}expression-macro-located-diagnostics.cpp:34:44: note: located note
// DIAG-DAG: {{.*}}expression-macro-located-diagnostics.cpp:41:33: error: invalid location fallback
// DIAG-DAG: {{.*}}expression-macro-located-diagnostics.cpp:50:30: error: replace fixit
// DIAG-DAG: {{.*}}expression-macro-located-diagnostics.cpp:57:28: warning: insert fixit
// DIAG-DAG: {{.*}}expression-macro-located-diagnostics.cpp:64:29: warning: remove fixit
// DIAG-DAG: {{.*}}expression-macro-located-diagnostics.cpp:74:42: warning: macro storage fixit
// DIAG-DAG: {{.*}}expression-macro-located-diagnostics.cpp:81:42: warning: macro insertion storage
// DIAG-DAG: {{.*}}expression-macro-located-diagnostics.cpp:92:20: error: deferred error
// DIAG-DAG: {{.*}}expression-macro-located-diagnostics.cpp:101:27: error: expression macro 'legacy_error' reported an error
