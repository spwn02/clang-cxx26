struct A { int x; int y; };
struct B : A { int z; };
B b{.x = 1, .z = 2};
