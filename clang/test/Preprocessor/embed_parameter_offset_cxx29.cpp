// RUN: %clang_cc1 -std=c++2d %s --embed-dir=%S/Inputs -fsyntax-only -verify=cxx29
// RUN: %clang_cc1 -std=c++2c %s --embed-dir=%S/Inputs -fsyntax-only -verify=cxx26

// P3540R3: 'offset' is a standard #embed parameter in C++29. Before, only the
// clang::offset spelling exists.

constexpr char data[] = {
#embed <jk.txt>
};
constexpr char offset_data[] = {
#embed <jk.txt> offset(1)
// cxx26-error@-1 {{unknown embed preprocessor parameter 'offset'}}
};
constexpr char scoped_offset_data[] = {
#embed <jk.txt> clang::offset(1)
};
static_assert(sizeof(scoped_offset_data) == 1);
static_assert(scoped_offset_data[0] == data[1]);

#if __cplusplus > 202400L
static_assert(sizeof(offset_data) == 1);
static_assert(offset_data[0] == data[1]);

// The offset applies to the resource before limit.
const char limited[] = {
#embed <jk.txt> offset(1) limit(5)
};
static_assert(sizeof(limited) == 1);
constexpr char limited_zero[] = {
#embed <jk.txt> limit(1) offset(1) if_empty(7)
};
static_assert(sizeof(limited_zero) == 1);
static_assert(limited_zero[0] == 'k');

// An offset at or past the end makes the resource empty, so if_empty applies.
constexpr unsigned char empty[] = {
#embed <jk.txt> offset(2) if_empty(42)
};
static_assert(sizeof(empty) == 1 && empty[0] == 42);

#if __has_embed(<jk.txt> offset(2)) != __STDC_EMBED_EMPTY__
#error "an offset past the end is an empty resource"
#endif
#if __has_embed(<jk.txt> offset(1)) != __STDC_EMBED_FOUND__
#error "found with a smaller offset"
#endif

// The parameter may appear at most once, in either spelling.
const char dup[] = {
#embed <jk.txt> offset(1) offset(0)
// cxx29-error@-1 {{cannot specify parameter 'offset' twice in the same '#embed' directive}}
};
const char dup2[] = {
#embed <jk.txt> offset(1) clang::offset(0)
// cxx29-error@-1 {{cannot specify parameter 'clang::offset' twice in the same '#embed' directive}}
};

// It cannot be negative.
#embed <jk.txt> offset(-1)
// cxx29-error@-1 {{invalid value '-1'; must be positive}}
#endif
