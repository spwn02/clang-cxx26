// RUN: clang-format -style=LLVM %s | FileCheck %s

int f(bool b,int n){
int y=do [k=1]->int{do_return k+1;};
int z=do{int t=do{do_return 5;};do_return t*2;}+1;
if(b)do{--n;}while(n>0);
return y+z;
}

// CHECK: int y = do [k = 1] -> int { do_return k + 1; };
// CHECK: int z = do {
// CHECK:   int t = do { do_return 5; };
// CHECK:   do_return t * 2;
// CHECK: } + 1;
// CHECK: if (b)
// CHECK-NEXT: do {
// CHECK-NEXT: --n;
// CHECK-NEXT: } while (n > 0);
