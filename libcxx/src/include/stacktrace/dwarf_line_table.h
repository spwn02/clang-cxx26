// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// Private implementation detail of libcxx/src/stacktrace.cpp: a from-scratch
// parser for the DWARF ".debug_line" section's line-number programs (DWARF
// version 4 and 5 -- other versions are skipped, not crashed on), per DWARF5
// spec section 6.2 (DWARF4's header is the same idea with a simpler,
// non-form-coded directory/file table). Not a public libc++ header.
//
// Deliberately does NOT walk .debug_info/.debug_abbrev to correlate PCs to
// compilation units: every line-number program in .debug_line is
// self-contained and directly emits (address, file, line) rows, so this
// parses the whole section once per module into one sorted, binary-searchable
// table instead.
//
// Disclosed limitations of this pass (see <stacktrace>'s implementation
// notes in stacktrace.cpp): VLIW op_index is not tracked (always treated as
// 0, which is correct for x86_64); nonzero DWARF5 segment selectors are not
// supported; MD5 checksum fields (DW_FORM_data16) are consumed and discarded,
// not verified.

#ifndef _LIBCPP_SRC_STACKTRACE_DWARF_LINE_TABLE_H
#define _LIBCPP_SRC_STACKTRACE_DWARF_LINE_TABLE_H

#include <__config>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

_LIBCPP_BEGIN_NAMESPACE_STD

namespace __stacktrace_detail {

struct __dwarf_line_row {
  uint64_t address;
  std::string file_path;
  uint32_t line;
};

namespace __dwarf {

// DW_FORM_* values this parser understands in a directory/file entry format
// descriptor (DWARF5's directory_entry_format / file_name_entry_format). Any
// other form aborts just the current line-number program, not the whole
// section -- see __skip_unit below.
enum __form : uint64_t {
  __form_string   = 0x08, // inline NUL-terminated string
  __form_strp     = 0x0e, // 4/8-byte offset into .debug_str
  __form_line_strp = 0x1f, // 4/8-byte offset into .debug_line_str
  __form_udata    = 0x0f, // ULEB128
  __form_data1    = 0x0b,
  __form_data2    = 0x05,
  __form_data4    = 0x06,
  __form_data8    = 0x07,
  __form_data16   = 0x1e, // fixed 16-byte block (e.g. an MD5 checksum) -- consumed, not interpreted.
};

// DW_LNS_* standard opcodes (1..12); values >= opcode_base with no meaning
// here are vendor/unknown standard opcodes, skipped via standard_opcode_lengths.
enum __std_opcode : uint8_t {
  __lns_copy               = 1,
  __lns_advance_pc         = 2,
  __lns_advance_line       = 3,
  __lns_set_file           = 4,
  __lns_set_column         = 5,
  __lns_negate_stmt        = 6,
  __lns_set_basic_block    = 7,
  __lns_const_add_pc       = 8,
  __lns_fixed_advance_pc   = 9,
  __lns_set_prologue_end   = 10,
  __lns_set_epilogue_begin = 11,
  __lns_set_isa            = 12,
};

// DW_LNE_* extended opcodes.
enum __ext_opcode : uint8_t {
  __lne_end_sequence      = 1,
  __lne_set_address       = 2,
  __lne_define_file       = 3, // DWARF <= 4 only.
  __lne_set_discriminator = 4,
};

// A small bounds-checked cursor over a byte range. Every read reports
// success/failure instead of asserting or throwing: a truncated or malformed
// line-number program must abort just that one program (see __skip_unit),
// never crash the resolver that's trying to symbolicate a crash.
struct __cursor {
  const uint8_t* __p;
  const uint8_t* __e;

  bool __read_bytes(void* __v, size_t __n) {
    if (static_cast<size_t>(__e - __p) < __n)
      return false;
    std::memcpy(__v, __p, __n);
    __p += __n;
    return true;
  }
  bool __read_u8(uint8_t& __v) { return __read_bytes(&__v, 1); }
  bool __read_u16(uint16_t& __v) { return __read_bytes(&__v, 2); }
  bool __read_u32(uint32_t& __v) { return __read_bytes(&__v, 4); }
  bool __read_u64(uint64_t& __v) { return __read_bytes(&__v, 8); }

  // A DWARF "offset" is 4 bytes in 32-bit DWARF format, 8 bytes in 64-bit
  // DWARF format (the format used by one line-number program, determined by
  // whether its unit_length began with the 0xffffffff escape).
  bool __read_offset(uint64_t& __v, bool __dwarf64) {
    if (__dwarf64)
      return __read_u64(__v);
    uint32_t __v32;
    if (!__read_u32(__v32))
      return false;
    __v = __v32;
    return true;
  }

