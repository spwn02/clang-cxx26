// RUN: %clang_cc1 -std=c++26  %s -fcontract-evaluation-semantic=observe


int foo(int x) pre(x != 0) {
    return x;
}

template <class T>
void foo2(T v) {
    contract_assert [[clang::contract_semantic("ignore")]] (v + 1 != 0) ;
}

int main() {

    foo2(0);
}


