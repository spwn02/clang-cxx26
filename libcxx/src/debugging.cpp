//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include <__debugging/is_debugger_present.h>

#if defined(__linux__)
#  include <fcntl.h>
#  include <unistd.h>
#endif

_LIBCPP_BEGIN_UNVERSIONED_NAMESPACE_STD

bool is_debugger_present() noexcept {
#if defined(__linux__)
  const int __fd = ::open("/proc/self/status", O_RDONLY | O_CLOEXEC);
  if (__fd < 0)
    return false;

  char __status[4096];
  const ssize_t __size = ::read(__fd, __status, sizeof(__status));
  ::close(__fd);
  if (__size <= 0)
    return false;

  constexpr char __tracer_pid[] = "TracerPid:";
  for (ssize_t __index = 0; __index + static_cast<ssize_t>(sizeof(__tracer_pid) - 1) < __size; ++__index) {
    bool __matches = true;
    for (size_t __offset = 0; __offset != sizeof(__tracer_pid) - 1; ++__offset) {
      if (__status[__index + static_cast<ssize_t>(__offset)] != __tracer_pid[__offset]) {
        __matches = false;
        break;
      }
    }

    if (!__matches)
      continue;

    __index += sizeof(__tracer_pid) - 1;
    while (__index < __size && (__status[__index] == ' ' || __status[__index] == '\t'))
      ++__index;
    return __index < __size && __status[__index] != '0' && __status[__index] >= '1' && __status[__index] <= '9';
  }
#endif

  return false;
}

_LIBCPP_END_UNVERSIONED_NAMESPACE_STD