  bool __read_uleb128(uint64_t& __v) {
    __v            = 0;
    unsigned __shift = 0;
    for (;;) {
      if (__shift >= 64)
        return false;
      uint8_t __byte;
      if (!__read_u8(__byte))
        return false;
      __v |= uint64_t(__byte & 0x7f) << __shift;
      if ((__byte & 0x80) == 0)
        return true;
      __shift += 7;
    }
  }

  bool __read_sleb128(int64_t& __v) {
    __v              = 0;
    unsigned __shift  = 0;
    uint8_t __byte;
    do {
      if (__shift >= 64 || !__read_u8(__byte))
        return false;
      __v |= int64_t(__byte & 0x7f) << __shift;
      __shift += 7;
    } while ((__byte & 0x80) != 0);
    if (__shift < 64 && (__byte & 0x40) != 0)
      __v |= -(int64_t(1) << __shift); // sign-extend
    return true;
  }

  bool __read_cstr(std::string& __s) {
    const uint8_t* __q = __p;
    while (__q != __e && *__q != 0)
      ++__q;
    if (__q == __e)
      return false; // unterminated -- treat as malformed, not a silent truncation.
    __s.assign(reinterpret_cast<const char*>(__p), __q - __p);
    __p = __q + 1;
    return true;
  }

  bool __skip_bytes(size_t __n) {
    if (static_cast<size_t>(__e - __p) < __n)
      return false;
    __p += __n;
    return true;
  }
};

inline std::string __string_at_offset(const std::vector<uint8_t>& __section, uint64_t __offset) {
  if (__offset >= __section.size())
    return std::string();
  const uint8_t* __p = __section.data() + __offset;
  const uint8_t* __e = __section.data() + __section.size();
  const uint8_t* __q = __p;
  while (__q != __e && *__q != 0)
    ++__q;
  return __q == __e ? std::string() : std::string(reinterpret_cast<const char*>(__p), __q - __p);
}

} // namespace __dwarf

class __dwarf_line_table {
public:
  // debug_line_str/debug_str may be empty (e.g. a pure DWARF4 module with no
  // .debug_str, or a section that genuinely doesn't exist) -- forms that
  // need a missing section just resolve to an empty string rather than
  // failing the whole parse.
  __dwarf_line_table(
      const std::vector<uint8_t>& __debug_line,
      const std::vector<uint8_t>& __debug_line_str,
      const std::vector<uint8_t>& __debug_str) {
    using namespace __dwarf;

    const uint8_t* __unit_begin = __debug_line.data();
    const uint8_t* __section_end = __debug_line.data() + __debug_line.size();

    while (static_cast<size_t>(__section_end - __unit_begin) >= 4) {
      __cursor __c{__unit_begin, __section_end};

      // unit_length (+ the 64-bit DWARF escape).
      uint32_t __len32;
      if (!__c.__read_u32(__len32))
        break;
      bool __dwarf64  = __len32 == 0xffffffffu;
      uint64_t __len = __len32;
      if (__dwarf64 && !__c.__read_u64(__len))
        break;
      const uint8_t* __unit_end = __c.__p;
      if (__len > static_cast<uint64_t>(__section_end - __unit_end))
        break; // corrupt length -- nothing safe to skip to, stop parsing this section.
      __unit_end += __len;

      // Any early return from here to the bottom of the loop body must
      // resume parsing at __unit_end, i.e. skip only this one unit.
      if (!__parse_one_unit(__c, __unit_end, __dwarf64, __debug_line_str, __debug_str))
        /* fall through: unit was malformed past a recoverable point, or
           finished normally -- either way, move on to the next unit. */;

      __unit_begin = __unit_end;
    }

    std::sort(__rows.begin(), __rows.end(), [](const __dwarf_line_row& __a, const __dwarf_line_row& __b) {
      return __a.address < __b.address;
    });
  }

  // Returns the row with the largest address <= __pc, or nullptr if __pc is
  // before every row (including when the table is empty).
  const __dwarf_line_row* find(uint64_t __pc) const noexcept {
    auto __it = std::upper_bound(
        __rows.begin(), __rows.end(), __pc, [](uint64_t __x, const __dwarf_line_row& __r) { return __x < __r.address; });
    if (__it == __rows.begin())
      return nullptr;
    return &*--__it;
  }

private:
  std::vector<__dwarf_line_row> __rows;

