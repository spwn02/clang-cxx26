// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// Private implementation detail of libcxx/src/stacktrace.cpp: locates a named
// ELF section in an on-disk ELF32/ELF64 file and returns its raw bytes. Not a
// public libc++ header -- never included outside stacktrace.cpp.
//
// Scope: Linux x86_64 only, native byte order (no byte-swapping), ELF32 and
// ELF64. Sections marked SHF_COMPRESSED are treated as "not found" rather
// than decompressed -- compressed debug sections are an explicit, disclosed
// limitation of this pass (see the <stacktrace> implementation notes in
// stacktrace.cpp), not something silently mishandled.

#ifndef _LIBCPP_SRC_STACKTRACE_ELF_READER_H
#define _LIBCPP_SRC_STACKTRACE_ELF_READER_H

#include <__config>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

_LIBCPP_BEGIN_NAMESPACE_STD

namespace __stacktrace_detail {

namespace __elf {

inline bool __in_range(uint64_t __off, uint64_t __size, uint64_t __total) {
  return __off <= __total && __size <= __total - __off;
}

// Local copies of just the ELF32/ELF64 header fields this reader needs.
// Deliberately not <elf.h>: that system header's availability and exact
// field layout for non-Linux libc implementations isn't something to
// depend on here, and only a handful of fields are actually needed.
inline uint16_t __read_u16(const uint8_t* __p) {
  uint16_t __v;
  std::memcpy(&__v, __p, sizeof(__v));
  return __v;
}
inline uint32_t __read_u32(const uint8_t* __p) {
  uint32_t __v;
  std::memcpy(&__v, __p, sizeof(__v));
  return __v;
}
inline uint64_t __read_u64(const uint8_t* __p) {
  uint64_t __v;
  std::memcpy(&__v, __p, sizeof(__v));
  return __v;
}

} // namespace __elf

// Returns the raw bytes of the ELF section named __section_name in the file
// at __path, or an empty optional if the file can't be opened or read, isn't
// a recognizable little-endian ELF32/ELF64 file, has no section with that
// name, or that section is compressed. Never throws: every failure mode --
// missing file, truncated file, malformed header, out-of-range offsets --
// reports as an empty optional, since this runs during stack-trace
// resolution and must not itself crash the very program it's inspecting.
inline std::optional<std::vector<uint8_t>> __read_elf_section(const std::string& __path, const char* __section_name) {
  using namespace __elf;
  if (__section_name == nullptr)
    return std::nullopt;

#if _LIBCPP_HAS_EXCEPTIONS
  try {
#endif // _LIBCPP_HAS_EXCEPTIONS
    std::ifstream __in(__path, std::ios::binary | std::ios::ate);
    if (!__in)
      return std::nullopt;
    std::streamoff __n = __in.tellg();
    if (__n < 0)
      return std::nullopt;

    std::vector<uint8_t> __data(static_cast<size_t>(__n));
    __in.seekg(0);
    if (!__in.read(reinterpret_cast<char*>(__data.data()), __n))
      return std::nullopt;

    // e_ident: magic (4), EI_CLASS (1), EI_DATA (1, must be ELFDATA2LSB == 1
    // -- this reader only supports little-endian, which is all x86_64 uses).
    if (__data.size() < 16 || std::memcmp(__data.data(), "\177ELF", 4) != 0 || __data[5] != 1)
      return std::nullopt;
    const bool __is64 = __data[4] == 2;
    const bool __is32 = __data[4] == 1;
    if (!__is64 && !__is32)
      return std::nullopt;
    const size_t __ehdr_size = __is64 ? 64 : 52;
    if (__data.size() < __ehdr_size)
      return std::nullopt;

    // e_shoff, e_shentsize, e_shnum, e_shstrndx -- offsets per the ELF32/64
    // header layout (they differ only in e_shoff's width and position).
    const uint64_t __shoff     = __is64 ? __read_u64(__data.data() + 40) : __read_u32(__data.data() + 32);
    const uint16_t __shentsize = __read_u16(__data.data() + (__is64 ? 58 : 46));
    const uint16_t __shnum     = __read_u16(__data.data() + (__is64 ? 60 : 48));
    const uint16_t __shstrndx  = __read_u16(__data.data() + (__is64 ? 62 : 50));
    const size_t __shentsize_min = __is64 ? 64 : 40;
    if (__shnum == 0 || __shstrndx >= __shnum || __shentsize < __shentsize_min ||
        !__in_range(__shoff, uint64_t(__shentsize) * __shnum, __data.size()))
      return std::nullopt;

    auto __section_header = [&](uint16_t __i) { return __data.data() + __shoff + uint64_t(__i) * __shentsize; };

    // Section header string table: sh_offset/sh_size are at the same byte
    // offsets within a section header on both ELF32 and ELF64.
    const uint8_t* __shstrtab_hdr = __section_header(__shstrndx);
    const uint64_t __stroff       = __is64 ? __read_u64(__shstrtab_hdr + 24) : __read_u32(__shstrtab_hdr + 16);
    const uint64_t __strsize      = __is64 ? __read_u64(__shstrtab_hdr + 32) : __read_u32(__shstrtab_hdr + 20);
    if (!__in_range(__stroff, __strsize, __data.size()))
      return std::nullopt;
    const char* __strtab = reinterpret_cast<const char*>(__data.data() + __stroff);

    for (uint16_t __i = 0; __i != __shnum; ++__i) {
      const uint8_t* __sh    = __section_header(__i);
      const uint32_t __name  = __read_u32(__sh);
      const uint64_t __flags = __is64 ? __read_u64(__sh + 8) : __read_u32(__sh + 8);
      const uint64_t __off   = __is64 ? __read_u64(__sh + 24) : __read_u32(__sh + 16);
      const uint64_t __size  = __is64 ? __read_u64(__sh + 32) : __read_u32(__sh + 20);

      if (__name >= __strsize || std::memchr(__strtab + __name, '\0', __strsize - __name) == nullptr)
        continue;

      constexpr uint64_t __SHF_COMPRESSED = 0x800;
      if ((__flags & __SHF_COMPRESSED) != 0)
        continue; // out of scope for this pass -- see the file-level comment above.
      if (std::strcmp(__strtab + __name, __section_name) != 0)
        continue;

      if (!__in_range(__off, __size, __data.size()))
        return std::nullopt;
      return std::vector<uint8_t>(__data.begin() + __off, __data.begin() + __off + __size);
    }
#if _LIBCPP_HAS_EXCEPTIONS
  } catch (...) {
    return std::nullopt;
  }
#endif // _LIBCPP_HAS_EXCEPTIONS
  return std::nullopt;
}

} // namespace __stacktrace_detail

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_SRC_STACKTRACE_ELF_READER_H
