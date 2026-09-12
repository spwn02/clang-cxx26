// RUN: rm -rf %t
// RUN: split-file %s %t
// RUN: cd %t
//
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=enforce %t/M.cppm -emit-module-interface -o %t/M.pcm
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=enforce -fprebuilt-module-path=%t %t/Use.cpp -emit-llvm -o %t/Use.ll

// A DeclRefExpr referring to a contract-clause parameter (e.g. `count` in
// `pre(count > 0)`) sets DeclRefExpr::isInContractContext(), a bit this fork
// added to the same packed-bitfield sequence AST(De)Serialization already
// uses for DeclRefExpr (see ASTWriterStmt.cpp's VisitDeclRefExpr and
// ASTWriterDecl.cpp's EXPR_DECL_REF abbreviation, which reserves only a
// fixed 4-bit field for flags that predate this addition). Without excluding
// isInContractContext() (and the sibling isConstified() bit) from the fast
// abbreviated DeclRefExpr encoding, precompiling any module whose exported
// declarations contain a contract clause referencing a parameter crashes
// with "High bits set!" while writing the .pcm -- a plain single-TU compile
// never hits this, since it never serializes an AST to a bitstream.

//--- M.cppm
export module M;

export template <class T>
struct S {
  T value;

  constexpr auto get(int count) pre(count > 0) {
    return value;
  }
};

//--- Use.cpp
import M;

int main() {
  S<int> s{42};
  return s.get(1);
}