  // Decodes one directory/file-table entry's field (DWARF5's form-coded
  // tables), advancing __c. On success, writes the decoded string to __s (for
  // string-like forms) or the decoded integer to __n (for numeric forms);
  // forms that carry no useful information for this parser (e.g. an MD5
  // checksum) are consumed but leave __s/__n untouched. Returns false for an
  // unrecognized form -- the caller aborts just the current unit.
  static bool __decode_form(
      __dwarf::__cursor& __c,
      uint64_t __form,
      const std::vector<uint8_t>& __debug_line_str,
      const std::vector<uint8_t>& __debug_str,
      bool __dwarf64,
      std::string& __s,
      uint64_t& __n) {
    using namespace __dwarf;
    switch (__form) {
    case __form_string:
      return __c.__read_cstr(__s);
    case __form_strp: {
      uint64_t __off;
      if (!__c.__read_offset(__off, __dwarf64))
        return false;
      __s = __dwarf::__string_at_offset(__debug_str, __off);
      return true;
    }
    case __form_line_strp: {
      uint64_t __off;
      if (!__c.__read_offset(__off, __dwarf64))
        return false;
      __s = __dwarf::__string_at_offset(__debug_line_str, __off);
      return true;
    }
    case __form_udata:
      return __c.__read_uleb128(__n);
    case __form_data1: {
      uint8_t __v;
      if (!__c.__read_u8(__v))
        return false;
      __n = __v;
      return true;
    }
    case __form_data2: {
      uint16_t __v;
      if (!__c.__read_u16(__v))
        return false;
      __n = __v;
      return true;
    }
    case __form_data4: {
      uint32_t __v;
      if (!__c.__read_u32(__v))
        return false;
      __n = __v;
      return true;
    }
    case __form_data8:
      return __c.__read_u64(__n);
    case __form_data16:
      // A fixed-size 16-byte block (e.g. an MD5 checksum, DW_LNCT_MD5). This
      // toolchain's clang emits one of these for every file entry by
      // default; it carries no information this line-table cares about, but
      // MUST be consumed so later fields/rows parse at the right offset.
      return __c.__skip_bytes(16);
    default:
      return false;
    }
  }

  // Parses one DWARF5-style form-coded directory or file-name table:
  //   count_of_formats: u8, then that many (content_type: ULEB, form: ULEB)
  //   entry_count: ULEB, then that many entries, each with one value per
  //   declared format entry (in order).
  // For each table entry, __out receives the LAST string-valued field
  // decoded (in practice, exactly one format entry is the path -- typically
  // DW_LNCT_path -- with any others, like DW_LNCT_directory_index or
  // DW_LNCT_MD5, contributing only their (discarded, for the directory
  // table) or separately-tracked (file table's directory index) values).
  // __out_index, if non-null, receives the last numeric field (used for the
  // file table's directory index).
  static bool __read_v5_table(
      __dwarf::__cursor& __c,
      const std::vector<uint8_t>& __debug_line_str,
      const std::vector<uint8_t>& __debug_str,
      bool __dwarf64,
      std::vector<std::string>& __out,
      std::vector<uint64_t>* __out_index) {
    uint8_t __format_count;
    if (!__c.__read_u8(__format_count))
      return false;
    std::vector<uint64_t> __forms(__format_count);
    for (auto& __form : __forms) {
      uint64_t __content_type;
      if (!__c.__read_uleb128(__content_type) || !__c.__read_uleb128(__form))
        return false;
    }

    uint64_t __entry_count;
    if (!__c.__read_uleb128(__entry_count))
      return false;

    for (uint64_t __i = 0; __i < __entry_count; ++__i) {
      std::string __s;
      uint64_t __n = 0;
      for (uint64_t __form : __forms) {
        if (!__decode_form(__c, __form, __debug_line_str, __debug_str, __dwarf64, __s, __n))
          return false;
      }
      __out.push_back(std::move(__s));
      if (__out_index)
        __out_index->push_back(__n);
    }
    return true;
  }

