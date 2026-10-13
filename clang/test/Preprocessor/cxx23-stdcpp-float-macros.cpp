// [cpp.predefined]: __STDCPP_FLOAT16_T__ ... __STDCPP_BFLOAT16_T__ are predefined
// (to 1) iff the implementation supports the extended floating-point type. They are
// predefined when the driver selects libc++; with libstdc++ they would be read as
// "GCC's _Float<N> types exist", so only the private spellings are defined there.

// RUN: %clang -target x86_64-unknown-linux-gnu -stdlib=libc++ -std=c++23 -x c++ -E -dM %s | FileCheck --check-prefix=LIBCXX %s
// RUN: %clang -target x86_64-unknown-linux-gnu -stdlib=libc++ -std=c++26 -x c++ -E -dM %s | FileCheck --check-prefix=LIBCXX %s
// RUN: %clang -target x86_64-unknown-linux-gnu -stdlib=libstdc++ -std=c++23 -x c++ -E -dM %s | FileCheck --check-prefix=LIBSTDCXX %s
// RUN: %clang -target x86_64-unknown-linux-gnu -stdlib=libc++ -std=c++20 -x c++ -E -dM %s | FileCheck --check-prefix=OLDER %s
// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++23 -fstdcpp-float-macros -E -dM %s | FileCheck --check-prefix=LIBCXX %s
// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++23 -E -dM %s | FileCheck --check-prefix=LIBSTDCXX %s

// LIBCXX-DAG: #define __STDCPP_BFLOAT16_T__ 1
// LIBCXX-DAG: #define __STDCPP_FLOAT128_T__ 1
// LIBCXX-DAG: #define __STDCPP_FLOAT16_T__ 1
// LIBCXX-DAG: #define __STDCPP_FLOAT32_T__ 1
// LIBCXX-DAG: #define __STDCPP_FLOAT64_T__ 1
// LIBCXX-DAG: #define __CLANG_STDCPP_FLOAT32_T__ 1

// LIBSTDCXX-NOT: #define __STDCPP_
// LIBSTDCXX: #define __CLANG_STDCPP_FLOAT32_T__ 1

// OLDER-NOT: STDCPP_{{B?}}FLOAT
