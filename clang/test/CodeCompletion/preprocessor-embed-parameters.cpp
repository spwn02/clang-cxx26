// The parameters of #embed and of __has_embed ([cpp.embed.param]) are offered
// after the resource, with the argument as a placeholder; one that is given
// already is not offered again.
#ifdef CASE_ALL
unsigned char a[] = {
#embed "preprocessor-embed-parameters.cpp" 
};
#endif
#ifdef CASE_GIVEN
unsigned char a[] = {
#embed "preprocessor-embed-parameters.cpp" limit(1) 
};
#endif
#ifdef CASE_CLANG
unsigned char a[] = {
#embed "preprocessor-embed-parameters.cpp" clang::
};
#endif
#ifdef CASE_PRE
unsigned char a[] = {
#embed "preprocessor-embed-parameters.cpp" pre
};
#endif
#if defined(CASE_HAS) && __has_embed("preprocessor-embed-parameters.cpp" suffix(0) )
#endif

// RUN: %clang_cc1 -std=c++26 -fsyntax-only -DCASE_ALL -code-completion-at=%s:6:44 %s | FileCheck -check-prefix=CHECK-ALL %s
// CHECK-ALL: COMPLETION: Pattern : clang::offset(<#count#>)
// CHECK-ALL: COMPLETION: Pattern : if_empty(<#tokens#>)
// CHECK-ALL: COMPLETION: Pattern : limit(<#count#>)
// CHECK-ALL: COMPLETION: Pattern : prefix(<#tokens#>)
// CHECK-ALL: COMPLETION: Pattern : suffix(<#tokens#>)
// C++26 has no underscored spellings, they are C23's.
// CHECK-ALL-NOT: __limit__
// RUN: %clang_cc1 -std=c++26 -fsyntax-only -DCASE_GIVEN -code-completion-at=%s:11:53 %s | FileCheck -check-prefix=CHECK-GIVEN %s
// CHECK-GIVEN-NOT: limit
// CHECK-GIVEN: COMPLETION: Pattern : prefix(<#tokens#>)
// RUN: %clang_cc1 -std=c++26 -fsyntax-only -DCASE_CLANG -code-completion-at=%s:16:51 %s | FileCheck -check-prefix=CHECK-CLANG %s
// CHECK-CLANG: COMPLETION: Pattern : offset(<#count#>)
// CHECK-CLANG-NOT: limit
// RUN: %clang_cc1 -std=c++26 -fsyntax-only -DCASE_PRE -code-completion-at=%s:21:47 %s | FileCheck -check-prefix=CHECK-PRE %s
// CHECK-PRE: COMPLETION: Pattern : prefix(<#tokens#>)
// RUN: %clang_cc1 -std=c++26 -fsyntax-only -DCASE_HAS -code-completion-at=%s:24:84 %s | FileCheck -check-prefix=CHECK-HAS %s
// CHECK-HAS-NOT: suffix
// CHECK-HAS: COMPLETION: Pattern : limit(<#count#>)
// The underscored spellings of the standard parameters are C23's ([cpp.embed]
// has none; C23 6.10p5).
// RUN: %clang_cc1 -x c -std=c23 -fsyntax-only -DCASE_ALL -code-completion-at=%s:6:44 %s | FileCheck -check-prefix=CHECK-C23 %s
// CHECK-C23: COMPLETION: Pattern : __limit__(<#count#>)
// CHECK-C23: COMPLETION: Pattern : limit(<#count#>)