  // Parses one line-number program's header (directory/file tables) and then
  // runs its opcode state machine, appending rows to __rows. Returns false
  // if the header itself was malformed past a recoverable point (the caller
  // still advances past the whole unit either way -- there's nothing more to
  // recover). A state-machine failure partway through simply stops emitting
  // rows for this one unit; rows already emitted are kept.
  bool __parse_one_unit(
      __dwarf::__cursor __c,
      const uint8_t* __unit_end,
      bool __dwarf64,
      const std::vector<uint8_t>& __debug_line_str,
      const std::vector<uint8_t>& __debug_str) {
    using namespace __dwarf;

    uint16_t __version;
    if (!__c.__read_u16(__version) || (__version != 4 && __version != 5))
      return false;

    uint8_t __address_size = 8, __segment_selector_size = 0;
    if (__version == 5 && (!__c.__read_u8(__address_size) || !__c.__read_u8(__segment_selector_size)))
      return false;

    uint64_t __header_length;
    if (!__c.__read_offset(__header_length, __dwarf64) ||
        __header_length > static_cast<uint64_t>(__unit_end - __c.__p))
      return false;
    const uint8_t* __program_begin = __c.__p + __header_length;

    uint8_t __min_instr_len, __max_ops_per_instr = 1, __default_is_stmt, __line_base_u8, __line_range, __opcode_base;
    if (!__c.__read_u8(__min_instr_len))
      return false;
    if (__version >= 4 && !__c.__read_u8(__max_ops_per_instr)) // present since DWARF4; unused (op_index == 0 always).
      return false;
    if (!__c.__read_u8(__default_is_stmt) || !__c.__read_u8(__line_base_u8) || !__c.__read_u8(__line_range) ||
        !__c.__read_u8(__opcode_base))
      return false;
    if (__opcode_base == 0)
      return false;
    const int8_t __line_base = static_cast<int8_t>(__line_base_u8);

    std::vector<uint8_t> __standard_opcode_lengths(__opcode_base - 1);
    for (auto& __len : __standard_opcode_lengths) {
      if (!__c.__read_u8(__len))
        return false;
    }

    // Directory and file-name tables. DWARF5 tables are genuinely 0-indexed
    // -- their first parsed entry IS index 0, matching the opcode state
    // machine's 'file'/'directory' register conventions for this version --
    // so no placeholder is added for version 5. DWARF<=4 tables are
    // 1-indexed (index 0 conventionally denotes "the primary source file"
    // and is never itself listed in the table), so a single leading empty
    // placeholder is added there, and only there, to keep parsed-entry N
    // sitting at __files[N] for version 4 too.
    std::vector<std::string> __dirs;
    std::vector<std::string> __files;
    std::vector<uint64_t> __file_dirs;

    if (__version == 4) {
      __dirs.emplace_back();
      __files.emplace_back();
      __file_dirs.emplace_back();

      std::string __s;
      while (__c.__p < __program_begin && __c.__read_cstr(__s) && !__s.empty())
        __dirs.push_back(__s);
      while (__c.__p < __program_begin && __c.__read_cstr(__s) && !__s.empty()) {
        uint64_t __dir_index, __mtime, __length;
        if (!__c.__read_uleb128(__dir_index) || !__c.__read_uleb128(__mtime) || !__c.__read_uleb128(__length))
          return false;
        __files.push_back(__s);
        __file_dirs.push_back(__dir_index);
      }
    } else {
      if (!__read_v5_table(__c, __debug_line_str, __debug_str, __dwarf64, __dirs, nullptr))
        return false;
      if (!__read_v5_table(__c, __debug_line_str, __debug_str, __dwarf64, __files, &__file_dirs))
        return false;
    }

    if (__c.__p != __program_begin)
      return false; // header_length promised exactly this many bytes; a mismatch means our parse drifted.

    // --- Opcode state machine (DWARF5 6.2.5) ---
    __c.__e             = __unit_end;
    uint64_t __address  = 0;
    uint64_t __file      = __version == 5 ? 0 : 1; // matches each version's file-table indexing convention.
    int64_t __line       = 1;
    bool __is_stmt       = __default_is_stmt != 0;

    auto __resolve_path = [&](uint64_t __file_index) -> std::string {
      if (__file_index >= __files.size())
        return std::string();
      std::string __path = __files[__file_index];
      uint64_t __dir_index = __file_index < __file_dirs.size() ? __file_dirs[__file_index] : 0;
      if (__dir_index < __dirs.size() && !__dirs[__dir_index].empty() && !__path.empty() && __path.front() != '/')
        __path = __dirs[__dir_index] + "/" + __path;
      return __path;
    };
    auto __emit_row = [&] { __rows.push_back({__address, __resolve_path(__file), static_cast<uint32_t>(__line)}); };

    // Hard cap: guards against a malformed program (e.g. a length field that
    // lets the loop condition below never naturally terminate) looping
    // forever while resolving a stack trace.
    constexpr size_t __opcode_iteration_cap = 10'000'000;
    for (size_t __iterations = 0; __c.__p < __unit_end && __iterations < __opcode_iteration_cap; ++__iterations) {
      uint8_t __opcode;
      if (!__c.__read_u8(__opcode))
        return true; // ran out of program bytes -- keep whatever rows were already emitted.

      if (__opcode == 0) {
        // Extended opcode: ULEB128 length, then that many bytes (a 1-byte
        // sub-opcode followed by its own operands).
        uint64_t __ext_len;
        if (!__c.__read_uleb128(__ext_len) || __ext_len == 0 ||
            __ext_len > static_cast<uint64_t>(__unit_end - __c.__p))
          return true;
        const uint8_t* __ext_end = __c.__p + __ext_len;
        uint8_t __sub_opcode;
        if (!__c.__read_u8(__sub_opcode))
          return true;
        switch (__sub_opcode) {
        case __lne_end_sequence:
          __emit_row();
          __address = 0;
          __file     = __version == 5 ? 0 : 1;
          __line     = 1;
          __is_stmt  = __default_is_stmt != 0;
          break;
        case __lne_set_address:
          if (__address_size == 8) {
            uint64_t __a;
            if (!__c.__read_u64(__a))
              return true;
            __address = __a;
          } else if (__address_size == 4) {
            uint32_t __a;
            if (!__c.__read_u32(__a))
              return true;
            __address = __a;
          } else {
            return true; // unsupported address size.
          }
          break;
        case __lne_define_file: { // DWARF <= 4 only, but harmless to accept unconditionally.
          std::string __name;
          uint64_t __dir_index, __mtime, __length;
          if (!__c.__read_cstr(__name) || !__c.__read_uleb128(__dir_index) || !__c.__read_uleb128(__mtime) ||
              !__c.__read_uleb128(__length))
            return true;
          __files.push_back(__name);
          __file_dirs.push_back(__dir_index);
          break;
        }
        case __lne_set_discriminator: {
          uint64_t __discriminator;
          if (!__c.__read_uleb128(__discriminator))
            return true;
          break;
        }
        default:
          break; // unknown extended opcode -- __ext_len already tells us how far to skip.
        }
        __c.__p = __ext_end;
        (void)__is_stmt; // tracked for fidelity with the state machine; not consumed by find().
        continue;
      }

      if (__opcode < __opcode_base) {
        // Standard opcode.
        uint64_t __u;
        int64_t __s;
        switch (__opcode) {
        case __lns_copy:
          __emit_row();
          break;
        case __lns_advance_pc:
          if (!__c.__read_uleb128(__u))
            return true;
          __address += __u * __min_instr_len;
          break;
        case __lns_advance_line:
          if (!__c.__read_sleb128(__s))
            return true;
          __line += __s;
          break;
        case __lns_set_file:
          if (!__c.__read_uleb128(__file))
            return true;
          break;
        case __lns_set_column:
          if (!__c.__read_uleb128(__u))
            return true;
          break;
        case __lns_negate_stmt:
          __is_stmt = !__is_stmt;
          break;
        case __lns_set_basic_block:
          break;
        case __lns_const_add_pc:
          __address += ((255 - __opcode_base) / __line_range) * __min_instr_len;
          break;
        case __lns_fixed_advance_pc: {
          uint16_t __adv;
          if (!__c.__read_u16(__adv))
            return true;
          __address += __adv; // NOT scaled by minimum_instruction_length -- see DWARF5 6.2.5.2.
          break;
        }
        case __lns_set_prologue_end:
        case __lns_set_epilogue_begin:
          break;
        case __lns_set_isa:
          if (!__c.__read_uleb128(__u))
            return true;
          break;
        default:
          // A vendor-defined standard opcode we don't recognize: skip
          // exactly as many ULEB128 operands as its declared length says.
          for (uint8_t __k = 0; __k < __standard_opcode_lengths[__opcode - 1]; ++__k) {
            if (!__c.__read_uleb128(__u))
              return true;
          }
          break;
        }
        continue;
      }

      // Special opcode (DWARF5 6.2.5.1): simultaneously advances address and
      // line, then emits a row.
      uint8_t __adjusted = __opcode - __opcode_base;
      __address += (__adjusted / __line_range) * __min_instr_len;
      __line += __line_base + (__adjusted % __line_range);
      __emit_row();
    }
    return true;
  }
};

} // namespace __stacktrace_detail

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_SRC_STACKTRACE_DWARF_LINE_TABLE_H
