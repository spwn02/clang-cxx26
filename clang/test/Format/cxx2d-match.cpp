// RUN: clang-format -style=LLVM %s | FileCheck %s

int f(P p,int x){
if(case[0,auto b]=p)return b;
return match(x){
case 0=>10;
case 1||2=>20;
case auto y if(y>5)=>y;
case{auto&&alt}=>1;
case _=>-1;
};
}
bool g(int x){return match(x,case 1||2);}

// CHECK: if (case [0, auto b] = p)
// CHECK: return match (x) {
// CHECK:   case 0 => 10;
// CHECK:   case 1 || 2 => 20;
// CHECK:   case auto y if (y > 5) => y;
// CHECK:   case {auto &&alt} => 1;
// CHECK:   case _ => -1;
// CHECK: };
// CHECK: bool g(int x) { return match (x, case 1 || 2); }
