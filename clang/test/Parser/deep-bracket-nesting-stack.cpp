// RUN: ulimit -s 8192
// RUN: %clang_cc1 -std=c++20 -fsyntax-only -fbracket-depth 2048 -Wno-stack-exhausted %s
// RUN: %clang_cc1 -std=c++20 -fsyntax-only -fbracket-depth 2048 -Wno-stack-exhausted -DCALLS %s

// Nesting up to -fbracket-depth must not overflow the stack of the parser (it recursed
// ParseParenExpression -> ParseAssignmentExpression -> ParseCastExpression without a stack check;
// about 2,000 nested brackets crashed on an 8 MB stack).

#define P1(x) (x)
#define P10(x) P1(P1(P1(P1(P1(P1(P1(P1(P1(P1(x))))))))))
#define P100(x) P10(P10(P10(P10(P10(P10(P10(P10(P10(P10(x))))))))))
#define P1000(x) P100(P100(P100(P100(P100(P100(P100(P100(P100(P100(x))))))))))
#define C1(x) f(x)
#define C10(x) C1(C1(C1(C1(C1(C1(C1(C1(C1(C1(x))))))))))
#define C100(x) C10(C10(C10(C10(C10(C10(C10(C10(C10(C10(x))))))))))
int f(int x);
#ifdef CALLS
int calls() { return C100(C100(C100(C100(C100(C100(C100(C100(C100(C100(C100(C100(C100(C100(C100(C100(C100(C100(C100(1))))))))))))))))))); } // 1900 nested calls
#else
int parens() { return P1000(P1000(P10(P10(1)))); } // 2040 nested parentheses
#endif
