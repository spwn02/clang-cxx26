//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

#include <cstdio>
#include <print>
#include <sstream>
#include <string_view>

int main(int, char**) {
  std::ostringstream stream;
  stream << "stream output";
  stream.flush();
  if (stream.str() != "stream output")
    return 1;

  std::FILE* file = std::tmpfile();
  if (!file)
    return 2;
  std::print(file, "formatted {}", 42);
  std::fflush(file);
  std::rewind(file);
  char buffer[32]{};
  if (!std::fgets(buffer, sizeof(buffer), file))
    return 3;
  std::fclose(file);
  return std::string_view(buffer) == "formatted 42" ? 0 : 4;
}
