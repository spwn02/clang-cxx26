//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20 || c++23 || c++26

// P4380R1 section 3.10, "Logging" (blocks 059 and 060), run: a member macro per
// enumerator of LogLevel, injected by a consteval block of the class, that
// does not evaluate its arguments when the level is filtered. The output is
// collected in a string instead of printed, and a formatter for the scoped
// enumeration is supplied (the paper's example prints the level with {}, which
// std::format does not support for a scoped enumeration).

// RUN: %{build}
// RUN: %{exec} %t.exe

#include <cassert>
#include <format>
#include <meta>
#include <string>
#include <string_view>

#include "test_macros.h"

static std::string g_out;

enum class LogLevel { debug, info, error };
template <>
struct std::formatter<LogLevel> : std::formatter<std::string_view> {
  auto format(LogLevel l, auto &ctx) const {
    return std::formatter<std::string_view>::format(l == LogLevel::debug ? "debug" : l == LogLevel::info ? "info" : "error", ctx);
  }
};


struct Logger {
    LogLevel min_level;

    explicit Logger(LogLevel min_level) : min_level(min_level) { }

    auto do_log(LogLevel level, std::string_view fmt, auto&&... args) -> void {
        g_out += std::format("[{}] {}\n", level, std::vformat(fmt, std::make_format_args(args...)));
    }

    consteval {
        for (std::meta::info e : enumerators_of(^^LogLevel)) {
            auto name = std::meta::id(identifier_of(e));
            queue_injection(^^{
                template <class... Args>
                __macro \(name)(this Logger& self, std::string_view raw, Args&&... args) {
                    auto fs = substitute(^^::std::basic_format_string, {^^char, ^^Args...});
                    constexpr auto level = ::LogLevel::\(name);

                    auto call_args = std::meta::list_builder(^^{ , });
                    call_args += ^^{ fmt };
                    ((call_args += ^^{ \(args) }), ...);
                    return ^^{
                        do {
                            constexpr std::string_view fmt = \(fs)(\(raw)).get();
                            auto& self = \(self);
                            if (\(level) >= self.min_level) {
                                self.do_log(\(level), \(call_args));
                            }
                        }
                    };
                }
            });
        }
    }
};


auto get() -> int {
    g_out += "** called get **\n";
    return 6;
}

void run_logging() {
    auto log = Logger(LogLevel::info);
    int x = 5;
    log.debug!("x={} y={}", x, get());
    log.info!("{:>3}|{}", "ab", 2.5);
    log.error!("plain");
}


int main(int, char**) {
  run_logging();
  // debug is below the minimum level: not logged, and get() is not called
  assert(g_out == "[info]  ab|2.5\n[error] plain\n");
  return 0;
}
