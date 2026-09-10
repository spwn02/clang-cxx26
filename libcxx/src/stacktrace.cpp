//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include <__stacktrace/stacktrace_decls.h>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#if defined(__linux__)
#  include <cxxabi.h>
#  include <dlfcn.h>
#  include <unwind.h>

#  include "include/stacktrace/dwarf_line_table.h"
#  include "include/stacktrace/elf_reader.h"
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

namespace {

#if defined(__linux__)

struct __backtrace_state {
  uintptr_t* __pcs;
  size_t __capacity;
  size_t __count = 0;
  size_t __skip;
};

_Unwind_Reason_Code __backtrace_callback(_Unwind_Context* __context, void* __arg) {
  auto* __state  = static_cast<__backtrace_state*>(__arg);
  uintptr_t __pc = _Unwind_GetIP(__context);
  if (__pc == 0)
    return _URC_NO_REASON;
  if (__state->__skip > 0) {
    --__state->__skip;
    return _URC_NO_REASON;
  }
  if (__state->__count >= __state->__capacity)
    return _URC_END_OF_STACK;
  __state->__pcs[__state->__count++] = __pc;
  return _URC_NO_REASON;
}

// Per-module resolution cache. Parsing a module's DWARF line-number programs
// is comparatively expensive (opening and scanning the whole .debug_line
// section); dladdr()+ELF/DWARF resolution is only ever done once per module
// per process, the first time an address inside it needs resolving.
struct __module_info {
  bool __attempted_dwarf                                         = false;
  __stacktrace_detail::__dwarf_line_table* __line_table = nullptr; // owned, may be null.
  ~__module_info() { delete __line_table; }
};

std::mutex& __module_cache_mutex() {
  static std::mutex __m;
  return __m;
}

std::unordered_map<std::string, __module_info>& __module_cache() {
  static std::unordered_map<std::string, __module_info> __cache;
  return __cache;
}

// Looks up (creating and populating on first use) the parsed DWARF line
// table for the module containing __pc. Also fills __info with dladdr()'s
// own resolution (symbol name, module load base) for callers that need
// those regardless of whether DWARF resolution succeeded. Returns false only
// if dladdr() itself can't place __pc in any loaded module at all.
// *__out_line_table is left null if the module has no usable .debug_line (a
// stripped binary, a module without debug info, or a parse failure) --
// description() still works via dladdr()'s symbol table in that case; only
// source_file()/source_line() come back empty.
bool __resolve_module(uintptr_t __pc, Dl_info& __info, __stacktrace_detail::__dwarf_line_table** __out_line_table) {
  *__out_line_table = nullptr;
  if (::dladdr(reinterpret_cast<void*>(__pc), &__info) == 0 || __info.dli_fname == nullptr)
    return false;

  std::lock_guard<std::mutex> __lock(__module_cache_mutex());
  auto& __cache            = __module_cache();
  auto [__it, __inserted] = __cache.try_emplace(__info.dli_fname);
  __module_info& __mod    = __it->second;
  if (!__mod.__attempted_dwarf) {
    __mod.__attempted_dwarf = true;
    auto __debug_line       = __stacktrace_detail::__read_elf_section(__info.dli_fname, ".debug_line");
    if (__debug_line) {
      auto __debug_line_str = __stacktrace_detail::__read_elf_section(__info.dli_fname, ".debug_line_str");
      auto __debug_str      = __stacktrace_detail::__read_elf_section(__info.dli_fname, ".debug_str");
      static const std::vector<uint8_t> __empty;
      __mod.__line_table = new __stacktrace_detail::__dwarf_line_table(
          *__debug_line, __debug_line_str ? *__debug_line_str : __empty, __debug_str ? *__debug_str : __empty);
    }
  }
  *__out_line_table = __mod.__line_table;
  return true;
}

#endif // defined(__linux__)

} // namespace

vector<uintptr_t> __stacktrace_capture(size_t __skip, size_t __max_depth) noexcept {
  vector<uintptr_t> __result;
#if defined(__linux__)
  if (__max_depth == 0)
    return __result;
  size_t __capacity                    = 64;
  constexpr size_t __capacity_hard_cap = 1u << 20;
  for (;;) {
    size_t __this_capacity = std::min(__capacity, __max_depth);
    __result.assign(__this_capacity, uintptr_t{0});
    __backtrace_state __state{__result.data(), __this_capacity, 0, __skip};
    _Unwind_Backtrace(&__backtrace_callback, &__state);
    __result.resize(__state.__count);
    // A walk that filled the buffer exactly (__count == __this_capacity)
    // might have more frames beyond it -- unless we've already reached the
    // caller's own requested max_depth, in which case that's the correct,
    // intentionally-truncated final answer. Otherwise grow and redo the
    // whole walk (idempotent: __skip is reapplied from scratch).
    if (__state.__count < __this_capacity || __this_capacity >= __max_depth || __capacity >= __capacity_hard_cap)
      return __result;
    __capacity *= 4;
  }
#else
  (void)__skip;
  (void)__max_depth;
  return __result;
#endif
}

string __stacktrace_description(uintptr_t __pc) {
#if defined(__linux__)
  Dl_info __info{};
  __stacktrace_detail::__dwarf_line_table* __unused_table = nullptr;
  if (!__resolve_module(__pc, __info, &__unused_table) || __info.dli_sname == nullptr)
    return string();

  int __status     = 0;
  char* __demangled = abi::__cxa_demangle(__info.dli_sname, nullptr, nullptr, &__status);
  string __result   = (__status == 0 && __demangled != nullptr) ? string(__demangled) : string(__info.dli_sname);
  std::free(__demangled);
  return __result;
#else
  (void)__pc;
  return string();
#endif
}

string __stacktrace_source_file(uintptr_t __pc) {
#if defined(__linux__)
  Dl_info __info{};
  __stacktrace_detail::__dwarf_line_table* __line_table = nullptr;
  if (!__resolve_module(__pc, __info, &__line_table) || __line_table == nullptr)
    return string();
  uintptr_t __bias  = reinterpret_cast<uintptr_t>(__info.dli_fbase);
  const auto* __row = __line_table->find(static_cast<uint64_t>(__pc - __bias));
  return __row ? string(__row->file_path) : string();
#else
  (void)__pc;
  return string();
#endif
}

uint_least32_t __stacktrace_source_line(uintptr_t __pc) {
#if defined(__linux__)
  Dl_info __info{};
  __stacktrace_detail::__dwarf_line_table* __line_table = nullptr;
  if (!__resolve_module(__pc, __info, &__line_table) || __line_table == nullptr)
    return 0;
  uintptr_t __bias  = reinterpret_cast<uintptr_t>(__info.dli_fbase);
  const auto* __row = __line_table->find(static_cast<uint64_t>(__pc - __bias));
  return __row ? static_cast<uint_least32_t>(__row->line) : 0;
#else
  (void)__pc;
  return 0;
#endif
}

_LIBCPP_END_NAMESPACE_STD
