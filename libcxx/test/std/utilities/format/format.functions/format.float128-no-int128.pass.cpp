// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20
// ADDITIONAL_COMPILE_FLAGS: -U__SIZEOF_INT128__

// Exercise the same coverage as format.float128.pass.cpp without the
// compiler's __int128 feature macro.
#include "format.float128.pass.cpp"
