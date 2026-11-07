// The driver tells the frontend that the C++ runtime has __dynamic_cast_first
// (used by pattern matching to try several type patterns in one call) for
// libc++ on ELF targets only.

// RUN: %clang -### -c --target=x86_64-unknown-linux-gnu -stdlib=libc++ %s 2>&1 | FileCheck %s --check-prefix=ON
// RUN: %clang -### -c --target=aarch64-unknown-freebsd -stdlib=libc++ %s 2>&1 | FileCheck %s --check-prefix=ON
// RUN: %clang -### -c --target=x86_64-unknown-linux-gnu -stdlib=libstdc++ %s 2>&1 | FileCheck %s --check-prefix=OFF
// RUN: %clang -### -c --target=x86_64-apple-darwin -stdlib=libc++ %s 2>&1 | FileCheck %s --check-prefix=OFF
// RUN: %clang -### -c --target=x86_64-pc-windows-msvc -stdlib=libc++ %s 2>&1 | FileCheck %s --check-prefix=OFF
// RUN: %clang -### -c --target=x86_64-unknown-linux-gnu -stdlib=libc++ -x c %s 2>&1 | FileCheck %s --check-prefix=OFF

// ON: "-fdynamic-cast-first"
// OFF-NOT: "-fdynamic-cast-first"
